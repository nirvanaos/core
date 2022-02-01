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
#include <CORBA/Proxy/TypeCodeEnum.h>
#include <CORBA/Proxy/TypeCodeException.h>
#include "tc_impex.h"
#include <CORBA/system_exceptions.h>

namespace CORBA {
namespace Internal {

template <>
const Char RepIdOf <CompletionStatus>::repository_id_ [] = CORBA_REPOSITORY_ID ("CompletionStatus");

template <>
const Char TypeCodeName <CompletionStatus>::name_ [] = "CompletionStatus";

template <>
const Char* const TypeCodeEnum <CompletionStatus>::members_ [] = {
	"COMPLETED_YES",
	"COMPLETED_NO",
	"COMPLETED_MAYBE"
};

template <>
const Parameter TypeCodeMembers <SystemException>::members_ [] = {
	{ "minor", Type <ULong>::type_code },
	{ "completed", Type <CompletionStatus>::type_code }
};

template <class E>
class TypeCodeSystemException :
	public TypeCodeStatic <TypeCodeSystemException <E>, TypeCodeWithId <TCKind::tk_except, RepIdOf <E> >, TypeCodeOps <SystemException::_Data> >,
	public TypeCodeMembers <SystemException>
{
public:
	using TypeCodeMembers <SystemException>::_member_count;
	using TypeCodeMembers <SystemException>::_member_name;
	using TypeCodeMembers <SystemException>::_member_type;

	static Type <String>::ABI_ret _name (Bridge <TypeCode>* _b, Interface* _env)
	{
		return const_string_ret_p (E::__name ());
	}
};

typedef TypeCodeEnum <CompletionStatus> TC_CompletionStatus;

#define TC_EXCEPTION(E) typedef TypeCodeSystemException <E> TC_##E;

template <>
const Parameter TypeCodeMembers <UnknownUserException>::members_ [] = {
	{ "exception", Type <Any>::type_code }
};

typedef TypeCodeException <UnknownUserException, true> TC_UnknownUserException;

SYSTEM_EXCEPTIONS (TC_EXCEPTION)
//TC_EXCEPTION (UnknownUserException)

}
}

TC_IMPEX_BY_ID (CompletionStatus)

SYSTEM_EXCEPTIONS (TC_IMPEX_BY_ID)
TC_IMPEX_BY_ID (UnknownUserException)
