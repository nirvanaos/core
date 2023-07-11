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
#include "MemContextUser.h"
#include "NameService/FileSystem.h"
#include "ORB/Services.h"

namespace Nirvana {
namespace Core {

MemContextUser& MemContextUser::current ()
{
	MemContextUser* p = MemContext::current ().user_context ();
	if (!p)
		throw CORBA::NO_IMPLEMENT ();
	return *p;
}

MemContextUser::MemContextUser () :
	MemContext (true)
{}

MemContextUser::MemContextUser (Heap& heap) noexcept :
	MemContext (heap, true)
{}

MemContextUser::~MemContextUser ()
{}

RuntimeProxy::_ref_type MemContextUser::runtime_proxy_get (const void* obj)
{
	return runtime_support_.runtime_proxy_get (obj);
}

void MemContextUser::runtime_proxy_remove (const void* obj) noexcept
{
	runtime_support_.runtime_proxy_remove (obj);
}

void MemContextUser::on_object_construct (MemContextObject& obj) noexcept
{
	object_list_.push_back (obj);
}

void MemContextUser::on_object_destruct (MemContextObject& obj) noexcept
{
	// The object itself will remove from list. Nothing to do.
}

CosNaming::Name MemContextUser::get_current_dir_name () const
{
	if (!current_dir_.empty ())
		return current_dir_;
	else {
		// Default is "/home"
		CosNaming::Name home (2);
		home.back ().id ("home");
		return home;
	}
}

void MemContextUser::chdir (const IDL::String& path)
{
	if (path.empty ()) {
		current_dir_.clear ();
		return;
	}

	IDL::String translated;
	const IDL::String* ppath;
	if (Port::FileSystem::translate_path (path, translated))
		ppath = &translated;
	else
		ppath = &path;

	CosNaming::Name new_dir;
	if (!FileSystem::is_absolute (*ppath))
		new_dir = MemContextUser::get_current_dir_name ();
	FileSystem::get_name_from_path (new_dir, *ppath);

	// Check that new directory exists
	Dir::_ref_type dir = Dir::_narrow (
		CosNaming::NamingContext::_narrow (CORBA::Core::Services::bind (CORBA::Core::Services::NameService)
		)->resolve (new_dir));
	if (!dir)
		throw RuntimeError (ENOTDIR);

	if (dir->_non_existent ())
		throw RuntimeError (ENOENT);

	current_dir_ = std::move (new_dir);
}

}
}
