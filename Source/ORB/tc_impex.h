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
#ifndef NIRVANA_ORB_CORE_TC_IMPEX_H_
#define NIRVANA_ORB_CORE_TC_IMPEX_H_
#pragma once

#include <Nirvana/OLF.h>

#define TC_IMPEX_EX(t, id, ...) const Nirvana::ImportInterfaceT <CORBA::TypeCode> NIRVANA_SELECTANY (CORBA::_tc_##t) =\
{ Nirvana::OLF_IMPORT_INTERFACE, nullptr, nullptr, NIRVANA_STATIC_BRIDGE (CORBA::TypeCode, __VA_ARGS__) };\
NIRVANA_EXPORT (_exp_CORBA_##t, id, CORBA::TypeCode, __VA_ARGS__);

// Import and export for type code
#define TC_IMPEX(t, ...) TC_IMPEX_EX (t, "CORBA/_tc_" #t, __VA_ARGS__)
#define TC_IMPEX_BY_ID(t) TC_IMPEX_EX (t, CORBA::Internal::TC_##t::RepositoryType::id, CORBA::Internal::TC_##t)

#endif
