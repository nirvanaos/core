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
#ifndef NIRVANA_ORB_CORE_TC_ENUM_H_
#define NIRVANA_ORB_CORE_TC_ENUM_H_
#pragma once

#include "TC_IdName.h"
#include "TC_Impl.h"
#include "../Array.h"

namespace CORBA {
namespace Core {

class TC_Enum :
	public TC_Impl <TC_Enum, TC_IdName>
{
	typedef TC_Impl <TC_Enum, TC_IdName> Impl;

public:
	using Servant::_s_id;
	using Servant::_s_name;
	using Servant::_s_member_count;
	using Servant::_s_member_name;

	typedef Nirvana::Core::Array <String, Nirvana::Core::SharedAllocator> Members;

	TC_Enum (String&& id, String&& name, Members&& members) NIRVANA_NOEXCEPT;

	bool equal (TypeCode::_ptr_type other) const
	{
		if (!TC_IdName::equal (other))
			return false;

		for (ULong i = 0, cnt = (ULong)members_.size (); i < cnt; ++i) {
			if (!(members_ [i] == other->member_name (i)))
				return false;
		}
		return true;
	}

	bool equivalent (TypeCode::_ptr_type other) const
	{
		TypeCode::_ptr_type tc = dereference_alias (other);
		return TC_IdName::equivalent_no_alias (tc) && members_.size () == tc->member_count ();
	}

	ULong member_count () const NIRVANA_NOEXCEPT
	{
		return (ULong)members_.size ();
	}

	IDL::String member_name (ULong i) const
	{
		if (i >= members_.size ())
			throw Bounds ();
		return members_ [i];
	}

	size_t n_size () const NIRVANA_NOEXCEPT
	{
		return sizeof (Internal::ABI_enum);
	}

	size_t n_align () const NIRVANA_NOEXCEPT
	{
		return alignof (Internal::ABI_enum);
	}

	bool n_is_CDR () const NIRVANA_NOEXCEPT
	{
		return true;
	}

	void n_construct (void* p) const
	{
		Internal::check_pointer (p);
		*(Internal::ABI_enum*)p = 0;
	}

	void n_destruct (void* p) const
	{}

	void n_copy (void* dst, const void* src) const;

	void n_move (void* dst, void* src) const
	{
		n_copy (dst, src);
	}

	void n_marshal_in (const void* src, size_t count, Internal::IORequest_ptr rq) const;

	void n_marshal_out (void* src, size_t count, Internal::IORequest_ptr rq) const
	{
		n_marshal_in (src, count, rq);
	}

	void n_unmarshal (Internal::IORequest_ptr rq, size_t count, void* dst) const
	{
		Internal::check_pointer (dst);
		Internal::ABI_enum* p = (Internal::ABI_enum*)dst;
		if (rq->unmarshal (alignof (Internal::ABI_enum), sizeof (Internal::ABI_enum) * count, p))
			for (Internal::ABI_enum* pv = p, *end = pv + count; pv != end; ++pv) {
				Nirvana::byteswap (*pv);
			}
		check (p, count);
	}

private:
	void check (const void* p, size_t count) const;

private:
	Members members_;
};

}
}

#endif
