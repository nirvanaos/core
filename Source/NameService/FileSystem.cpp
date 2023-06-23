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

Nirvana::File::_ref_type FileSystem::get_file (const DirItemId& id)
{
	assert (get_item_type (id) != Nirvana::DirItem::FileType::directory);
	Nirvana::File::_ref_type file = Nirvana::File::_narrow (get_reference (id, Internal::RepIdOf <Nirvana::File>::id));
	assert (file);
	return file;
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

Object::_ref_type FileSystem::resolve1 (const Istring& name, BindingType& type, Name& n)
{
	Nirvana::Dir::_ref_type dir = resolve_root (name);
	if (dir) {
		type = BindingType::ncontext;
		return dir;
	} else
		throw NotFound (NotFoundReason::missing_node, std::move (n));
}

void FileSystem::bind1 (Istring&& name, Object::_ptr_type obj, Name& n)
{
	throw CannotProceed (_this (), std::move (n));
}

void FileSystem::rebind1 (Istring&& name, Object::_ptr_type obj, Name& n)
{
	throw CannotProceed (_this (), std::move (n));
}

void FileSystem::bind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n)
{
	throw CannotProceed (_this (), std::move (n));
}

void FileSystem::rebind_context1 (Istring&& name, NamingContext::_ptr_type nc, Name& n)
{
	throw CannotProceed (_this (), std::move (n));
}

void FileSystem::unbind1 (const Istring& name, Name& n)
{
	throw CannotProceed (_this (), std::move (n));
}

NamingContext::_ref_type FileSystem::create_context1 (Istring&& name, Name& n, bool& created)
{
	throw CannotProceed (_this (), std::move (n));
}

NamingContext::_ref_type FileSystem::bind_new_context1 (Istring&& name, Name& n)
{
	throw CannotProceed (_this (), std::move (n));
}

std::unique_ptr <CosNaming::Core::Iterator> FileSystem::make_iterator () const
{
	std::unique_ptr <CosNaming::Core::IteratorStack> it (std::make_unique <CosNaming::Core::IteratorStack> ());
	it->reserve (roots_.size ());
	for (const auto& root : roots_) {
		it->push (root.first, BindingType::ncontext);
	}
	return it;
}

}
}
