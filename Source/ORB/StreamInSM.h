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
#ifndef NIRVANA_ORB_CORE_STREAMINSM_H_
#define NIRVANA_ORB_CORE_STREAMINSM_H_
#pragma once

#include "StreamIn.h"
#include "../CoreObject.h"

namespace Nirvana {
namespace ESIOP {

/// Shared memory input stream
/// 
/// This object is created from the postman context.
/// So it should be CoreObject for quick creation.
/// 
class NIRVANA_NOVTABLE StreamInSM :
	public CORBA::Core::StreamIn,
	public Nirvana::Core::CoreObject
{
public:
	virtual void read (size_t align, size_t size, void* buf) override;
	virtual void* read (size_t align, size_t& size) override;
	virtual void set_size (size_t size) override;
	virtual size_t end () override;

protected:
	StreamInSM (void* mem) :
		cur_block_ ((StreamHdr*)mem),
		cur_ptr_ ((const uint8_t*)((StreamHdr*)mem + 1)),
		segments_ (((StreamHdr*)mem)->segments),
		size_ (std::numeric_limits <size_t>::max ()),
		position_ (0)
	{}

	~StreamInSM ();

private:
	struct BlockHdr
	{
		BlockHdr* next;
		size_t size;
	};

	struct Segment
	{
		Segment* next;
		void* pointer;
		size_t allocated_size;
	};

	struct StreamHdr : BlockHdr
	{
		Segment* segments;
	};

	void next_block ();
	const Segment* get_segment (size_t align, size_t size);
	void physical_read (size_t align, size_t size, void* buf);
	void check_position (size_t align, size_t size) const;
	void set_position (size_t align, size_t size) NIRVANA_NOEXCEPT;

private:
	BlockHdr* cur_block_;
	const uint8_t* cur_ptr_;
	Segment* segments_;
	size_t size_;
	size_t position_; // virtual stream position
};

}
}

#endif
