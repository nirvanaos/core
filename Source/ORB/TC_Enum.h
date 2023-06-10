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
#include "ORB.h"
#include "../Array.h"
#include "../UserAllocator.h"

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

	typedef Nirvana::Core::Array <IDL::String, Nirvana::Core::UserAllocator> Members;

	TC_Enum (IDL::String&& id, IDL::String&& name, Members&& members) noexcept;

	Boolean equal (TypeCode::_ptr_type other) const
	{
		if (!TC_IdName::equal (other))
			return false;

		if (other->member_count () != (ULong)members_.size ())
			return false;

		for (ULong i = 0, cnt = (ULong)members_.size (); i < cnt; ++i) {
			if (!(members_ [i] == other->member_name (i)))
				return false;
		}
		return true;
	}

	Boolean equivalent (TypeCode::_ptr_type other) const
	{
		return TypeCodeBase::equivalent_ (TCKind::tk_enum, id_,
			(ULong)members_.size (), TypeCodeBase::dereference_alias (other)) != TypeCodeBase::EqResult::NO;
	}

	ULong member_count () const noexcept
	{
		return (ULong)members_.size ();
	}

	IDL::String member_name (ULong i) const
	{
		if (i >= members_.size ())
			throw Bounds ();
		return members_ [i];
	}

	TypeCode::_ref_type get_compact_typecode ()
	{
		bool already_compact = true;
		if (!name_.empty ())
			already_compact = false;
		else {
			for (const auto& m : members_) {
				if (!m.empty ()) {
					already_compact = false;
					break;
				}
			}
		}
		if (already_compact)
			return Servant::_get_ptr ();
		else
			return ORB::create_enum_tc (id_, IDL::String (), EnumMemberSeq (members_.size ()));
	}

	size_t n_size () const noexcept
	{
		return sizeof (Internal::ABI_enum);
	}

	size_t n_align () const noexcept
	{
		return alignof (Internal::ABI_enum);
	}

	bool n_is_CDR () const noexcept
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

	using Servant::_s_n_byteswap;

	static void n_byteswap (void* p, size_t count)
	{
		Internal::check_pointer (p);
		for (Internal::ABI_enum* pv = (Internal::ABI_enum*)p, *end = pv + count; pv != end; ++pv) {
			Nirvana::byteswap (*pv);
		}
	}

private:
	void check (const void* p, size_t count) const;

private:
	Members members_;
};

}
}

#endif
