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
#include "tc_impex.h"

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

typedef CORBA::Internal::TypeCodeEnum < ::CORBA::TCKind> TC_TCKind;

class TC_TypeCode :
	public TypeCodeStatic <TC_TypeCode, TypeCodeTK <tk_TypeCode>, TypeCodeOps <TypeCode> >
{
public:
	typedef TypeCode RepositoryType;
};

typedef TypeCodeException <Definitions <TypeCode>::BadKind, false> TC_BadKind;
typedef TypeCodeException <Definitions <TypeCode>::Bounds, false> TC_Bounds;

}
}

TC_IMPEX_BY_ID (TypeCode)
TC_IMPEX_BY_ID (TCKind)
TC_IMPEX_BY_ID_EX (CORBA::Internal::Definitions <CORBA::TypeCode>, BadKind)
TC_IMPEX_BY_ID_EX (CORBA::Internal::Definitions <CORBA::TypeCode>, Bounds)
