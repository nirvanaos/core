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
#include <CORBA/TypeCodeString.h>
#include <CORBA/TypeCodeValue.h>
#include <CORBA/TypeCodeInterface.h>
#include "../Source/ORB/tc_impex.h"

namespace CORBA {
namespace Internal {

template <typename T, TCKind tk>
class TypeCodeScalar : public TypeCodeStatic <TypeCodeScalar <T, tk>,
	TypeCodeTK <tk>, TypeCodeOps <T> > {};

typedef TypeCodeInterface <Object> TC_Object;

template <>
const Char TypeCodeName <Object>::name_ [] = "Object";

template <>
class TypeCodeValue <ValueBase> : public TypeCodeValueConcrete <ValueBase, VM_NONE, false, nullptr>
{};

typedef TypeCodeValue <ValueBase> TC_ValueBase;

template <>
const Char TypeCodeName <ValueBase>::name_ [] = "ValueBase";

class TC_TypeCode : public TypeCodeStatic <TC_TypeCode,
	TypeCodeTK <TCKind::tk_TypeCode>, TypeCodeOps <TypeCode> >
{};

}
}

#define TC_IMPL_SCALAR(T, t) TC_IMPEX (t, CORBA::Internal::TypeCodeScalar <T, CORBA::TCKind::tk_##t>)

TC_IMPL_SCALAR (void, void)
TC_IMPL_SCALAR (CORBA::Short, short)
TC_IMPL_SCALAR (CORBA::Long, long)
TC_IMPL_SCALAR (CORBA::UShort, ushort)
TC_IMPL_SCALAR (CORBA::ULong, ulong)
TC_IMPL_SCALAR (CORBA::Float, float)
TC_IMPL_SCALAR (CORBA::Double, double)
TC_IMPL_SCALAR (CORBA::Boolean, boolean)
TC_IMPL_SCALAR (CORBA::Char, char)
TC_IMPL_SCALAR (CORBA::Octet, octet)
TC_IMPL_SCALAR (CORBA::LongLong, longlong)
TC_IMPL_SCALAR (CORBA::ULongLong, ulonglong)
TC_IMPL_SCALAR (CORBA::LongDouble, longdouble)
TC_IMPL_SCALAR (CORBA::WChar, wchar)
TC_IMPL_SCALAR (CORBA::Any, any)
TC_IMPEX (string, CORBA::Internal::TypeCodeString <CORBA::Internal::String, 0>)
TC_IMPEX (wstring, CORBA::Internal::TypeCodeString <CORBA::Internal::WString, 0>)

TC_IMPEX_BY_ID (Object)
TC_IMPEX_BY_ID (ValueBase)
TC_IMPEX_EX (TypeCode, CORBA::Internal::RepIdOf <CORBA::TypeCode>::id, CORBA::Internal::TC_TypeCode)
