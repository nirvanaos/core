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

namespace CORBA {
namespace Internal {
namespace Core {

class StreamInSM : public StreamIn
{
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

public:
	StreamInSM (void* mem)
	{
		StreamHdr* hdr = (StreamHdr*)mem;
		cur_block_ = hdr;
		cur_ptr_ = (const Octet*)(hdr + 1);
		segments_ = hdr->segments;
	}

	~StreamInSM ();

	virtual void read (size_t align, size_t size, void* buf);
	virtual void* read (size_t align, size_t& size);

private:
	BlockHdr* cur_block_;
	const Octet* cur_ptr_;
	Segment* segments_;
};

}
}
}

#endif
