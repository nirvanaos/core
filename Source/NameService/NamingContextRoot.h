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
#ifndef NIRVANA_NAMESERVICE_NAMINGCONTEXTROOT_H_
#define NIRVANA_NAMESERVICE_NAMINGCONTEXTROOT_H_
#pragma once

#include <CORBA/CORBA.h>
#include <CORBA/CosNaming.h>
#include <memory>

namespace CosNaming {
namespace Core {

class Iterator;

/// \brief Root of the NamingContext class hierarchy
class NIRVANA_NOVTABLE NamingContextRoot
{
public:
	void list (uint32_t how_many, BindingList& bl, CosNaming::BindingIterator::_ref_type& bi) const;
	virtual std::unique_ptr <Iterator> make_iterator () const = 0;

	static Istring to_string (const NameComponent& nc);
	static Istring to_string (NameComponent&& nc);
	static NameComponent to_component (Istring s);
	static void check_name (const Name& n);

protected:
	NamingContextRoot (uint32_t signature = 0) :
		signature_ (signature)
	{}

private:
	static Istring escape (Istring s);
	static Istring unescape (Istring s);

protected:
	const uint32_t signature_;
};

}
}

#endif
