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

#include "MemContextCore.h"

namespace Nirvana {
namespace Core {

MemContextCore::MemContextCore ()
{}

MemContextCore::MemContextCore (Heap& heap) noexcept :
	MemContext (heap)
{}

MemContextCore::~MemContextCore ()
{}

RuntimeProxy::_ref_type MemContextCore::runtime_proxy_get (const void* obj)
{
	return RuntimeProxy::_nil ();
}

void MemContextCore::runtime_proxy_remove (const void* obj) noexcept
{}

void MemContextCore::on_object_construct (MemContextObject& obj) noexcept
{}

void MemContextCore::on_object_destruct (MemContextObject& obj) noexcept
{}

CosNaming::Name MemContextCore::get_current_dir_name () const
{
	assert (false);
	throw CORBA::NO_IMPLEMENT ();
}

void MemContextCore::chdir (const IDL::String& path)
{
	assert (false);
	throw CORBA::NO_IMPLEMENT ();
}

}
}
