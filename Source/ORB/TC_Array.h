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
	using Servant::_s_n_byteswap;

	TC_Array () :
		Impl (TCKind::tk_array)
	{}

	TC_Array (TC_Ref&& content_type, ULong bound) :
		Impl (TCKind::tk_array, std::move (content_type), bound)
	{}

	size_t n_size () const NIRVANA_NOEXCEPT
	{
		return bound_ * element_size_;
	}

	size_t n_align () const NIRVANA_NOEXCEPT
	{
		return element_align_;
	}

	Boolean n_is_CDR () const NIRVANA_NOEXCEPT
	{
		return KIND_CDR == content_kind_;
	}

	void n_construct (void* p) const
	{
		Octet* pv = (Octet*)p;
		size_t count = bound_;
		do {
			content_type_->n_construct (pv);
			pv += element_size_;
		} while (--count);
	}

	void n_destruct (void* p) const
	{
		Octet* pv = (Octet*)p;
		size_t count = bound_;
		do {
			content_type_->n_destruct (pv);
			pv += element_size_;
		} while (--count);
	}

	void n_copy (void* dst, const void* src) const
	{
		const Octet* ps = (const Octet*)src;
		Octet* pd = (Octet*)dst;
		size_t count = bound_;
		do {
			content_type_->n_copy (pd, ps);
			ps += element_size_;
			pd += element_size_;
		} while (--count);
	}

	void n_move (void* dst, void* src) const
	{
		Octet* ps = (Octet*)src;
		Octet* pd = (Octet*)dst;
		size_t count = bound_;
		do {
			content_type_->n_move (pd, ps);
			ps += element_size_;
			pd += element_size_;
		} while (--count);
	}

	void n_marshal_in (const void* src, size_t count, Internal::IORequest_ptr rq) const
	{
		marshal (src, count, rq, false);
	}

	void n_marshal_out (void* src, size_t count, Internal::IORequest_ptr rq) const
	{
		marshal (src, count, rq, true);
	}

	void n_unmarshal (Internal::IORequest_ptr rq, size_t count, void* dst) const
	{
		Internal::check_pointer (dst);
		switch (content_kind_) {
			case KIND_CHAR:
				rq->unmarshal_char (count * element_size_, (Char*)dst);
				break;

			case KIND_WCHAR:
				rq->unmarshal_wchar (count * element_size_ / sizeof (WChar), (WChar*)dst);
				break;

			case KIND_CDR:
				rq->unmarshal (element_align_, element_size_ * count, dst);
				break;

			default:
				content_type_->n_unmarshal (rq, count, dst);
				break;
		}
	}

	void n_byteswap (void* p, size_t count) const
	{
		content_type_->n_byteswap (p, count * bound_);
	}

private:
	void marshal (const void* src, size_t count, Internal::IORequest_ptr rq, bool out) const;
};

}
}

#endif