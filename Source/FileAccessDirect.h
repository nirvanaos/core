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

#include <Nirvana/Nirvana.h>
#include <Port/FileAccess.h>
#include "UserObject.h"
#include "Chrono.h"
#include <map>
#include <File_s.h>

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

	// Write timeout currently defined as constant
	static const Chrono::Duration WRITE_TIMEOUT = 500 * System::MILLISECOND;
public:
	FileAccessDirect (const std::string& path, int flags) :
		Base (path, flags),
		base_block_size_ (Base::block_size ()),
		block_size_ (std::max (base_block_size_, (Size)Port::Memory::SHARING_ASSOCIATIVITY))
	{
		if (block_size_ / base_block_size_ > 128)
			throw RuntimeError (ENOTSUP);
		file_size_ = file_size ();
	}

	/// Destructor. Without the flush() call all dirty entries will be lost!
	~FileAccessDirect ()
	{
		for (Cache::iterator it = cache_.begin (); it != cache_.end ();) {
			if (it->second.request)
				complete_request (*it->second.request);
			it = release_cache (it);
		}
	}

	inline
	void flush ();

	uint64_t file_size () const
	{
		return file_size_;
	}

	void file_size (uint64_t new_size)
	{
		Base::file_size ((Pos)new_size);
		file_size_ = (Pos)new_size;
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
		void* buffer;
		Request* request;
		unsigned lock_cnt;
		short error;
		// Dirty base blocks
		uint8_t dirty_begin, dirty_end;

		CacheEntry (void* buf) NIRVANA_NOEXCEPT :
		last_write_time (0),
			buffer (buf),
			request (nullptr),
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

		Cache::iterator first_block () const
		{
			return first_block_;
		}

	private:
		Cache::iterator first_block_;
	};

	void complete_request (Request& request) NIRVANA_NOEXCEPT;
	void complete_request (Cache::iterator it, unsigned wait_mask = ~0);

	bool can_be_released (Cache::iterator it);
	Cache::iterator release_cache (Cache::iterator it);

	void clear_cache (BlockIdx excl_begin, BlockIdx excl_end);

	CacheRange request_read (BlockIdx begin, BlockIdx end);

	void set_dirty (Cache::iterator it, const Chrono::Duration& time,
		size_t offset, size_t size) NIRVANA_NOEXCEPT
	{
		it->second.last_write_time = time;
		set_dirty (it, offset, size);
	}

	void set_dirty (Cache::iterator it, size_t offset, size_t size) NIRVANA_NOEXCEPT;

	void clear_dirty (Cache::iterator it) NIRVANA_NOEXCEPT {
		if (it->second.dirty ()) {
			assert (dirty_blocks_);
			it->second.dirty_begin = it->second.dirty_end = 0;
			--dirty_blocks_;
		}
	}

	void write_dirty_blocks (Chrono::Duration timeout);

	void unlock (Cache::iterator it) NIRVANA_NOEXCEPT {
		assert (it->second.lock_cnt);
		--(it->second.lock_cnt);
	}

private:
	const Size base_block_size_;
	const Size block_size_;
	Pos file_size_;
	Cache cache_;
	size_t dirty_blocks_;
};

}
}

#endif
