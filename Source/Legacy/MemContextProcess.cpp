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
#include "MemContextProcess.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

using namespace Nirvana::Core;

RuntimeProxy::_ref_type MemContextProcess::runtime_proxy_get (const void* obj)
{
	if (!RUNTIME_SUPPORT_DISABLE) {
		std::lock_guard <MutexCore> lock (*this);
		return Base::runtime_proxy_get (obj);
	} else
		return nullptr;
}

void MemContextProcess::runtime_proxy_remove (const void* obj)
{
	if (!RUNTIME_SUPPORT_DISABLE) {
		lock ();
		Base::runtime_proxy_remove (obj);
		unlock ();
	}
}

void MemContextProcess::on_object_construct (MemContextObject& obj)
{
	lock ();
	Base::on_object_construct (obj);
	unlock ();
}

void MemContextProcess::on_object_destruct (MemContextObject& obj)
{
	lock ();
	obj.remove ();
	unlock ();
}

}
}
}
