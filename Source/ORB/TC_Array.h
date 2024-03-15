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
#ifndef NIRVANA_ORB_CORE_TC_ARRAY_H_
#define NIRVANA_ORB_CORE_TC_ARRAY_H_
#pragma once

#include "TC_ArrayBase.h"
#include "TC_Impl.h"
#include "ORB.h"

namespace CORBA {
namespace Core {

class TC_Array : public TC_Impl <TC_Array, TC_ArrayBase>
{
	typedef TC_Impl <TC_Array, TC_ArrayBase> Impl;

public:
	using Servant::_s_length;
	using Servant::_s_content_type;
	using Servant::_s_n_size;
	using Servant::_s_n_align;
	using Servant::_s_get_compact_typecode;

	TC_Array () :
		Impl (TCKind::tk_array),
		traits_ (0),
		element_align_ (1),
		element_size_ (0),
		var_len_ (false)
	{}

	TC_Array (TC_Ref&& content_type, ULong bound) :
		TC_Array ()
	{
		set_content_type (std::move (content_type), bound);
	}

	void set_content_type (TC_Ref&& content_type, ULong bound)
	{
		if (TC_ArrayBase::set_content_type (std::move (content_type), bound))
			initialize ();
	}

	TypeCode::_ref_type get_compact_typecode ()
	{
		TypeCode::_ref_type compact_content = content_type_->get_compact_typecode ();
		if (&content_type_ == &TypeCode::_ptr_type (compact_content))
			return Impl::get_compact_typecode ();
		else
			return Static_the_orb::create_array_tc (bound_, compact_content);
	}

	size_t n_size () const noexcept
	{
		return traits_.element_count * element_size_;
	}

	size_t n_align () const noexcept
	{
		return element_align_;
	}

	void n_construct (void* p) const
	{
		Octet* pv = (Octet*)p;
		size_t count = traits_.element_count;
		do {
			content_type_->n_construct (pv);
			pv += element_size_;
		} while (--count);
	}

	void n_destruct (void* p) const
	{
		if (var_len_) {
			Octet* pv = (Octet*)p;
			size_t count = traits_.element_count;
			do {
				content_type_->n_destruct (pv);
				pv += element_size_;
			} while (--count);
		}
	}

	void n_copy (void* dst, const void* src) const
	{
		if (var_len_) {
			const Octet* ps = (const Octet*)src;
			Octet* pd = (Octet*)dst;
			size_t count = traits_.element_count;
			do {
				content_type_->n_copy (pd, ps);
				ps += element_size_;
				pd += element_size_;
			} while (--count);
		} else
			Nirvana::real_copy ((const Octet*)src, (const Octet*)src + element_size_ * traits_.element_count, (Octet*)dst);
	}

	void n_move (void* dst, void* src) const
	{
		if (var_len_) {
			Octet* ps = (Octet*)src;
			Octet* pd = (Octet*)dst;
			size_t count = traits_.element_count;
			do {
				content_type_->n_move (pd, ps);
				ps += element_size_;
				pd += element_size_;
			} while (--count);
		} else
			Nirvana::real_copy ((const Octet*)src, (const Octet*)src + element_size_ * traits_.element_count, (Octet*)dst);
	}

	void n_marshal_in (const void* src, size_t count, Internal::IORequest_ptr rq) const
	{
		if (!count)
			return;

		Internal::check_pointer (src);
		traits_.element_type->n_marshal_in (src, count * traits_.element_count, rq);
	}

	void n_marshal_out (void* src, size_t count, Internal::IORequest_ptr rq) const
	{
		if (!count)
			return;

		Internal::check_pointer (src);
		traits_.element_type->n_marshal_out (src, count * traits_.element_count, rq);
	}

	void n_unmarshal (Internal::IORequest_ptr rq, size_t count, void* dst) const
	{
		if (!count)
			return;

		Internal::check_pointer (dst);
		traits_.element_type->n_unmarshal (rq, count * traits_.element_count, dst);
	}

private:
	virtual bool set_recursive (const IDL::String& id, const TC_Ref& ref) noexcept override;
	void initialize ();

private:
	ArrayTraits traits_;
	size_t element_align_;
	size_t element_size_;
	bool var_len_;
};

}
}

#endif
