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
#ifndef NIRVANA_ORB_CORE_STREAMOUTSM_H_
#define NIRVANA_ORB_CORE_STREAMOUTSM_H_
#pragma once

#include "StreamOut.h"
#include "OtherMemory.h"
#include "../UserAllocator.h"
#include <vector>

namespace CORBA {
namespace Internal {
namespace Core {

class NIRVANA_NOVTABLE StreamOutSM : public StreamOut
{
	struct Block
	{
		OtherMemory::Pointer other_ptr;
		void* ptr;
		size_t size;

		Block () :
			other_ptr (0),
			ptr (nullptr),
			size (0)
		{}
	};

	struct OtherAllocated
	{
		OtherMemory::Pointer ptr;
		size_t size;

		OtherAllocated () :
			ptr (0),
			size (0)
		{}
	};

public:
	StreamOutSM (OtherMemory& mem) :
		other_memory_ (&mem)
	{
		mem.get_sizes (sizes_);
		allocate_block (sizes_.sizeof_pointer, sizes_.sizeof_pointer);
		stream_hdr_ = cur_block ().other_ptr;
		segments_tail_ = cur_ptr_;
		cur_ptr_ = (Octet*)other_memory_->store_pointer (segments_tail_, 0); // segments
	}

	~StreamOutSM ()
	{
		for (const auto& a : other_allocated_) {
			other_memory_->release (a.ptr, a.size);
		}
		for (const auto& a : blocks_) {
			if (a.other_ptr)
				other_memory_->release (a.other_ptr, a.size);
			if (a.ptr)
				Nirvana::Core::Port::Memory::release (a.ptr, a.size);
		}
	}

	virtual void write (size_t align, size_t size, void* data, size_t* allocated_size);

	void* store_stream_ptr (void* where) const
	{
		return other_memory_->store_pointer (where, stream_hdr_);
	}

	void detach_data ()
	{
		blocks_.clear ();
		other_allocated_.clear ();
	}

private:
	void allocate_block (size_t align, size_t size);
	
	const Block& cur_block () const
	{
		return blocks_.back ();
	}

	void purge ();

private:
	Nirvana::Core::CoreRef <OtherMemory> other_memory_;
	OtherMemory::Sizes sizes_;
	OtherMemory::Pointer stream_hdr_;
	std::vector <Block, Nirvana::Core::UserAllocator <Block> > blocks_;
	std::vector <OtherAllocated, Nirvana::Core::UserAllocator <OtherAllocated> > other_allocated_;
	Octet* cur_ptr_;
	void* segments_tail_;
};

}
}
}

#endif
