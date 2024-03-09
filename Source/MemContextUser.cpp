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
#include "pch.h"
#include "MemContextUser.h"
#include "BinderMemory.h"

namespace Nirvana {
namespace Core {

Ref <MemContext> MemContextUser::create (bool class_library_init)
{
	if (class_library_init)
		return MemContext::create <MemContextUser> (BinderMemory::heap (), create_heap (), MC_CLASS_LIBRARY);
	else
		return MemContext::create <MemContextUser> (create_heap (), MC_USER);
}

inline
MemContextUser::Data* MemContextUser::Data::create ()
{
	return new Data;
}

MemContextUser::Data::~Data ()
{}

MemContextUser& MemContextUser::current ()
{
	MemContextUser* p = MemContext::current ().user_context ();
	if (!p)
		throw_BAD_OPERATION (); // Unavailable for core contexts
	return *p;
}

MemContextUser::MemContextUser ()
{}

MemContextUser::Data& MemContextUser::data ()
{
	if (!data_)
		data_.reset (Data::create ());
	return *data_;
}

CosNaming::Name MemContextUser::get_current_dir_name () const
{
	CosNaming::Name cur_dir;
	if (data_)
		cur_dir = data_->current_dir ();
	if (cur_dir.empty ())
		cur_dir = Data::default_dir ();
	return cur_dir;
}

void MemContextUser::chdir (const IDL::String& path)
{
	data ().chdir (path);
}

}
}
