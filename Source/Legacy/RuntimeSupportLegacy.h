/// \file
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
#ifndef NIRVANA_LEGACY_CORE_RUNTIMESUPPORTLEGACY_H_
#define NIRVANA_LEGACY_CORE_RUNTIMESUPPORTLEGACY_H_

#include "../RuntimeSupportImpl.h"
#include <mutex> // TODO: Replace with own implementation.

namespace Nirvana {
namespace Legacy {
namespace Core {

class RuntimeSupportLegacy :
	public Nirvana::Core::RuntimeSupportImpl
{
public:
	RuntimeProxy::_ref_type runtime_proxy_get (const void* obj)
	{
		std::lock_guard <std::mutex> lock (mutex_);
		return Nirvana::Core::RuntimeSupportImpl::runtime_proxy_get (obj);
	}

	void runtime_proxy_remove (const void* obj)
	{
		std::lock_guard <std::mutex> lock (mutex_);
		Nirvana::Core::RuntimeSupportImpl::runtime_proxy_remove (obj);
	}

private:
	std::mutex mutex_;
};

}
}
}

#endif
