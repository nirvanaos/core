#include "FileAccessDirect.h"
#include <Port/Memory.h>

using namespace std;

namespace Nirvana {
namespace Core {

FileAccessDirect::~FileAccessDirect ()
{
	for (Cache::iterator it = cache_.begin (); it != cache_.end ();) {
		if (it->second.request)
			complete_request (*it->second.request);
		Port::Memory::release (it->second.buffer, block_size_);
	}
}

void FileAccessDirect::read (uint64_t pos, uint32_t size, vector <uint8_t>& data)
{
	if (pos > file_size_)
		throw RuntimeError (ESPIPE);
	Pos end = (Pos)pos + size;
	if (end > file_size_) {
		end = file_size_;
		size = (uint32_t)(file_size_ - pos);
	}

	BlockIdx begin_block = pos / block_size_, end_block = (end + block_size_ - 1) / block_size_;

	clear_cache (begin_block, end_block);

	if (!size)
		return;

	CacheRange blocks = request_read (begin_block, end_block);
	try {
		Cache::iterator block = blocks.begin;
			// Copy first block
		{
			complete_request (block, Request::OP_READ);
			// Reserve space when read across multiple blocks
			Cache::iterator second_block = block;
			if (blocks.end != ++second_block)
				data.reserve (size);
			Size off = pos % block_size_;
			const uint8_t* bl = (uint8_t*)block->second.buffer + off;
			Size cb = min (size, block_size_ - off);
			data.insert (data.end (), bl, bl + cb);
			size -= cb;
			block = second_block;
		}
		// Copy remaining blocks
		while (block != blocks.end) {
			complete_request (block, Request::OP_READ);
			const uint8_t* bl = (uint8_t*)block->second.buffer;
			Size cb = min (size, block_size_);
			data.insert (data.end (), bl, bl + cb);
			size -= cb;
			++block;
		}
		// Set read time and unlock blocks
		Chrono::Duration time = Chrono::steady_clock ();
		while (blocks.begin != blocks.end) {
			blocks.begin->second.last_read_time = time;
			unlock (blocks.begin);
			++blocks.begin;
		}

	} catch (...) {
		while (blocks.begin != blocks.end) {
			unlock (blocks.begin);
			++blocks.begin;
		}
		throw;
	}

	write_dirty_blocks (WRITE_TIMEOUT);
}

FileAccessDirect::CacheRange FileAccessDirect::request_read (BlockIdx begin_block, BlockIdx end_block)
{
	CacheRange blocks (cache_);
	try {
		for (Cache::iterator cached_block = cache_.lower_bound (begin_block);;) {
			BlockIdx not_cached_end;
			if (cached_block == cache_.end ())
				not_cached_end = end_block;
			else
				not_cached_end = min (end_block, cached_block->first);
			if (begin_block < not_cached_end) {
				// Issue new read request
				Size cb = (Size)(not_cached_end - begin_block) * block_size_;
				void* buffer = Port::Memory::allocate (nullptr, cb, 0);
				Request* request = nullptr;
				unsigned new_blocks = 0;
				try {
					for (uint8_t* block_buf = (uint8_t*)buffer;;) {
						Cache::iterator it = cache_.emplace_hint (cached_block, begin_block, block_buf);
						it->second.request = request;
						blocks.append (it);
						++new_blocks;
						if (!request)
							request = new Request (IO_Request::OP_READ, begin_block * block_size_, buffer, cb, it);
						if (not_cached_end == ++begin_block)
							break;
						block_buf += block_size_;
					}
				} catch (...) {
					delete request;
					while (new_blocks--) {
						cache_.erase (--blocks.end);
					}
					Port::Memory::release (buffer, cb);
					throw;
				}
				issue_request (*request);
				if (begin_block == end_block)
					break;
			}
			++(cached_block->second.lock_cnt);
			blocks.append (cached_block);
			if (end_block == ++begin_block)
				break;
			++cached_block;
		}

	} catch (...) {
		while (blocks.begin != blocks.end) {
			unlock (blocks.begin);
			++blocks.begin;
		}
		throw;
	}

	return blocks;
}

void FileAccessDirect::write (uint64_t pos, const vector <uint8_t>& data)
{
	if (pos == numeric_limits <uint64_t>::max ())
		pos = file_size_;
	Pos end = (Pos)pos + data.size ();
	BlockIdx cur_block = pos / block_size_;
	BlockIdx end_block = (end + block_size_ - 1) / block_size_;

	clear_cache (cur_block, end_block);

	if (!data.empty ())
		return;

	struct ReadRange
	{
		BlockIdx start;
		unsigned count;
	} read_ranges [2] = { 0 }; // Head, tail

	if (file_size_ > pos) {

		if (pos % block_size_) {
			// We need to read the first block before writing to it.
			read_ranges [0].start = cur_block;
			read_ranges [0].count = 1;
		}

		Pos read_end = min (end, file_size_);
		if (end % block_size_) {
			BlockIdx tail = end / block_size_;
			if ((Pos)tail * (Pos)block_size_ < file_size_) {
				// We need to read last block before writing to it.
				if (read_ranges [0].count && tail <= read_ranges [0].start + 1) {
					if (tail > read_ranges [0].start)
						++read_ranges [0].count;
				} else {
					read_ranges [1].start = tail;
					read_ranges [1].count = 1;
				}
			}
		}
	}

	Cache::iterator read_blocks [2] = { cache_.end (), cache_.end () }; // Head, tail
	try {
		if (read_ranges [0].count) {
			CacheRange blocks = request_read (read_ranges [0].start, read_ranges [0].count);
			assert (blocks.begin != blocks.end);
			read_blocks [0] = blocks.begin;
			if (blocks.end != ++blocks.begin) {
				assert (!read_ranges [1].count);
				read_blocks [1] = blocks.begin;
			}
		}
		if (read_ranges [1].count) {
			CacheRange blocks = request_read (read_ranges [0].start, read_ranges [0].count);
			assert (blocks.begin != blocks.end);
			read_blocks [1] = blocks.begin;
			assert (blocks.end == ++blocks.begin);
		}

		// Try to decide first block iterator in cache without the search
		Cache::iterator cached_block = read_blocks [0];
		if (cached_block == cache_.end ()) {
			if (read_blocks [1] != cache_.end ()) {
				cached_block = read_blocks [1];
				assert (cached_block != cache_.begin ());
				--cached_block;
				if (cached_block->first != cur_block)
					cached_block = cache_.end ();
			}
		}
		// Find next cached block
		if (cached_block == cache_.end ())
			cached_block = cache_.lower_bound (cur_block);

		// Copy data to cache
		const uint8_t* src_data = data.data ();
		size_t src_size = data.size ();
		size_t block_offset = pos % block_size_;
		for (;; block_offset = 0) {
			BlockIdx not_cached_end;
			if (cached_block == cache_.end ())
				not_cached_end = end_block;
			else
				not_cached_end = min (end_block, cached_block->first);
			if (cur_block < not_cached_end) {
				// Insert new blocks to cache
				Size cb = (Size)(not_cached_end - cur_block) * block_size_;
				uint8_t* buffer = nullptr;
				CacheRange new_blocks (cache_);
				try {
					size_t cb_copy;
					if (!block_offset && src_size >= cb) {
						buffer = (uint8_t*)Port::Memory::copy (nullptr, const_cast <uint8_t*> (src_data), cb, 0);
						cb_copy = cb;
					} else {
						buffer = (uint8_t*)Port::Memory::allocate (nullptr, cb, 0);
						cb_copy = max (cb - block_offset, src_size);
						Port::Memory::copy (buffer + block_offset, const_cast <uint8_t*> (src_data), cb_copy, 0);
					}
					src_data += cb_copy;
					src_size -= cb_copy;
					Pos end = (Pos)cur_block * (Pos)block_size_ + cb_copy;
					Chrono::Duration time = Chrono::steady_clock ();
					for (uint8_t* block_buf = buffer;;) {
						Cache::iterator it = cache_.emplace_hint (cached_block, cur_block, block_buf);
						size_t dirty_size = max ((size_t)block_size_ - block_offset, cb_copy);
						set_dirty (it, time, block_offset, dirty_size);
						block_offset = 0;
						cb_copy -= dirty_size;
						new_blocks.append (it);
						if (not_cached_end == ++cur_block)
							break;
						block_buf += block_size_;
					}

					// Update file size
					if (file_size_ < end)
						file_size_ = end;
				} catch (...) {
					while (new_blocks.begin != new_blocks.end)
						cache_.erase (--new_blocks.end);
					Port::Memory::release (buffer, cb);
					throw;
				}
				block_offset = 0;
				if (cur_block == end_block)
					break;
			}
			// Write to cached block
			complete_request (cached_block); // Complete previous read/write operation
			size_t cb_copy = max (block_size_ - block_offset, src_size);
			Port::Memory::copy ((uint8_t*)cached_block->second.buffer + block_offset, const_cast <uint8_t*> (src_data), cb_copy, 0);
			set_dirty (cached_block, Chrono::steady_clock (), block_offset, cb_copy);

			// Update file size
			Pos end = (Pos)cached_block->first * (Pos)block_size_ + block_offset + cb_copy;
			if (file_size_ < end)
				file_size_ = end;

			// If this block is readed block, unlock it
			for (auto p = read_blocks; p != std::end (read_blocks); ++p) {
				Cache::iterator block = *p;
				if (block == cached_block) {
					unlock (block);
					*p = cache_.end ();
					break;
				}
			}
			src_data += cb_copy;
			src_size -= cb_copy;
			if (end_block == ++cur_block)
				break;
			++cached_block;
		}

	} catch (...) {
		for (auto p = read_blocks; p != std::end (read_blocks); ++p) {
			Cache::iterator block = *p;
			if (block != cache_.end ())
				unlock (block);
		}
		throw;
	}

	write_dirty_blocks (WRITE_TIMEOUT);
}

void FileAccessDirect::set_dirty (Cache::iterator it, size_t offset, size_t size) NIRVANA_NOEXCEPT
{
	assert (size > 0 && size <= block_size_);
	if (!it->second.dirty ())
		++dirty_blocks_;
	size_t block = offset / base_block_size_;
	if (it->second.dirty_begin > block)
		it->second.dirty_begin = (uint8_t)block;
	block = (offset + size + base_block_size_ - 1) / base_block_size_;
	if (it->second.dirty_end < block)
		it->second.dirty_end = (uint8_t)block;
}

void FileAccessDirect::write_dirty_blocks (Chrono::Duration timeout)
{
	for (Cache::iterator block = cache_.begin (); dirty_blocks_ && block != cache_.end (); ) {
		if (block->second.dirty ()) {
			Chrono::Duration time = Chrono::steady_clock ();
			if (time - block->second.last_write_time >= timeout) {
				Cache::iterator first_block = block;
				BlockIdx idx = first_block->first;
				BlockIdx max_idx = idx + numeric_limits <Size>::max () / block_size_;
				uint8_t* buf = (uint8_t*)first_block->second.buffer;
				size_t dirty_begin = block->second.dirty_begin;
				size_t dirty_end;
				do {
					dirty_end = block->second.dirty_end;
					++idx;
					++block;
					buf += block_size_;
				} while (block != cache_.end ()
					&& block->second.dirty ()
					&& idx < max_idx && block->first == idx
					&& block->second.buffer == buf
					&& time - block->second.last_write_time >= timeout);

				dirty_begin *= base_block_size_;
				dirty_end *= base_block_size_;
				Pos pos = (Pos)first_block->first * (Pos)block_size_ + (Pos)dirty_begin;
				Size size = (Size)((Pos)(idx - 1) * (Pos)block_size_ + (Pos)dirty_end - pos);
				assert (pos + (Pos)size <= file_size_);
				Request* request = new Request (Request::OP_WRITE, pos,
					(uint8_t*)first_block->second.buffer + dirty_begin, size, first_block);

				do {
					first_block->second.request = request;
					clear_dirty (first_block);
				} while (++first_block != block);

				issue_request (*request);
				continue;
			}
		}
		++block;
	}
}

void FileAccessDirect::flush ()
{
	write_dirty_blocks (0);
	for (Cache::iterator p = cache_.begin (); p != cache_.end (); ++p) {
		if (p->second.request && p->second.request->operation () == Request::OP_WRITE) {
			complete_request (*p->second.request);
		}
	}
	for (Cache::iterator p = cache_.begin (); p != cache_.end (); ++p) {
		if (p->second.dirty () && p->second.error)
			throw RuntimeError (p->second.error);
	}
}

void FileAccessDirect::complete_request (Request& request) NIRVANA_NOEXCEPT
{
	IO_Result result = request.get ();
	Cache::iterator block = request.first_block ();
	for (;;) {
		block->second.request = nullptr;
		if (result.size >= block_size_) {
			block->second.error = 0;
			result.size -= block_size_;
		} else if ((block->second.error = result.error) && request.operation () == Request::OP_WRITE) {
			set_dirty (block, result.size, block_size_ - result.size);
		}
		if ((cache_.end () == ++block) || block->second.request != &request)
			break;
	}
	delete& request;
}

void FileAccessDirect::complete_request (Cache::iterator it, unsigned wait_mask)
{
	if (it->second.request && (it->second.request->signalled ()
		|| (it->second.request->operation () & wait_mask))) {
		lock (it);
		complete_request (*it->second.request);
		unlock (it);
	}
	if (it->second.error)
		throw RuntimeError (it->second.error);
}

FileAccessDirect::Cache::iterator FileAccessDirect::release_cache (Cache::iterator it, Chrono::Duration time)
{
	if (it->second.request && it->second.request->signalled ())
		complete_request (*it->second.request);
	if (!it->second.lock_cnt && !it->second.dirty ()
		&& (
			(Pos)it->first * (Pos)block_size_ >= file_size_
			|| (!it->second.error && time - it->second.last_read_time >= DISCARD_TIMEOUT)))
	{
		Port::Memory::release (it->second.buffer, block_size_);
		return cache_.erase (it);
	} else
		return ++it;
}

void FileAccessDirect::clear_cache (BlockIdx excl_begin, BlockIdx excl_end)
{
	Chrono::Duration time = Chrono::steady_clock ();
	for (Cache::iterator p = cache_.begin (); p != cache_.end ();) {
		if (p->first < excl_begin)
			p = release_cache (p, time);
		else
			break;
	}
	for (Cache::iterator p = cache_.end (); p != cache_.begin ();) {
		--p;
		if (p->first >= excl_end)
			p = release_cache (p, time);
		else
			break;
	}
}

}
}
