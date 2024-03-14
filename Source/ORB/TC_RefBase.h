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
#ifndef NIRVANA_ORB_CORE_TC_REFBASE_H_
#define NIRVANA_ORB_CORE_TC_REFBASE_H_
#pragma once

#include "TC_IdName.h"

namespace CORBA {
namespace Core {

class NIRVANA_NOVTABLE TC_RefBase :
	public TC_IdName
{
public:
	TC_RefBase (TCKind kind, IDL::String&& id, IDL::String&& name) noexcept :
		TC_IdName (kind, std::move (id), std::move (name))
	{}

	static size_t _s_n_size (Internal::Bridge <TypeCode>*, Internal::Interface*);
	static size_t _s_n_align (Internal::Bridge <TypeCode>*, Internal::Interface*);

	static void n_construct (void* p)
	{
		Internal::check_pointer (p);
		*reinterpret_cast <Internal::Interface**> (p) = nullptr;
	}

	static void n_destruct (void* p)
	{
		Internal::check_pointer (p);
		Internal::Interface** itf = reinterpret_cast <Internal::Interface**> (p);
		Internal::interface_release (*itf);
		*itf = nullptr;
	}

	void n_copy (void* dst, const void* src) const
	{
		Internal::check_pointer (dst);
		Internal::check_pointer (src);
		Internal::Interface** idst = reinterpret_cast <Internal::Interface**> (dst);
		Internal::Interface* const* isrc = reinterpret_cast <Internal::Interface* const*> (src);
		Internal::Interface* itf = Internal::interface_duplicate (Internal::Interface::_check (*isrc, id_));
		Internal::interface_release (*idst);
		*idst = itf;
	}

	void n_move (void* dst, void* src) const
	{
		Internal::check_pointer (dst);
		Internal::check_pointer (src);
		Internal::Interface** idst = reinterpret_cast <Internal::Interface**> (dst);
		Internal::Interface** isrc = reinterpret_cast <Internal::Interface**> (src);
		Internal::Interface* itf = Internal::Interface::_check (*isrc, id_);
		Internal::interface_release (*idst);
		*idst = itf;
		*isrc = nullptr;
	}
};

}
}

#endif
