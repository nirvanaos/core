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
#include "NameService.h"
#include "../ORB/ESIOP.h"
#include "../ORB/ORB.h"
#include "../ORB/ServantProxyObject.h"
#include "FileSystem.h"

using namespace CORBA;

namespace CosNaming {
namespace Core {

Object::_ref_type create_NameService ()
{
	if (ESIOP::is_system_domain ())
		return make_reference <NameService> ()->_this ();
	else
		return CORBA::Core::ORB::string_to_object (
			"corbaloc::1.1@/NameService", CORBA::Internal::RepIdOf <CosNaming::NamingContextExt>::id);
}

bool NameService::is_file_system (const NameComponent& nc)
{
	return nc.id () == "/" && nc.kind ().empty ();
}

inline void NameService::check_no_fs (const Name& n)
{
	if (is_file_system (n.front ()))
		throw NamingContext::AlreadyBound ();
}

void NameService::bind1 (Name& n, Object::_ptr_type obj)
{
	check_no_fs (n);
	Base::bind1 (n, obj);
}

void NameService::rebind1 (Name& n, Object::_ptr_type obj)
{
	check_no_fs (n);
	return Base::rebind1 (n, obj);
}

void NameService::bind_context1 (Name& n, NamingContext::_ptr_type nc)
{
	check_no_fs (n);
	Base::bind_context1 (n, nc);
}

void NameService::rebind_context1 (Name& n, NamingContext::_ptr_type nc)
{
	check_no_fs (n);
	Base::rebind_context1 (n, nc);
}

Object::_ref_type NameService::resolve1 (Name& n)
{
	assert (!n.empty ());
	if (is_file_system (n.front ())) {
		auto ins = bindings_.emplace (Base::to_string (n.front ()), MapVal ());
		if (ins.second) {
			try {
				ins.first->second.binding_type = BindingType::ncontext;
				ins.first->second.object = make_reference <Nirvana::Core::FileSystem> ()->_this ();
			} catch (...) {
				bindings_.erase (ins.first);
				throw;
			}
		}
		return ins.first->second.object;
	} else
		return Base::resolve1 (n);
}

NamingContext::_ref_type NameService::bind_new_context1 (Name& n)
{
	check_no_fs (n);
	return Base::bind_new_context1 (n);
}

Name NameService::to_name (const StringName& sn)
{
	Name n;
	size_t begin = 0;
	for (size_t end = 0; (end = sn.find ('/', end)) != StringName::npos;) {
		if (end > 0 && sn [end - 1] == '\\') { // Escaped, search next
			++end;
			continue;
		}

		n.push_back (to_component (sn.substr (begin, end - begin)));
		begin = ++end;
	}
	n.push_back (to_component (sn.substr (begin)));
	return n;
}

}
}
