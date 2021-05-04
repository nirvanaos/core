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
#include <Nirvana/ImportInterface.h>

NIRVANA_LINK_SYMBOL (_exp_Nirvana_g_memory)
NIRVANA_LINK_SYMBOL (_exp_Nirvana_g_runtime_support)
NIRVANA_LINK_SYMBOL (_exp_PortableServer_POA_ProxyFactory)
NIRVANA_LINK_SYMBOL (_exp_PortableServer_POA_ServantAlreadyActive_TC)
NIRVANA_LINK_SYMBOL (_exp_PortableServer_POA_ObjectNotActive_TC)
NIRVANA_LINK_SYMBOL (_exp_CORBA_Nirvana_TCKind_TC)
NIRVANA_LINK_SYMBOL (_exp_CORBA_Nirvana_TypeCode_BadKind_TC)
NIRVANA_LINK_SYMBOL (_exp_CORBA_Nirvana_TypeCode_Bounds_TC)
NIRVANA_LINK_SYMBOL (_exp_CORBA_TypeCode_TC)

NIRVANA_LINK_SYMBOL (_exp_CORBA_Nirvana_g_object_factory)

#define EXPORT_TC(t) NIRVANA_LINK_SYMBOL (_exp_CORBA_tc_##t)
EXPORT_TC (void)
EXPORT_TC (short)
EXPORT_TC (long)
EXPORT_TC (ushort)
EXPORT_TC (ulong)
EXPORT_TC (float)
EXPORT_TC (double)
EXPORT_TC (boolean)
EXPORT_TC (char)
EXPORT_TC (octet)
EXPORT_TC (longlong)
EXPORT_TC (ulonglong)
EXPORT_TC (longdouble)
EXPORT_TC (wchar)
EXPORT_TC (any)
EXPORT_TC (string)
EXPORT_TC (wstring)
EXPORT_TC (Object)
EXPORT_TC (TypeCode)

SYSTEM_EXCEPTIONS(EXPORT_TC)
