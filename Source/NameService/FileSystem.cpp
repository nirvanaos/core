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
#include "FileSystem.h"
#include "IteratorStack.h"
#include "NameService.h"

using namespace CORBA;
using namespace PortableServer;
using CORBA::Core::Services;
using namespace CosNaming;

namespace Nirvana {
namespace Core {

const char FileSystem::adapter_name_ [] = "_fs";
StaticallyAllocated <POA::_ref_type> FileSystem::adapter_;

Object::_ref_type FileSystem::get_reference (const DirItemId& id, Internal::String_in iid)
{
	return adapter ()->create_reference_with_id (id, iid);
}

Nirvana::Dir::_ref_type FileSystem::get_dir (const DirItemId& id)
{
	assert (get_item_type (id) == Nirvana::DirItem::FileType::directory);
	Nirvana::Dir::_ref_type dir = Nirvana::Dir::_narrow (get_reference (id, Internal::RepIdOf <Nirvana::Dir>::id));
	assert (dir);
	return dir;
}

Object::_ref_type FileSystem::get_reference (const DirItemId& id)
{
	return get_reference (id, get_item_type (id) == Nirvana::DirItem::FileType::directory ?
		Internal::RepIdOf <Nirvana::Dir>::id : Internal::RepIdOf <Nirvana::File>::id);
}

void FileSystem::set_error_number (int err)
{
	ExecDomain::current ().runtime_global_.error_number = err;
}

Object::_ref_type FileSystem::resolve1 (Name& n)
{
	Nirvana::Dir::_ref_type dir = resolve_root (n);
	if (!dir)
		throw NotFound (NotFoundReason::missing_node, std::move (n));
	return dir;
}

Nirvana::Dir::_ref_type FileSystem::resolve_root (const Name& n)
{
	Nirvana::Dir::_ref_type dir;
	auto it = roots_.find (to_string (n.front ()));
	if (it != roots_.end ()) {
		if (it->second.cached)
			dir = it->second.cached;
		else {
			bool cache;
			dir = get_dir ((it->second.factory) (it->first, cache));
			if (cache)
				it->second.cached = dir;
		}
	}
	return dir;
}

void FileSystem::bind1 (Name& n, Object::_ptr_type obj)
{
	throw CannotProceed (_this (), std::move (n));
}

void FileSystem::rebind1 (Name& n, Object::_ptr_type obj)
{
	throw CannotProceed (_this (), std::move (n));
}

void FileSystem::bind_context1 (Name& n, NamingContext::_ptr_type nc)
{
	throw CannotProceed (_this (), std::move (n));
}

void FileSystem::rebind_context1 (Name& n, NamingContext::_ptr_type nc)
{
	throw CannotProceed (_this (), std::move (n));
}

void FileSystem::unbind1 (Name& n)
{
	throw CannotProceed (_this (), std::move (n));
}

NamingContext::_ref_type FileSystem::bind_new_context1 (Name& n)
{
	throw CannotProceed (_this (), std::move (n));
}

void FileSystem::get_bindings (CosNaming::Core::IteratorStack& iter) const
{
	iter.reserve (roots_.size ());
	for (const auto& root : roots_) {
		iter.push (root.first, BindingType::ncontext);
	}
}

Name FileSystem::get_name_from_path (const IDL::String& path)
{
	if (path.empty ())
		throw NamingContext::InvalidName ();
	if ('/' == path.front ()) {
		Name n = CosNaming::Core::NameService::to_name (path);
		assert (n.front ().id ().empty ());
		assert (n.front ().kind ().empty ());
		n.front ().id ("/");
		return n;
	} else
		return Port::FileSystem::get_name_from_path (path);
}

}
}
