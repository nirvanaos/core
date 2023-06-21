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

}
}
