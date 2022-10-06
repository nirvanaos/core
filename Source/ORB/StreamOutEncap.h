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
#ifndef NIRVANA_ORB_CORE_STREAMOUTENCAP_H_
#define NIRVANA_ORB_CORE_STREAMOUTENCAP_H_
#pragma once

#include "StreamOut.h"

namespace CORBA {
namespace Core {

/// Output stream for data encapsulated as octet sequence.
class NIRVANA_NOVTABLE StreamOutEncap
	: public StreamOut
{
public:
	StreamOutEncap () :
		chunk_begin_ (0)
	{}

	virtual void write (size_t align, size_t size, void* data, size_t& allocated_size) override;
	virtual size_t size () const override;
	virtual void* header (size_t hdr_size) override;
	virtual void rewind (size_t hdr_size) override;
	virtual void chunk_begin () override;
	virtual bool chunk_end () override;
	virtual Long chunk_size () const override;

	OctetSeq& data () NIRVANA_NOEXCEPT
	{
		return buffer_;
	}

private:
	OctetSeq buffer_;
	size_t chunk_begin_;
};

}
}

#endif
