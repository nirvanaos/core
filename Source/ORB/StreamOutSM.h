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

class StreamOutSM : public StreamOut
{
	struct Block
	{
		void* ptr;
		size_t size;

		Block () :
			ptr (nullptr),
			size (0)
		{}
	};

	struct OtherAllocated
	{
		OtherMemory::Pointer ptr;
		size_t size;
	};

public:
	StreamOutSM (OtherMemory& mem) :
		other_memory_ (&mem)
	{
		mem.get_sizes (sizes_);
		allocate_block (sizes_.block_size);
		const Block& block = cur_block ();
		void* p = block.ptr;

		// Stream header
		p = other_memory_->store_pointer (p, 0); // next block
		p = other_memory_->store_size (p, block.size); // block size
		segments_tail_ = p;
		p = other_memory_->store_pointer (p, 0); // segments
		cur_ptr_ = p;
	}

	~StreamOutSM ()
	{
		for (const auto& a : other_allocated_) {
			other_memory_->release (a.ptr, a.size);
		}
		for (const auto& a : blocks_) {
			Nirvana::Core::Port::Memory::release (a.ptr, a.size);
		}
	}

	virtual void write (size_t align, size_t size, const void* buf, size_t allocated_size);

private:
	void allocate_block (size_t cb);
	
	const Block& cur_block () const
	{
		return blocks_.back ();
	}

private:
	Nirvana::Core::CoreRef <OtherMemory> other_memory_;
	OtherMemory::Sizes sizes_;
	std::vector <Block, Nirvana::Core::UserAllocator <Block> > blocks_;
	std::vector <OtherAllocated> other_allocated_;
	void* cur_ptr_;
	void* segments_tail_;
};

}
}
}

#endif
