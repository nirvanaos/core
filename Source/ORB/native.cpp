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
#include <CORBA/CORBA.h>
#include <CORBA/Proxy/TypeCodeNative.h>
#include <Nirvana/OLF.h>

namespace CORBA {
namespace Internal {
template <>
const Char TypeCodeName <void*>::name_ [] = "Pointer";
}
}

namespace Nirvana {
typedef CORBA::Internal::TypeCodeNative <void*> TC_Pointer;
}

NIRVANA_SELECTANY
const Nirvana::ImportInterfaceT <CORBA::TypeCode> Nirvana::_tc_Pointer = {
	Nirvana::OLF_IMPORT_INTERFACE, nullptr, nullptr, NIRVANA_STATIC_BRIDGE (CORBA::TypeCode, Nirvana::TC_Pointer) };
NIRVANA_EXPORT (_exp_Nirvana_Pointer, CORBA::Internal::RepIdOf <void*>::id, CORBA::TypeCode, Nirvana::TC_Pointer);


