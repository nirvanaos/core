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
#include "../pch.h"
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
	assert (!iid.empty ());
	return adapter ()->create_reference_with_id (id, iid);
}

Nirvana::Dir::_ref_type FileSystem::get_dir (const DirItemId& id)
{
	assert (get_item_type (id) == Nirvana::FileType::directory);
	Object::_ref_type obj = get_reference (id, Internal::RepIdOf <Nirvana::Dir>::id);
	assert (obj);
	Nirvana::Dir::_ref_type dir = Nirvana::Dir::_narrow (obj);
	assert (dir);
	return dir;
}

Nirvana::File::_ref_type FileSystem::get_file (const DirItemId& id)
{
	assert (get_item_type (id) != Nirvana::FileType::directory);
	Object::_ref_type obj = get_reference (id, CORBA::Internal::RepIdOf <Nirvana::File>::id);
	assert (obj);
	Nirvana::File::_ref_type file = Nirvana::File::_narrow (obj);
	assert (file);
	return file;
}

Nirvana::DirItem::_ref_type FileSystem::get_reference (const DirItemId& id)
{
	Object::_ref_type obj = get_reference (id, get_item_type (id) == Nirvana::FileType::directory ?
		Internal::RepIdOf <Nirvana::Dir>::id : Internal::RepIdOf <Nirvana::File>::id);
	assert (obj);
	Nirvana::DirItem::_ref_type item = Nirvana::DirItem::_narrow (obj);
	assert (item);
	return item;
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
	assert (!n.empty ());
	if (n.front ().kind ().empty ()) {
		auto it = roots_.find (n.front ().id ());
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
		iter.push (NameComponent (root.first, Istring ()), BindingType::ncontext);
	}
}

void FileSystem::append_path (CosNaming::Name& name, const IDL::String& path)
{
	if (path.empty ())
		return;

	typedef IDL::String::const_iterator It;
	typedef IDL::String::const_reverse_iterator Rit;

	It name_begin = path.begin ();
	It end = path.end ();
	if ('/' == *name_begin) {
		name.clear ();
		name.emplace_back ();
		++name_begin;
	}

	while (name_begin != end) {
		auto slash = std::find (name_begin, end, '/');
		if (slash == name_begin) {
			// Skip multiple slashes
			++name_begin;
		} else {
			if (slash - name_begin > 1 || '.' != *name_begin) {
				if (slash - name_begin == 2 && '.' == *name_begin && '.' == *(name_begin + 1)) {
					// Up one level
					if (name.size () <= 1)
						throw InvalidName ();
					name.pop_back ();
				} else {
					name.emplace_back ();
					NameComponent& nc = name.back ();
					Rit rbegin (name_begin);
					Rit rdot = std::find (Rit (slash), rbegin, '.');
					if (rdot != rbegin) {
						It dot (rdot.base () - 1);
						nc.id ().assign (name_begin, dot);
						nc.kind ().assign (dot + 1, slash);
					} else
						nc.id ().assign (name_begin, slash);
				}
			}
			if (slash != end)
				name_begin = slash + 1;
			else
				break;
		}
	}
}

bool FileSystem::is_absolute (const Name& n) noexcept
{
	if (n.empty ())
		return false;

	const NameComponent& nc = n.front ();
	return nc.id ().empty () && nc.kind ().empty ();
}

bool FileSystem::is_absolute (const IDL::String& path) noexcept
{
	return !path.empty () && path.front () == '/';
}

}
}
