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

const char file_system_root [] = "/";

Object::_ref_type create_NameService ()
{
	if (ESIOP::is_system_domain ())
		return make_reference <NameService> ()->_this ();
	else
		return CORBA::Core::ORB::string_to_object (
			"corbaloc::1.1@/NameService", CORBA::Internal::RepIdOf <CosNaming::NamingContextExt>::id);
}

inline void NameService::check_no_fs (const Istring& name)
{
	if (name == file_system_root)
		throw NamingContext::AlreadyBound ();
}

void NameService::bind1 (Istring&& name, Object::_ptr_type obj, Name& n)
{
	check_no_fs (name);
	Base::bind1 (std::move (name), obj, n);
}

void NameService::rebind1 (Istring&& name, Object::_ptr_type obj, Name& n)
{
	check_no_fs (name);
	return Base::rebind1 (std::move (name), obj, n);
}

void NameService::bind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n)
{
	check_no_fs (name);
	Base::bind_context1 (std::move (name), nc, n);
}

void NameService::rebind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n)
{
	check_no_fs (name);
	Base::rebind_context1 (std::move (name), nc, n);
}

Object::_ref_type NameService::resolve1 (const Istring& name, BindingType& type, Name& n)
{
	if (name == file_system_root) {
		type = BindingType::ncontext;
		auto ins = bindings_.emplace (name, MapVal ());
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
		return Base::resolve1 (name, type, n);
}

NamingContext::_ref_type NameService::create_context1 (Istring&& name, Name& n, bool& created)
{
	check_no_fs (name);
	return Base::create_context1 (std::move (name), n, created);
}

NamingContext::_ref_type NameService::bind_new_context1 (Istring&& name, Name& n)
{
	check_no_fs (name);
	return Base::bind_new_context1 (std::move (name), n);
}

NameService::StringName NameService::escape (Istring s)
{
	for (size_t pos = 0; pos < s.size ();) {
		size_t esc = s.find_first_of ("/.\\", 0);
		if (esc != Istring::npos) {
			s.insert (esc, 1, '\\');
			pos = esc + 2;
		} else
			break;
	}
	return s;
}

Istring NameService::unescape (StringName s)
{
	for (size_t pos = 0; (pos = s.find ('\\', pos)) != Istring::npos;) {
		s.erase (pos, 1);
		++pos;
	}
	return s;
}

NameComponent NameService::escaped_to_component (StringName s)
{
	const size_t s_size = s.size ();
	size_t id_size = StringName::npos;
	for (;;) {
		id_size = s.rfind ('.', id_size);
		if (id_size != StringName::npos) {
			id_size = s_size;
			break;
		} else if (id_size == 0 || s [id_size - 1] != '\\')
			break;
		else
			id_size -= 2;
	}

	NameComponent nc;
	if (id_size > 0) {

		// A trailing '.' character is not permitted by the specification.
		if (id_size == s_size - 1)
			throw NamingContext::InvalidName ();

		if (id_size < s_size)
			nc.id (unescape (s.substr (0, id_size)));
		else
			nc.id (unescape (std::move (s)));
	}
	if (id_size < s_size) {
		if (id_size > 0)
			nc.kind (unescape (s.substr (id_size + 1)));
		else {
			s.erase (1, 1);
			nc.kind (unescape (std::move (s)));
		}
	}
	return nc;
}

NameService::StringName NameService::to_escaped (NameComponent&& nc)
{
	StringName name = escape (std::move (nc.id ()));
	if (!nc.kind ().empty ()) {
		name += '.';
		name += escape (std::move (nc.kind ()));
	}
	return name;
}

}
}
