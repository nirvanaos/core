/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#ifndef NIRVANA_CORE_FILEACCESSDIRECT_H_
#define NIRVANA_CORE_FILEACCESSDIRECT_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Port/FileAccess.h>
#include "UserObject.h"
#include "Chrono.h"
#include <map>
#include "IDL/File_s.h"

namespace Nirvana {
namespace Core {

class FileAccessDirect :
	public CORBA::servant_traits <Nirvana::FileAccessDirect>::Servant <FileAccessDirect>,
	private Port::FileAccessDirect
{
	typedef Port::FileAccessDirect Base;
	typedef Base::Pos Pos;
	typedef Base::Size Size;
	typedef Base::BlockIdx BlockIdx;

	// Timeouts currently defined as a constant
	static const Chrono::Duration WRITE_TIMEOUT = 500 * System::MILLISECOND;
	static const Chrono::Duration DISCARD_TIMEOUT = 500 * System::MILLISECOND;
public:
	FileAccessDirect (const std::string& path, int flags) :
		Base (path, flags, file_size_, base_block_size_),
		block_size_ (std::max (base_block_size_, (Size)Port::Memory::SHARING_ASSOCIATIVITY)),
		dirty_blocks_ (0)
	{
		if (block_size_ / base_block_size_ > 128)
			throw RuntimeError (ENOTSUP);
	}

	/// Destructor. Without the flush() call all dirty entries will be lost!
	~FileAccessDirect ()
	{
		for (Cache::iterator it = cache_.begin (); it != cache_.end (); ++it) {
			if (it->second.request)
				complete_request (it->second.request);
			Port::Memory::release (it->second.buffer, block_size_);
		}
	}


	inline
	void flush ();

	uint64_t size () const
	{
		return file_size_;
	}

	void size (uint64_t new_size)
	{
		if (file_size_ == new_size)
			return;

		if (file_size_ > new_size) {
			// Truncation
			file_size_ = new_size;
			for (Cache::iterator it = cache_.end (); it != cache_.begin ();) {
				--it;
				if (it->first < new_size)
					break;
				clear_dirty (*it);
			}
			for (Cache::iterator it = cache_.end (); it != cache_.begin ();) {
				--it;
				if (it->first < new_size)
					break;
				if (it->second.request) {
					complete_request (it->second.request);
					if (new_size != (volatile Pos&)file_size_)
						return;
				}
				if (!it->second.lock_cnt)
					it = cache_.erase (it);
			}
		}
		set_size (new_size);
	}

	inline
	void read (uint64_t pos, uint32_t size, std::vector <uint8_t>& data);

	inline
	void write (uint64_t pos, const std::vector <uint8_t>& data);

private:
	class Request;

	struct CacheEntry
	{
		Chrono::Duration last_write_time; // Valid only if dirty_begin != dirty_end
		Chrono::Duration last_read_time;
		void* buffer;
		CoreRef <Request> request;
		unsigned lock_cnt;
		short error;
		// Dirty base blocks
		uint8_t dirty_begin, dirty_end;

		CacheEntry (void* buf) NIRVANA_NOEXCEPT :
			last_write_time (0),
			last_read_time (0),
			buffer (buf),
			lock_cnt (1),
			error (0),
			dirty_begin (0),
			dirty_end (0)
		{}

		bool dirty () const NIRVANA_NOEXCEPT
		{
			return dirty_begin | dirty_end;
		}
	};

	// We can not use `phmap::btree_map` here because we need the iterator stability.
	typedef std::map <BlockIdx, CacheEntry, std::less <BlockIdx>, UserAllocator <std::pair <BlockIdx, CacheEntry>>> Cache;

	struct CacheRange
	{
		Cache::iterator begin, end;

		CacheRange (Cache& cache) :
			begin (cache.end ()),
			end (cache.end ())
		{}

		void append (Cache::iterator it)
		{
			if (begin == end)
				begin = it;
			end = it;
			++end;
		}
	};

	typedef Base::Request RequestBase;
	class Request :
		public RequestBase,
		public UserObject
	{
		typedef RequestBase Base;
	public:
		Request (Operation op, Pos offset, void* buf, Size size, Cache::iterator first_block) :
			Base (op, offset, buf, size),
			first_block_ (first_block)
		{}

		~Request ()
		{}

		Cache::iterator first_block_;
	};

	void complete_request (CoreRef <Request> request) NIRVANA_NOEXCEPT;
	void complete_request (Cache::reference entry, int op = 0);

	Cache::iterator release_cache (Cache::iterator it, Chrono::Duration time);

	void clear_cache (BlockIdx excl_begin, BlockIdx excl_end);

	CacheRange request_read (BlockIdx begin, BlockIdx end);

	void set_dirty (Cache::reference entry, const Chrono::Duration& time,
		size_t offset, size_t size) NIRVANA_NOEXCEPT
	{
		entry.second.last_write_time = time;
		set_dirty (entry, offset, size);
	}

	void set_dirty (Cache::reference entry, size_t offset, size_t size) NIRVANA_NOEXCEPT;

	void clear_dirty (Cache::reference entry) NIRVANA_NOEXCEPT {
		if (entry.second.dirty ()) {
			assert (dirty_blocks_);
			entry.second.dirty_begin = entry.second.dirty_end = 0;
			--dirty_blocks_;
		}
	}

	void write_dirty_blocks (Chrono::Duration timeout);

	static void lock (Cache::reference entry) NIRVANA_NOEXCEPT {
		++(entry.second.lock_cnt);
	}

	static void unlock (Cache::reference entry) NIRVANA_NOEXCEPT {
		assert (entry.second.lock_cnt);
		--(entry.second.lock_cnt);
	}

	void set_size (Pos new_size);

	void complete_size_request () NIRVANA_NOEXCEPT;

private:
	Pos file_size_;
	Cache cache_;
	CoreRef <Request> size_request_;
	const Size block_size_;
	Size base_block_size_;
	size_t dirty_blocks_;
};

void FileAccessDirect::read (uint64_t pos, uint32_t size, std::vector <uint8_t>& data)
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
			complete_request (*block, Request::OP_READ);
			// Reserve space when read across multiple blocks
			Cache::iterator second_block = block;
			if (blocks.end != ++second_block)
				data.reserve (size);
			Size off = pos % block_size_;
			const uint8_t* bl = (uint8_t*)block->second.buffer + off;
			Size cb = std::min (size, block_size_ - off);
			data.insert (data.end (), bl, bl + cb);
			size -= cb;
			block = second_block;
		}
		// Copy remaining blocks
		while (block != blocks.end) {
			complete_request (*block, Request::OP_READ);
			const uint8_t* bl = (uint8_t*)block->second.buffer;
			Size cb = std::min (size, block_size_);
			data.insert (data.end (), bl, bl + cb);
			size -= cb;
			++block;
		}
		// Set read time and unlock blocks
		Chrono::Duration time = Chrono::steady_clock ();
		while (blocks.begin != blocks.end) {
			blocks.begin->second.last_read_time = time;
			unlock (*blocks.begin);
			++blocks.begin;
		}

	} catch (...) {
		while (blocks.begin != blocks.end) {
			unlock (*blocks.begin);
			++blocks.begin;
		}
		throw;
	}

	write_dirty_blocks (WRITE_TIMEOUT);
}

void FileAccessDirect::write (uint64_t pos, const std::vector <uint8_t>& data)
{
	if (pos == std::numeric_limits <uint64_t>::max ())
		pos = file_size_;
	Pos end = (Pos)pos + data.size ();
	BlockIdx cur_block = pos / block_size_;
	BlockIdx end_block = (end + block_size_ - 1) / block_size_;

	clear_cache (cur_block, end_block);

	if (data.empty ())
		return;

	struct ReadRange
	{
		BlockIdx start;
		unsigned count;
	} read_ranges [2] = { {0}, {0} }; // Head, tail

	if (file_size_ > pos) {

		if (pos % block_size_) {
			// We need to read the first block before writing to it.
			read_ranges [0].start = cur_block;
			read_ranges [0].count = 1;
		}

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
				not_cached_end = std::min (end_block, cached_block->first);
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
						cb_copy = std::min (cb - block_offset, src_size);
						Port::Memory::copy (buffer + block_offset, const_cast <uint8_t*> (src_data), cb_copy, 0);
					}
					src_data += cb_copy;
					src_size -= cb_copy;
					Pos end = (Pos)cur_block * (Pos)block_size_ + cb_copy;
					Chrono::Duration time = Chrono::steady_clock ();
					for (uint8_t* block_buf = buffer;;) {
						Cache::iterator it = cache_.emplace_hint (cached_block, cur_block, block_buf);
						size_t dirty_size = std::min ((size_t)block_size_ - block_offset, cb_copy);
						set_dirty (*it, time, block_offset, dirty_size);
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
			complete_request (*cached_block); // Complete previous read/write operation
			size_t cb_copy = std::max (block_size_ - block_offset, src_size);
			Port::Memory::copy ((uint8_t*)cached_block->second.buffer + block_offset, const_cast <uint8_t*> (src_data), cb_copy, 0);
			set_dirty (*cached_block, Chrono::steady_clock (), block_offset, cb_copy);

			// Update file size
			Pos end = (Pos)cached_block->first * (Pos)block_size_ + block_offset + cb_copy;
			if (file_size_ < end)
				file_size_ = end;

			// If this block is readed block, unlock it
			for (auto p = read_blocks; p != std::end (read_blocks); ++p) {
				Cache::iterator block = *p;
				if (block == cached_block) {
					unlock (*block);
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
				unlock (*block);
		}
		throw;
	}

	write_dirty_blocks (WRITE_TIMEOUT);
}

void FileAccessDirect::flush ()
{
	write_dirty_blocks (0);
	
	for (Cache::iterator it = cache_.begin (); it != cache_.end (); ++it) {
		// Complete pending write requests.
		// Do not throw the exceptions.
		if (it->second.request && it->second.request->operation () == Request::OP_WRITE)
			complete_request (it->second.request);
	}
	
	// Set file size if unaligned
	if (file_size_ % base_block_size_)
		set_size (file_size_);

	// Check for write failures
	for (Cache::iterator it = cache_.begin (); it != cache_.end (); ++it) {
		if (it->second.dirty () && it->second.error)
			throw RuntimeError (it->second.error);
	}
}

}
}

#endif
