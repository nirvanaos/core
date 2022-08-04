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
#ifndef NIRVANA_ORB_CORE_STREAMIN_H_
#define NIRVANA_ORB_CORE_STREAMIN_H_
#pragma once

#include "../CoreInterface.h"

namespace CORBA {
namespace Core {

class NIRVANA_NOVTABLE StreamIn
{
	DECLARE_CORE_INTERFACE

public:
	///@{
	/// MARSHAL minor codes
	/// See CORBA 3.0: Chapter 15, General Inter-ORB Protocol
	/// 9.1.4 GIOP Message Header

	/// A MARSHAL exception with minor code 9 indicates that fewer bytes were present in a message
	/// than indicated by	the count. This condition can arise if the sender sends a message
	/// in fragments, and the receiver detects that the final fragment was received but contained
	/// insufficient data for all parameters to be unmarshaled.
	static const uint32_t MARSHAL_MINOR_FEWER = MAKE_OMG_MINOR (9);

	/// A MARSHAL exception with minor code 8 indicates that more bytes were present in a message than
	/// indicated by the count. Depending on the ORB implementation, this condition may be reported
	/// for the current message or the next message that is processed (when the receiver detects that
	/// the previous message is not immediately followed by the GIOP magic number).
	static const uint32_t MARSHAL_MINOR_MORE = MAKE_OMG_MINOR (8);

	///@}

	/// Read data into user-allocated buffer.
	/// 
	/// \param align Data alignment
	/// \param size  Data size.
	/// \param data Pointer to the data buffer.
	virtual void read (size_t align, size_t size, void* data) = 0;

	/// Allocate buffer and read.
	/// 
	/// \param align Data alignment
	/// \param [in,out] size Data size on input. Allocated size on return.
	/// \returns Allocated data buffer.
	virtual void* read (size_t align, size_t& size) = 0;

	/// Set the remaining data size.
	/// 
	/// Initially, the data size is set to numeric_limits<size_t>::max().
	/// Then it may be reduced to the real data size.
	/// 
	/// \param size The remaining data size in bytes.
	///   This count includes any alignment gaps and must match the size of the actual request
	///   parameters (plus any final padding bytes that may follow the parameters to have a fragment
	///   message terminate on an 8 - byte boundary).
	virtual void set_size (size_t size) = 0;

	/// Check for stream end.
	/// 
	/// \returns `true` if no more data in stream.
	virtual bool end () const = 0;
};

}
}

#endif
