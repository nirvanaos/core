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
#ifndef NIRVANA_ORB_CORE_TC_ABSTRACT_H_
#define NIRVANA_ORB_CORE_TC_ABSTRACT_H_
#pragma once

#include "TC_RefBase.h"
#include "TC_Impl.h"
#include "ORB.h"

namespace CORBA {
namespace Core {

class TC_Abstract :
	public TC_Impl <TC_Abstract, TC_RefBase>
{
	typedef TC_Impl <TC_Abstract, TC_RefBase> Impl;

public:
	using TC_RefBase::_s_n_CDR_size;
	using TC_RefBase::_s_n_aligned_size;
	using TC_RefBase::_s_n_align;
	using TC_RefBase::_s_n_is_CDR;
	using TC_RefBase::_s_n_byteswap;
	using Servant::_s_id;
	using Servant::_s_name;

	TC_Abstract (IDL::String&& id, IDL::String&& name) noexcept;

	static void n_marshal_in (const void* src, size_t count, Internal::IORequest_ptr rq);

	static void n_marshal_out (void* src, size_t count, Internal::IORequest_ptr rq)
	{
		n_marshal_in (src, count, rq);
	}

	void n_unmarshal (Internal::IORequest_ptr rq, size_t count, void* dst) const
	{
		Internal::check_pointer (dst);
		for (Internal::Interface::_ref_type* p = reinterpret_cast <Internal::Interface::_ref_type*> (dst);
			count; ++p, --count) {
			*p = rq->unmarshal_abstract (id_);
		}
	}

	TypeCode::_ref_type get_compact_typecode ()
	{
		if (name_.empty ())
			return Impl::get_compact_typecode ();
		else
			return ORB::create_abstract_interface_tc (id_, IDL::String ());
	}
};

}
}

#endif
