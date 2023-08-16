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
#include "DomainProt.h"
#include "../UserAllocator.h"
#include "../UserObject.h"
#include <vector>
#include <memory>

namespace CORBA {
namespace Core {

/// \brief Shared memory output stream.
class NIRVANA_NOVTABLE StreamOutSM :
	public StreamOut,
	public Nirvana::Core::UserObject
{
public:
	virtual void write (size_t align, size_t element_size, size_t CDR_element_size,
		size_t element_count, void* data, size_t& allocated_size) override;
	virtual size_t size () const override;
	virtual void* header (size_t hdr_size) override;
	virtual void rewind (size_t hdr_size) override;
	virtual void chunk_begin () override;
	virtual bool chunk_end () override;
	virtual Long chunk_size () const override;

	void store_stream (ESIOP::SharedMemPtr& where);

	void detach () noexcept
	{
		blocks_.clear ();
		other_allocated_.clear ();
	}

	ESIOP::OtherDomain& other_domain () const noexcept
	{
		return *other_domain_;
	}

protected:
	StreamOutSM (DomainProt& target, bool client) :
		other_domain_ (&target)
	{
		if (client)
			initialize ();
	}

	void initialize ();

	~StreamOutSM ()
	{
		clear ();
	}

	void clear (size_t leave_header = 0) noexcept;

private:
	struct Block
	{
		ESIOP::SharedMemPtr other_ptr;
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
		ESIOP::SharedMemPtr ptr;
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

	size_t stream_hdr_size () const noexcept;

private:
	servant_reference <DomainProt> other_domain_;
	ESIOP::PlatformSizes sizes_;
	ESIOP::SharedMemPtr stream_hdr_;
	std::vector <Block, Nirvana::Core::UserAllocator <Block> > blocks_;
	std::vector <OtherAllocated, Nirvana::Core::UserAllocator <OtherAllocated> > other_allocated_;
	uint8_t* cur_ptr_;
	void* segments_tail_;
	size_t size_;
	size_t chunk_begin_;
	int32_t* chunk_;
	size_t commit_unit_;
};

}
}

namespace ESIOP {

inline
Request::Request (ProtDomainId client, CORBA::Core::StreamOutSM& stream, uint32_t rq_id) :
	MessageHeader (REQUEST),
	client_domain (client),
	request_id (rq_id)
{
	stream.store_stream (GIOP_message);
}

inline
Reply::Reply (CORBA::Core::StreamOutSM& stream, uint32_t rq_id) :
	MessageHeader (REPLY),
	request_id (rq_id)
{
	stream.store_stream (GIOP_message);
}

}

#endif
