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
#include "pch.h"
#include "BindError.h"

namespace Nirvana {
namespace BindError {

void set_message (Info& info, std::string msg)
{
	info.s (std::move (msg));
	info._d (Type::ERR_MESSAGE);
}

NIRVANA_NORETURN void throw_message (std::string msg)
{
	Nirvana::BindError::Error err;
	set_message (err.info (), std::move (msg));
	throw err;
}

NIRVANA_NORETURN void throw_invalid_metadata ()
{
	throw_message ("Invalid metadata");
}

void set_system (Error& err, const CORBA::SystemException& ex)
{
	CORBA::Any any;
	any <<= ex;
	err.info ().system_exception (std::move (any));
}

Info& push (Error& err)
{
	err.stack ().emplace_back ();
	return err.stack ().back ();
}

void push_obj_name (Error& err, std::string name)
{
	Info& info = push (err);
	info.s (std::move (name));
	info._d (Type::ERR_OBJ_NAME);
}

}
}
