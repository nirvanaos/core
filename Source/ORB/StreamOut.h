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
#ifndef NIRVANA_ORB_CORE_STREAMOUT_H_
#define NIRVANA_ORB_CORE_STREAMOUT_H_
#pragma once

#include "../CoreInterface.h"
#include "IDL/GIOP.h"

namespace CORBA {
namespace Core {

class NIRVANA_NOVTABLE StreamOut
{
	DECLARE_CORE_INTERFACE

public:
	/// Marshal data.
	/// 
	/// \param align Data alignment
	/// \param size Data size.
	/// \param data Data pointer.
	/// \param allocated_size If this parameter is not zero, the stream may adopt the memory block.
	///   When stream adopts the memory block, it sets \p allocated_size to 0.
	virtual void write (size_t align, size_t size, void* data, size_t& allocated_size) = 0;

	/// \returns The data size include any alignment gaps.
	virtual size_t size () const = 0;

	/// Get stream buffer header.
	/// 
	/// \param hdr_size The header size in bytes.
	///   This parameter used only for check.
	///   At least \p hdr_size contiguous bytes must be available for writing.
	/// \returns Pointer to the stream begin.
	virtual void* header (size_t hdr_size) = 0;

	/// Rewind stream to the header size.
	/// Stream must drop all data after \p hdr_size bytes
	/// and set write position after hdr_size.
	/// 
	/// \param hdr_size Header size in bytes.
	virtual void rewind (size_t hdr_size) = 0;

	/// Marshal data helper.
	/// 
	/// \param align Data alignment
	/// \param size Data size.
	/// \param data Data pointer.
	void write (size_t align, size_t size, const void* data)
	{
		size_t zero = 0;
		write (align, size, const_cast <void*> (data), zero);
	}

	/// Write GIOP message header.
	void write_message_header (GIOP::MsgType msg_type, unsigned GIOP_minor);
};

}
}

#endif
