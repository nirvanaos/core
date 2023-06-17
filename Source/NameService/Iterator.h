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
#ifndef NIRVANA_NAMESERVICE_ITERATOR_H_
#define NIRVANA_NAMESERVICE_ITERATOR_H_
#pragma once

#include <CORBA/CORBA.h>
#include <CORBA/CosNaming.h>
#include "../UserObject.h"
#include <memory>

namespace CosNaming {
namespace Core {

/// \brief Pure virtual base for iterators
class NIRVANA_NOVTABLE Iterator :
	public Nirvana::Core::UserObject
{
public:
	virtual bool end () const noexcept = 0;
	bool next_one (CosNaming::Binding& b);
	bool next_n (uint32_t how_many, CosNaming::BindingList& bl);

	static CosNaming::BindingIterator::_ref_type create_iterator (std::unique_ptr <Iterator>&& vi);

protected:
	struct Binding
	{
		Istring name;
		BindingType type;

		Binding ()
		{}

		Binding (const Istring& n, BindingType t) :
			name (n), type (t)
		{}

		Binding (Istring&& n, BindingType t) :
			name (std::move (n)), type (t)
		{}
	};

	virtual bool next_one (Binding& b) = 0;
};

}
}

#endif
