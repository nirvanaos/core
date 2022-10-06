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
#include "RemoteReferences.h"
#include "../UserAllocator.h"
#include "../UserObject.h"
#include <vector>
#include <memory>

namespace ESIOP {

class NIRVANA_NOVTABLE StreamOutSM :
	public CORBA::Core::StreamOut,
	public Nirvana::Core::UserObject
{
public:
	virtual void write (size_t align, size_t size, void* data, size_t& allocated_size) override;
	virtual size_t size () const override;
	virtual void* header (size_t hdr_size) override;
	virtual void rewind (size_t hdr_size) override;
	virtual void chunk_begin () override;
	virtual bool chunk_end () override;
	virtual CORBA::Long chunk_size () const override;

	SharedMemPtr get_shared () NIRVANA_NOEXCEPT
	{
		// Truncate size of the last block
		uint8_t* block = (uint8_t*)cur_block ().ptr;
		other_domain_->store_size (block + sizes_.sizeof_pointer, cur_ptr_ - block);

		// Purge all blocks
		for (auto it = blocks_.begin (); it != blocks_.end (); ++it) {
			if (it->ptr) {
				other_domain_->copy (it->other_ptr, it->ptr, it->size, true);
				it->ptr = nullptr;
			}
		}
		return stream_hdr_;
	}

	void detach () NIRVANA_NOEXCEPT
	{
		blocks_.clear ();
		other_allocated_.clear ();
	}

protected:
	StreamOutSM (CORBA::Core::DomainLocal& target) :
		other_domain_ (&target)
	{
		initialize ();
	}

	StreamOutSM (ProtDomainId client_id) :
		other_domain_ (CORBA::Core::RemoteReferences::singleton ().get_domain (client_id))
	{} // initialize () can be called later

	void initialize ();

	~StreamOutSM ()
	{
		clear ();
	}

	void clear (size_t leave_header = 0) NIRVANA_NOEXCEPT;

	OtherDomain& other_domain () const NIRVANA_NOEXCEPT
	{
		return *other_domain_;
	}

private:
	struct Block
	{
		SharedMemPtr other_ptr;
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
		SharedMemPtr ptr;
		size_t size;

		OtherAllocated () :
			ptr (0),
			size (0)
		{}
	};

	void allocate_block (size_t align, size_t size);

	const Block& cur_block () const
	{
		return blocks_.back ();
	}

	void purge ();

	size_t stream_hdr_size () const NIRVANA_NOEXCEPT;

private:
	CORBA::servant_reference <CORBA::Core::DomainLocal> other_domain_;
	PlatformSizes sizes_;
	SharedMemPtr stream_hdr_;
	std::vector <Block, Nirvana::Core::UserAllocator <Block> > blocks_;
	std::vector <OtherAllocated, Nirvana::Core::UserAllocator <OtherAllocated> > other_allocated_;
	uint8_t* cur_ptr_;
	void* segments_tail_;
	size_t size_;
	size_t chunk_begin_;
	int32_t* chunk_;
};

}

#endif
