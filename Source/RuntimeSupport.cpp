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

#include "RuntimeSupport.h"

namespace Nirvana {
namespace Core {

RuntimeProxy::_ref_type RuntimeSupport::runtime_proxy_get (const void* obj)
{
	auto ins = proxy_map_.emplace (obj, nullptr);
	if (ins.second) {
		try {
			ins.first->second = CoreRef <RuntimeProxyImpl>::template create <RuntimeProxyImpl> (obj);
		} catch (...) {
			proxy_map_.erase (ins.first);
			throw;
		}
	}
	return ins.first->second->_get_ptr ();
}

void RuntimeSupport::runtime_proxy_remove (const void* obj) NIRVANA_NOEXCEPT
{
	auto f = proxy_map_.find (obj);
	if (f != proxy_map_.end ()) {
		f->second->remove ();
		proxy_map_.erase (f);
	}
}

}
}
