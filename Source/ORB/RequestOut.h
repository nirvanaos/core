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
#ifndef NIRVANA_ORB_CORE_REQUESTOUT_H_
#define NIRVANA_ORB_CORE_REQUESTOUT_H_
#pragma once

#include "Request.h"
#include "../UserObject.h"

namespace CORBA {
namespace Core {

/// Implements client-side IORequest for GIOP.
class RequestOut :
	public Request,
	public Nirvana::Core::UserObject
{
public:
	RequestOut (StreamOut& stream, const GIOP::MessageHeader_1_3& hdr);

	void reply (StreamIn& data);

private:
	virtual void unmarshal_end () override;
	virtual void marshal_op () override;
	virtual void success () override;
	virtual void cancel () override;
};

}
}

#endif