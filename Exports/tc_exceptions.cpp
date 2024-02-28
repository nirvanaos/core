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
#include <CORBA/Proxy/TypeCodeEnum.h>
#include <CORBA/Proxy/TypeCodeException.h>
#include "../Source/ORB/tc_impex.h"
#include <CORBA/system_exceptions.h>

namespace CORBA {
namespace Internal {

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

class TypeCodeSystemExceptionBase :
	public TypeCodeTK <TCKind::tk_except>,
	public TypeCodeOps <SystemException::_Data>,
	public TypeCodeMembers <SystemException>,
	public TypeCodeORB
{
public:
	using TypeCodeORB::_s_get_compact_typecode;
	using TypeCodeMembers <SystemException>::_s_member_count;
	using TypeCodeMembers <SystemException>::_s_member_name;
	using TypeCodeMembers <SystemException>::_s_member_type;
};

template <class E>
class TypeCodeSystemException :
	public ServantStatic <TypeCodeSystemException <E>, TypeCode>,
	public TypeCodeSystemExceptionBase
{
	typedef TypeCodeSystemExceptionBase Base;

public:
	using Base::_s_kind;
	using Base::_s_id;
	using Base::_s_name;
	using Base::_s_equal;
	using Base::_s_equivalent;
	using Base::_s_get_compact_typecode;
	using Base::_s_member_count;
	using Base::_s_member_name;
	using Base::_s_member_type;
	using Base::_s_member_label;
	using Base::_s_discriminator_type;
	using Base::_s_default_index;
	using Base::_s_length;
	using Base::_s_content_type;
	using Base::_s_fixed_digits;
	using Base::_s_fixed_scale;
	using Base::_s_member_visibility;
	using Base::_s_type_modifier;
	using Base::_s_concrete_base_type;
	using Base::_s_n_aligned_size;
	using Base::_s_n_CDR_size;
	using Base::_s_n_align;
	using Base::_s_n_is_CDR;

	typedef RepIdOf <E> RepositoryType;

	static Type <String>::ABI_ret _s_id (Bridge <TypeCode>* _b, Interface* _env)
	{
		return const_string_ret (RepositoryType::id);
	}

	static Type <String>::ABI_ret _s_name (Bridge <TypeCode>* _b, Interface* _env)
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

}
}

TC_IMPEX_BY_ID (CompletionStatus)

SYSTEM_EXCEPTIONS (TC_IMPEX_BY_ID)
TC_IMPEX_BY_ID (UnknownUserException)
