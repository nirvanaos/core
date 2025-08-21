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
#ifndef NIRVANA_ORB_CORE_TC_FIXED_H_
#define NIRVANA_ORB_CORE_TC_FIXED_H_
#pragma once

#include "TC_Base.h"
#include "TC_Impl.h"
#include "../virtual_copy.h"

namespace CORBA {
namespace Core {

class TC_Fixed :
	public TC_Impl <TC_Fixed, TC_Base>
{
	typedef TC_Impl <TC_Fixed, TC_Base> Impl;
public:
	using CORBA::Internal::TypeCodeBase::_s_get_compact_typecode;

	TC_Fixed (UShort digits, Short scale) noexcept :
		Impl (TCKind::tk_fixed),
		digits_ (digits),
		scale_ (scale)
	{}

	Boolean equal (TypeCode::_ptr_type other) const
	{
		return TCKind::tk_fixed == other->kind ()
			&& digits_ == other->fixed_digits ()
			&& scale_ == other->fixed_scale ();
	}
	
	Boolean equivalent (TypeCode::_ptr_type other) const
	{
		return equal (dereference_alias (other));
	}

	using Servant::_s_fixed_digits;
	using Servant::_s_fixed_scale;

	UShort fixed_digits () const noexcept
	{
		return digits_;
	}

	Short fixed_scale () const noexcept
	{
		return scale_;
	}

	size_t n_size () const noexcept
	{
		return size ();
	}

	static size_t _s_n_align (Internal::Bridge <TypeCode>*, Internal::Interface*) noexcept
	{
		return 1;
	}

	void n_construct (void* p) const
	{
		Internal::check_pointer (p);
		Nirvana::DecimalBase::BCD_zero ((Octet*)p, size ());
	}

	static void _s_n_destruct (Internal::Bridge <TypeCode>*, void* p, Internal::Interface*) noexcept
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
		void* data;
		size_t cb = size ();
		size_t total = cb * count;
		rq->unmarshal (1, total, data);
		for (const Octet* p = (const Octet*)data, *end = p + total; p != end; p += cb) {
			Internal::BCD_check (p, cb);
		}
		Nirvana::Core::virtual_copy (data, total, dst);
	}

private:
	size_t size () const noexcept
	{
		return (digits_ + 2) / 2;
	}

private:
	const UShort digits_;
	const Short  scale_;
};

}
}

#endif
