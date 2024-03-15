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
#ifndef NIRVANA_ORB_CORE_TC_STRUCT_H_
#define NIRVANA_ORB_CORE_TC_STRUCT_H_
#pragma once

#include "TC_IdName.h"
#include "TC_Impl.h"
#include "TC_Ref.h"
#include "../Array.h"
#include "../UserAllocator.h"
#include "ORB.h"

namespace CORBA {
namespace Core {

class TC_Struct :
	public TC_Impl <TC_Struct, TC_Complex <TC_IdName> >
{
	typedef TC_Impl <TC_Struct, TC_Complex <TC_IdName> > Impl;

public:
	using Servant::_s_id;
	using Servant::_s_name;
	using Servant::_s_member_count;
	using Servant::_s_member_name;
	using Servant::_s_member_type;
	using Servant::_s_get_compact_typecode;

	struct Member
	{
		IDL::String name;
		TC_Ref type;
		size_t offset;
	};

	typedef Nirvana::Core::Array <Member, Nirvana::Core::UserAllocator> Members;

	TC_Struct (TCKind kind, IDL::String&& id, IDL::String&& name) :
		Impl (kind, std::move (id), std::move (name)),
		align_ (1),
		size_ (0),
		kind_ (KIND_CDR)
	{}

	TC_Struct (TCKind kind, IDL::String&& id, IDL::String&& name, Members&& members) :
		TC_Struct (kind, std::move (id), std::move (name))
	{
		set_members (std::move (members));

		TC_Ref self (_get_ptr (), this);
		for (auto& m : members_) {
			m.type.replace_recursive_placeholder (id_, self);
		}
	}

	void set_members (Members&& members);

	Boolean equal (TypeCode::_ptr_type other)
	{
		return Static_the_orb::tc_equal (Impl::_get_ptr (), other);
	}

	bool equivalent (TypeCode::_ptr_type other)
	{
		return Static_the_orb::tc_equivalent (Impl::_get_ptr (), other);
	}

	ULong member_count () const noexcept
	{
		return (ULong)members_.size ();
	}

	IDL::String member_name (ULong i) const
	{
		if (i >= members_.size ())
			throw Bounds ();
		return members_ [i].name;
	}

	TypeCode::_ref_type member_type (ULong i) const
	{
		if (i >= members_.size ())
			throw Bounds ();
		return members_ [i].type;
	}

	TypeCode::_ref_type get_compact_typecode ()
	{
		return Static_the_orb::get_compact_typecode (_get_ptr ());
	}

	size_t n_size () const noexcept
	{
		return size_;
	}

	size_t n_align () const noexcept
	{
		return align_;
	}

	void n_construct (void* p) const
	{
		Internal::check_pointer (p);
		for (const auto& m : members_) {
			m.type->n_construct ((Octet*)p + m.offset);
		}
	}

	void n_destruct (void* p) const
	{
		Internal::check_pointer (p);
		if (KIND_VARLEN == kind_) {
			for (const auto& m : members_) {
				m.type->n_destruct ((Octet*)p + m.offset);
			}
		}
	}

	void n_copy (void* dst, const void* src) const
	{
		Internal::check_pointer (dst);
		Internal::check_pointer (src);
		if (KIND_VARLEN != kind_)
			Nirvana::real_move ((const Octet*)src, (const Octet*)src + size_, (Octet*)dst);
		else
			for (const auto& m : members_) {
				m.type->n_copy ((Octet*)dst + m.offset, (const Octet*)src + m.offset);
			}
	}

	void n_move (void* dst, void* src) const
	{
		Internal::check_pointer (dst);
		Internal::check_pointer (src);
		if (KIND_VARLEN != kind_)
			Nirvana::real_move ((const Octet*)src, (const Octet*)src + size_, (Octet*)dst);
		else
			for (const auto& m : members_) {
				m.type->n_move ((Octet*)dst + m.offset, (Octet*)src + m.offset);
			}
	}

	void n_marshal_in (const void* src, size_t count, Internal::IORequest_ptr rq) const
	{
		Internal::check_pointer (src);
		if (KIND_CDR == kind_)
			marshal_CDR (src, count, rq);
		else {
			for (const Octet* osrc = (const Octet*)src; count; osrc += size_, --count) {
				for (const auto& m : members_) {
					m.type->n_marshal_in (osrc + m.offset, 1, rq);
				}
			}
		}
	}

	void n_marshal_out (void* src, size_t count, Internal::IORequest_ptr rq) const
	{
		Internal::check_pointer (src);
		if (KIND_CDR == kind_)
			marshal_CDR (src, count, rq);
		else {
			for (Octet* osrc = (Octet*)src; count; osrc += size_, --count) {
				for (const auto& m : members_) {
					m.type->n_marshal_out (osrc + m.offset, 1, rq);
				}
			}
		}
	}

	void n_unmarshal (Internal::IORequest_ptr rq, size_t count, void* dst) const
	{
		Internal::check_pointer (dst);
		if (KIND_CDR == kind_) {
			for (Octet* odst = (Octet*)dst; count; odst += size_, --count) {
				rq->unmarshal (CDR_size_.alignment, CDR_size_.size, odst);
			}
		} else {
			for (Octet* odst = (Octet*)dst; count; odst += size_, --count) {
				for (const auto& m : members_) {
					m.type->n_unmarshal (rq, 1, odst + m.offset);
				}
			}
		}
	}

protected:
	virtual bool mark () noexcept override;
	virtual bool set_recursive (const IDL::String& id, const TC_Ref& ref) noexcept override;

private:
	void marshal_CDR (const void* src, size_t count, Internal::IORequest_ptr rq) const;

private:
	Members members_;
	size_t align_;
	size_t size_;

	enum
	{
		KIND_CDR,
		KIND_FIXLEN,
		KIND_VARLEN
	}
	kind_;

	SizeAndAlign CDR_size_;
};

}
}

#endif
