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
#ifndef NIRVANA_ORB_CORE_OFFSET_PTR_H_
#define NIRVANA_ORB_CORE_OFFSET_PTR_H_
#pragma once

#include <CORBA/CORBA.h>
#include "../TLS.h"
#include "IDL/ObjectFactory.h"

namespace CORBA {
namespace Internal {
namespace Core {

inline
size_t offset_ptr () NIRVANA_NOEXCEPT
{
	ObjectFactory::StatelessCreationFrame* scf =
		(ObjectFactory::StatelessCreationFrame*)Nirvana::Core::TLS::current ()
		.get (Nirvana::Core::TLS::CORE_TLS_OBJECT_FACTORY);
	if (scf)
		return scf->offset ();
	return 0;
}

template <class I> inline
I_ptr <I> offset_ptr (I_ptr <I> p, size_t cb) NIRVANA_NOEXCEPT
{
	return reinterpret_cast <I*> ((Octet*)&p + cb);
}

template <class I> inline
I_ptr <I> offset_ptr (I_ptr <I> p) NIRVANA_NOEXCEPT
{
	return offset_ptr (p, offset_ptr ());
}

}
}
}

#endif
