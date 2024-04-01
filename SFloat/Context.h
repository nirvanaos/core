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
#ifndef CORBA_INTERNAL_SFLOAT_CONTEXT_H_
#define CORBA_INTERNAL_SFLOAT_CONTEXT_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/System.h>

namespace CORBA {
namespace Internal {

class SFloatContext
{
public:
	SFloatContext ();
	~SFloatContext ();

	uint_fast8_t exception_flags ()
	{
		return exception_flags_;
	}

private:
	uint_fast8_t exception_flags_;
};

class SFloatTLS
{
public:
	SFloatTLS ();
	~SFloatTLS ();

	Nirvana::System::CS_Key cs_key () const noexcept
	{
		return cs_key_;
	}

private:
	Nirvana::System::CS_Key cs_key_;
};

}
}

#endif
