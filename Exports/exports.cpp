/// \file
/// This file must not be included in the core library build.
/// It must be included in the Nirvana core executable project instead.
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
#include <Nirvana/OLF.h>
#include <CORBA/system_exceptions.h>

NIRVANA_LINK_SYMBOL (_exp_CORBA_Internal_g_object_factory)

#define EXPORT_TC(t) NIRVANA_LINK_SYMBOL (_exp_CORBA_##t)
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
EXPORT_TC (TCKind)
EXPORT_TC (TypeCode)
EXPORT_TC (TypeCode_BadKind)
EXPORT_TC (TypeCode_Bounds)
EXPORT_TC (ValueBase)

SYSTEM_EXCEPTIONS(EXPORT_TC)

NIRVANA_LINK_SYMBOL (exp_Nirvana_file_factory)
