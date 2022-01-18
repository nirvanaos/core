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
#include "MemContextEx.h"

namespace Nirvana {
namespace Core {

MemContextEx::MemContextEx ()
{}

MemContextEx::~MemContextEx ()
{
	object_list_.clear ();
}

RuntimeProxy::_ref_type MemContextEx::runtime_proxy_get (const void* obj)
{
	return runtime_support_.runtime_proxy_get (obj);
}

void MemContextEx::runtime_proxy_remove (const void* obj)
{
	runtime_support_.runtime_proxy_remove (obj);
}

void MemContextEx::on_object_construct (MemContextObject& obj)
{
	object_list_.push_back (obj);
}

void MemContextEx::on_object_destruct (MemContextObject& obj)
{
}

}
}
