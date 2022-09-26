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
#ifndef NIRVANA_ORB_CORE_REQUESTENCAPOUT_H_
#define NIRVANA_ORB_CORE_REQUESTENCAPOUT_H_
#pragma once

#include "RequestEncap.h"
#include "StreamOutEncap.h"

namespace CORBA {
namespace Core {

/// Implements IORequest for encapsulated data.
class NIRVANA_NOVTABLE RequestEncapOut : public RequestEncap
{
public:
	OctetSeq& data () NIRVANA_NOEXCEPT
	{
		return stream_.data ();
	}

protected:
	RequestEncapOut (const RequestGIOP& parent) NIRVANA_NOEXCEPT :
		RequestEncap (parent)
	{
		stream_out_ = &stream_;
	}

	virtual bool marshal_op () override
	{
		return true;
	}

private:
	Nirvana::Core::ImplStatic <StreamOutEncap> stream_;
};

}
}

#endif
