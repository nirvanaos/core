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
#ifndef NIRVANA_ORB_CORE_TC_SEQUENCE_H_
#define NIRVANA_ORB_CORE_TC_SEQUENCE_H_
#pragma once

#include "TC_Base.h"
#include "TC_Impl.h"
#include "TC_Ref.h"

namespace CORBA {
namespace Core {

class TC_Sequence : public TC_Impl <TC_Sequence, TC_Base>
{
	typedef TC_Impl <TC_Sequence, TC_Base> Impl;
	typedef Internal::ABI <Internal::Sequence <void> > ABI;

public:
	using Servant::_s_length;
	using Servant::_s_content_type;

	TC_Sequence (TC_Ref&& content_type, ULong bound);

	bool equal (TypeCode::_ptr_type other) const
	{
		return TCKind::tk_sequence == other->kind () && bound_ == other->length ();
	}

	bool equivalent (TypeCode::_ptr_type other) const
	{
		return equal (dereference_alias (other));
	}

	ULong length () const NIRVANA_NOEXCEPT
	{
		return bound_;
	}

	TypeCode::_ref_type content_type () const NIRVANA_NOEXCEPT
	{
		return content_type_;
	}

	static size_t _s_n_size (Internal::Bridge <TypeCode>*, Internal::Interface*)
	{
		return sizeof (ABI);
	}

	static size_t _s_n_align (Internal::Bridge <TypeCode>*, Internal::Interface*)
	{
		return alignof (ABI);
	}

	static Octet _s_n_is_CDR (Internal::Bridge <TypeCode>*, Internal::Interface*)
	{
		return false;
	}

	void n_construct (void* p) const
	{
		Internal::check_pointer (p);
		reinterpret_cast <ABI*> (p)->reset ();
	}

	void n_destruct (void* p) const
	{
		Internal::check_pointer (p);
		ABI& abi = *reinterpret_cast <ABI*> (p);
	}

	void n_copy (void* dst, const void* src) const
	{
	}

	void n_move (void* dst, void* src) const
	{}

	void n_marshal_in (const void* src, size_t count, Internal::IORequest_ptr rq) const;

	void n_marshal_out (void* src, size_t count, Internal::IORequest_ptr rq) const;
	void n_unmarshal (Internal::IORequest_ptr rq, size_t count, void* dst) const;

private:
	TC_Ref content_type_;
	ULong bound_;
	size_t element_size_;
	bool is_CDR_;
};

}
}

#endif
