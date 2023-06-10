#include "FileAccessDirect.h"
#include <Port/Memory.h>

namespace Nirvana {
namespace Core {

FileAccessDirect::CacheRange FileAccessDirect::request_read (BlockIdx begin_block, BlockIdx end_block)
{
	CacheRange blocks (cache_);
	try {
		for (Cache::iterator cached_block = cache_.lower_bound (begin_block);;) {
			BlockIdx not_cached_end;
			if (cached_block == cache_.end ())
				not_cached_end = end_block;
			else
				not_cached_end = std::min (end_block, cached_block->first);
			if (begin_block < not_cached_end) {
				// Issue new read request
				size_t cb = (Size)(not_cached_end - begin_block) * block_size_;
				void* buffer = Port::Memory::allocate (nullptr, cb, 0);
				Ref <Request> request;
				unsigned new_blocks = 0;
				try {
					for (uint8_t* block_buf = (uint8_t*)buffer;;) {
						Cache::iterator it = cache_.emplace_hint (cached_block, begin_block, block_buf);
						blocks.append (it);
						++new_blocks;
						if (!request)
							request = Ref <Request>::create <ImplDynamic <Request>> (IO_Request::OP_READ, (Pos)begin_block * (Pos)block_size_, buffer, (Size)cb, it);
						it->second.request = request;
						if (not_cached_end == ++begin_block)
							break;
						block_buf += block_size_;
					}
				} catch (...) {
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
			unlock (*blocks.begin);
			++blocks.begin;
		}
		throw;
	}

	return blocks;
}

void FileAccessDirect::set_dirty (Cache::reference entry, size_t offset, size_t size) noexcept
{
	assert (size > 0 && size <= block_size_);
	if (!entry.second.dirty ())
		++dirty_blocks_;
	size_t block = offset / base_block_size_;
	if (entry.second.dirty_begin > block)
		entry.second.dirty_begin = (uint8_t)block;
	block = (offset + size + base_block_size_ - 1) / base_block_size_;
	if (entry.second.dirty_end < block)
		entry.second.dirty_end = (uint8_t)block;
}

void FileAccessDirect::write_dirty_blocks (SteadyTime timeout)
{
	for (Cache::iterator block = cache_.begin (); dirty_blocks_ && block != cache_.end (); ) {
		if (block->second.dirty ()) {
			SteadyTime time = Chrono::steady_clock ();
			if (time - block->second.last_write_time >= timeout) {
				Cache::iterator first_block = block;
				BlockIdx idx = first_block->first;
				BlockIdx max_idx = idx + std::numeric_limits <Size>::max () / block_size_;
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
				Ref <Request> request = Ref <Request>::create <ImplDynamic <Request>> (Request::OP_WRITE, pos,
					(uint8_t*)first_block->second.buffer + dirty_begin, size, first_block);

				do {
					first_block->second.request = request;
					clear_dirty (*first_block);
				} while (++first_block != block);

				issue_request (*request);
				continue;
			}
		}
		++block;
	}
}

void FileAccessDirect::complete_request (Ref <Request> request) noexcept
{
	request->wait ();
	if (request->first_block_ != cache_.end ()) {
		IO_Result result = request->result ();
		Cache::iterator block = request->first_block_;
		for (;;) {
			block->second.request = nullptr;
			if (result.size >= block_size_) {
				block->second.error = 0;
				result.size -= block_size_;
			} else if ((block->second.error = result.error) && request->operation () == Request::OP_WRITE) {
				set_dirty (*block, result.size, block_size_ - result.size);
			}
			if ((cache_.end () == ++block) || block->second.request != request)
				break;
		}
		request->first_block_ = cache_.end ();
	}
}

void FileAccessDirect::complete_request (Cache::reference entry, int op)
{
	while (entry.second.request && (entry.second.request->signalled ()
		|| (!op || entry.second.request->operation () == op)))
	{
		lock (entry);
		complete_request (entry.second.request);
		unlock (entry);
	}
	if (entry.second.error)
		throw RuntimeError (entry.second.error);
}

void FileAccessDirect::complete_size_request () noexcept
{
	size_request_->wait ();
	if (size_request_) {
		if (!size_request_->result ().error)
			file_size_ = size_request_->offset ();
		size_request_ = nullptr;
	}
}

FileAccessDirect::Cache::iterator FileAccessDirect::release_cache (Cache::iterator it, SteadyTime time)
{
	if (it->second.request && it->second.request->signalled ())
		complete_request (it->second.request);
	if (!it->second.lock_cnt && !it->second.dirty ()
		&& (
			(Pos)it->first * (Pos)block_size_ >= file_size_
			|| (!it->second.error && Port::Memory::is_private (it->second.buffer, block_size_)
				&& it->second.last_read_time <= time)))
	{
		Port::Memory::release (it->second.buffer, block_size_);
		return cache_.erase (it);
	} else
		return ++it;
}

void FileAccessDirect::clear_cache (BlockIdx excl_begin, BlockIdx excl_end)
{
	SteadyTime time = Chrono::steady_clock ();
	if (time < discard_timeout_)
		return;
	time -= discard_timeout_;
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

void FileAccessDirect::set_size (Pos new_size)
{
	while (size_request_)
		complete_size_request ();

	Ref <Request> request = Ref <Request>::create <ImplDynamic <Request>>
		(Request::OP_SET_SIZE, new_size, nullptr, 0, cache_.end ());
	size_request_ = request;
	issue_request (*request);
	complete_size_request ();
	int err = request->result ().error;
	if (err)
		throw RuntimeError (err);
}

}
}
