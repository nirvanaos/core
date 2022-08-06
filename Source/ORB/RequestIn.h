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
#ifndef NIRVANA_ORB_CORE_REQUESTIN_H_
#define NIRVANA_ORB_CORE_REQUESTIN_H_
#pragma once

#include "Request.h"
#include "../CoreObject.h"

namespace CORBA {
namespace Core {

/// Implements server-side IORequest for GIOP.
class NIRVANA_NOVTABLE RequestIn :
	public Request,
	public Nirvana::Core::CoreObject // Must be created quickly
{
protected:
	RequestIn (StreamIn& stream, CodeSetConverterW& cscw) :
		Request (&stream, nullptr, cscw)
	{}

private:
	virtual void unmarshal_end () override;
	virtual void marshal_op () override;
	virtual void success () override;
	virtual void cancel () override;

	void switch_to_reply ();
};

/// Implements server-side IORequest for GIOP 1.0.
class NIRVANA_NOVTABLE RequestIn_1_0 : public RequestIn
{
public:
	const GIOP::RequestHeader_1_0& header () const NIRVANA_NOEXCEPT
	{
		return header_;
	}

protected:
	RequestIn_1_0 (StreamIn& stream) :
		RequestIn (stream, *CodeSetConverterW_1_0::get_default (false))
	{
		Internal::Type <GIOP::RequestHeader_1_0>::unmarshal (_get_ptr (), header_);
	}

private:
	GIOP::RequestHeader_1_0 header_;
};

}
}

#endif
