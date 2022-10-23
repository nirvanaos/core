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
#ifndef NIRVANA_ORB_CORE_TC_RECURSIVE_H_
#define NIRVANA_ORB_CORE_TC_RECURSIVE_H_
#pragma once

#include <CORBA/Server.h>
#include "RefCntProxy.h"
#include "TC_Ref.h"

namespace CORBA {
namespace Core {

class TC_Recursive :
	public servant_traits <TypeCode>::Servant <TC_Recursive>,
	public TC_ComplexBase,
	public Internal::LifeCycleRefCnt <TC_Recursive>
{
public:
	TC_Recursive (IDL::String&& id) :
		TC_ComplexBase (true),
		id_ (std::move (id)),
		ref_cnt_ (1)
	{}

	virtual bool set_recursive (const IDL::String& id, const TC_Ref& ref) NIRVANA_NOEXCEPT override
	{
		if (id_ == id) {
			content_ = ref;
			return true;
		}
		return false;
	}

	virtual bool has_extern_ref () const NIRVANA_NOEXCEPT override
	{
		return ref_cnt_.load () != 0;
	}

	virtual bool mark () NIRVANA_NOEXCEPT override
	{
		if (!TC_ComplexBase::mark ())
			return false;
		content_.mark ();
		return true;
	}

	Boolean equal (TypeCode::_ptr_type other) const
	{
		return content ()->equal (other);
	}

	Boolean equivalent (TypeCode::_ptr_type other) const
	{
		return content ()->equivalent (other);
	}

	TypeCode::_ref_type get_compact_typecode () const
	{
		return content ()->get_compact_typecode ();
	}

	TCKind kind () const
	{
		return content ()->kind ();
	}

	RepositoryId id () const
	{
		return content ()->id ();
	}

	Identifier name () const
	{
		return content ()->name ();
	}

	ULong member_count () const
	{
		return content ()->member_count ();
	}

	Identifier member_name (ULong i) const
	{
		return content ()->member_name (i);
	}

	TypeCode::_ref_type member_type (ULong i) const
	{
		return content ()->member_type (i);
	}

	Any member_label (ULong i) const
	{
		return content ()->member_label (i);
	}

	TypeCode::_ref_type discriminator_type () const
	{
		return content ()->discriminator_type ();
	}

	Long default_index () const
	{
		return content ()->default_index ();
	}

	ULong length () const
	{
		return content ()->length ();
	}

	TypeCode::_ref_type content_type () const
	{
		return content ()->content_type ();
	}

	UShort fixed_digits () const
	{
		return content ()->fixed_digits ();
	}

	Short fixed_scale () const
	{
		return content ()->fixed_scale ();
	}

	Visibility member_visibility (ULong i) const
	{
		return content ()->member_visibility (i);
	}

	ValueModifier type_modifier () const
	{
		return content ()->type_modifier ();
	}

	TypeCode::_ref_type concrete_base_type () const
	{
		return content ()->concrete_base_type ();
	}

	size_t n_size () const
	{
		return content ()->n_size ();
	}

	size_t n_align () const
	{
		return content ()->n_align ();
	}

	bool n_is_CDR () const
	{
		return content ()->n_is_CDR ();
	}

	void n_construct (void* p) const
	{
		content ()->n_construct (p);
	}

	void n_destruct (void* p) const
	{
		content ()->n_destruct (p);
	}

	void n_copy (void* dst, const void* src) const
	{
		content ()->n_copy (dst, src);
	}

	void n_move (void* dst, void* src) const
	{
		content ()->n_move (dst, src);
	}

	void n_marshal_in (const void* src, size_t count, Internal::IORequest_ptr rq) const
	{
		content ()->n_marshal_in (src, count, rq);
	}

	void n_marshal_out (void* src, size_t count, Internal::IORequest_ptr rq) const
	{
		content ()->n_marshal_out (src, count, rq);
	}

	void n_unmarshal (Internal::IORequest_ptr rq, size_t count, void* dst) const
	{
		content ()->n_unmarshal (rq, count, dst);
	}

	void n_byteswap (void* p, size_t count) const
	{
		content ()->n_byteswap (p, count);
	}

	void _add_ref () NIRVANA_NOEXCEPT
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () NIRVANA_NOEXCEPT
	{
		ref_cnt_.decrement ();
	}

private:
	TypeCode::_ptr_type content () const
	{
		if (!content_)
			throw BAD_PARAM (MAKE_OMG_MINOR (13)); // Attempt to use an incomplete TypeCode as a parameter.
		return content_;
	}

private:
	IDL::String id_;
	RefCntProxy ref_cnt_;
	TC_Ref content_;
};

}
}

#endif
