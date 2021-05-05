/// \file
/// Platform domain environment specific inter-ORB protocol.
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
#ifndef NIRVANA_ORB_CORE_MESSAGE_H_
#define NIRVANA_ORB_CORE_MESSAGE_H_

#include <CORBA/SystemException.h>
#include <Port/config.h>

namespace Nirvana {
namespace ESIOP {

struct Message
{
	enum class Type : uint16_t
	{
		REQUEST,
		REPLY,
		USER_EXCEPTION,
		SYSTEM_EXCEPTION,

		RESERVED_MESSAGES = 8
	};

	struct ObjRef
	{
		uintptr_t address;
		Core::ObjRefSignature signature;
	};

	struct ObjRefLocal : ObjRef
	{
		Core::ProtDomainId prot_domain;
	};

	struct Header
	{
		Type type;

		uint16_t flags;
		static const uint16_t IMMEDIATE_DATA = 0x0001;

		ObjRef target;
	};

	struct Request : Header
	{
		ObjRefLocal reply_target; // Zero for oneway requests
		Core::UserToken user;
	};

	struct SystemException : Header
	{
		CORBA::SystemException::Code code;
		CORBA::SystemException::_Data data;
	};

	union Buffer
	{
		Request request;
		SystemException system_exception;
	};
};

}
}

#endif
