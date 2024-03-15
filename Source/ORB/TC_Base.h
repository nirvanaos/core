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
#ifndef NIRVANA_ORB_CORE_TC_BASE_H_
#define NIRVANA_ORB_CORE_TC_BASE_H_
#pragma once

#include <CORBA/CORBA.h>
#include "RefCntProxy.h"
#include "GarbageCollector.h"

namespace CORBA {
namespace Core {

class NIRVANA_NOVTABLE TC_Base :
	public CORBA::Internal::TypeCodeBase,
	public SyncGC
{
public:
	virtual void _add_ref () noexcept override;
	virtual void _remove_ref () noexcept override;

	TCKind kind () const noexcept
	{
		return kind_;
	}
	
protected:
	TC_Base (TCKind kind) noexcept :
		ref_cnt_ (1),
		kind_ (kind)
	{}

	virtual ~TC_Base ()
	{}

	virtual void collect_garbage () noexcept;

	static TypeCode::_ref_type dereference_alias (TypeCode::_ptr_type tc);

	struct SizeAndAlign
	{
		unsigned alignment; // Alignment of first element after a gap
		unsigned size;      // Size of data after the gap.

		SizeAndAlign (unsigned initial_align = 1) :
			alignment (initial_align),
			size (0)
		{}

		bool append (unsigned member_align, unsigned member_size) noexcept;

		void invalidate () noexcept
		{
			size = std::numeric_limits <unsigned>::max ();
		}

		bool is_valid () const noexcept
		{
			return size != std::numeric_limits <unsigned>::max ();
		}
	};

	static bool is_var_len (TypeCode::_ptr_type tc, SizeAndAlign& sa);

	struct ArrayTraits
	{
		TypeCode::_ref_type element_type;
		size_t element_count;

		ArrayTraits (ULong bound) :
			element_count (bound)
		{}
	};

	static void get_array_traits (TypeCode::_ptr_type content_tc, ArrayTraits& traits);

protected:
	RefCntProxy ref_cnt_;
	const TCKind kind_;
};

}
}

#endif
