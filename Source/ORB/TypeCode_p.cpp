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
#include <CORBA/Proxy/Proxy.h>

namespace CORBA {
namespace Internal {

template <>
const Char TypeCodeName <TCKind>::name_ [] = "TCKind";

template <>
const Char* const TypeCodeEnum <TCKind>::members_ [] = {
	"tk_null", "tk_void",
	"tk_short", "tk_long", "tk_ushort", "tk_ulong",
	"tk_float", "tk_double", "tk_boolean", "tk_char",
	"tk_octet", "tk_any", "tk_TypeCode", "tk_Principal", "tk_objref",
	"tk_struct", "tk_union", "tk_enum", "tk_string",
	"tk_sequence", "tk_array", "tk_alias", "tk_except",
	"tk_longlong", "tk_ulonglong", "tk_longdouble",
	"tk_wchar", "tk_wstring", "tk_fixed",
	"tk_value", "tk_value_box",
	"tk_native",
	"tk_abstract_interface",
	"tk_local_interface"
};

}

}

NIRVANA_EXPORT (_exp_CORBA_Nirvana_TCKind, CORBA::Internal::RepIdOf < ::CORBA::TCKind>::repository_id_, CORBA::TypeCode, CORBA::Internal::TypeCodeEnum < ::CORBA::TCKind>)

NIRVANA_EXPORT (_exp_CORBA_Nirvana_TypeCode_BadKind, CORBA::Internal::RepIdOf < ::CORBA::Internal::Definitions <::CORBA::TypeCode>::BadKind>::repository_id_,
	CORBA::TypeCode, CORBA::Internal::TypeCodeException < ::CORBA::Internal::Definitions <::CORBA::TypeCode>::BadKind, false>)

NIRVANA_EXPORT (_exp_CORBA_Nirvana_TypeCode_Bounds, CORBA::Internal::RepIdOf < ::CORBA::Internal::Definitions <::CORBA::TypeCode>::Bounds>::repository_id_,
	CORBA::TypeCode, CORBA::Internal::TypeCodeException < ::CORBA::Internal::Definitions <::CORBA::TypeCode>::Bounds, false>)
