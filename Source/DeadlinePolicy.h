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
#ifndef NIRVANA_CORE_DEADLINEPOLICY_H_
#define NIRVANA_CORE_DEADLINEPOLICY_H_
#pragma once

#include "MemContext.h"

namespace Nirvana {
namespace Core {

class DeadlinePolicy
{
public:
	static const DeadlineTime& policy_async () noexcept
	{
		MemContext* mc = MemContext::current_ptr ();
		if (mc) {
			const DeadlinePolicyContext* ctx = MemContext::current ().deadline_policy_ptr ();
			if (ctx)
				return ctx->policy_async ();
		}
		return DeadlinePolicyContext::ASYNC_DEFAULT;
	}

	static void policy_async (const DeadlineTime& dp)
	{
		MemContext::current ().deadline_policy ().policy_async (dp);
	}

	static const DeadlineTime& policy_oneway () noexcept
	{
		MemContext* mc = MemContext::current_ptr ();
		if (mc) {
			const DeadlinePolicyContext* ctx = MemContext::current ().deadline_policy_ptr ();
			if (ctx)
				return ctx->policy_oneway ();
		}
		return DeadlinePolicyContext::ONEWAY_DEFAULT;
	}

	static void policy_oneway (const DeadlineTime& dp)
	{
		MemContext::current ().deadline_policy ().policy_oneway (dp);
	}
};

}
}

#endif
