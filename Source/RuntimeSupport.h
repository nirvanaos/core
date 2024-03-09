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
#ifndef NIRVANA_CORE_RUNTIMESUPPORT_H_
#define NIRVANA_CORE_RUNTIMESUPPORT_H_
#pragma once

#include "MemContextUser.h"

namespace Nirvana {
namespace Core {

class RuntimeSupportReal
{
public:
	/// Search map for runtime proxy for object \p obj.
	/// If proxy exists, returns it. Otherwise creates a new one.
	/// 
	/// \param obj Pointer used as a key.
	/// \returns RuntimeProxy for obj.
	///   May return nil reference if RuntimeSupport is disabled.
	static RuntimeProxy::_ref_type proxy_get (const void* obj)
	{
		MemContextUser* mc = MemContext::current ().user_context ();
		if (mc)
			return static_cast <RuntimeSupportContext&> (*mc).proxy_get (obj);
		else
			return nullptr;
	}

	/// Remove runtime proxy for object \p obj.
	/// 
	/// \param obj Pointer used as a key.
	static void proxy_remove (const void* obj) noexcept
	{
		MemContextUser* mc = MemContext::current ().user_context ();
		if (mc)
			static_cast <RuntimeSupportContext&> (*mc).proxy_remove (obj);
	}
};

typedef std::conditional <RUNTIME_SUPPORT_DISABLE, RuntimeSupportFake, RuntimeSupportReal>::type RuntimeSupport;

}
}

#endif
