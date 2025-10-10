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
#include "NamingContextBase.h"

using namespace CORBA;

namespace CosNaming {
namespace Core {

void NamingContextBase::check_exist () const
{
	if (destroyed ())
		throw CORBA::OBJECT_NOT_EXIST ();
}

void NamingContextBase::check_name (const Name& n) const
{
	check_exist ();
	NamingContextRoot::check_name (n);
}

NamingContext::_ref_type NamingContextBase::resolve_child (Name& n)
{
	Object::_ref_type obj = resolve1 (n);
	NamingContext::_ref_type nc = NamingContext::_narrow (obj);
	if (!nc)
		throw NamingContext::NotFound (NamingContext::NotFoundReason::not_context, std::move (n));
	n.erase (n.begin ());
	return nc;
}

std::unique_ptr <Iterator> NamingContextBase::make_iterator () const
{
	std::unique_ptr <IteratorStack> iter (std::make_unique <IteratorStack> ());
	get_bindings (*iter);
	return iter;
}

}
}
