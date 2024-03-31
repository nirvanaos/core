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
#ifndef NIRVANA_CORE_DEADLINEPOLICYCONTEXT_H_
#define NIRVANA_CORE_DEADLINEPOLICYCONTEXT_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/System.h>

namespace Nirvana {
namespace Core {

class DeadlinePolicyContext
{
public:
	static const DeadlineTime ASYNC_DEFAULT = Nirvana::System::DEADLINE_POLICY_INHERIT;
	static const DeadlineTime ONEWAY_DEFAULT = Nirvana::System::DEADLINE_POLICY_INFINITE;

	DeadlinePolicyContext () :
		policy_async_ (ASYNC_DEFAULT),
		policy_oneway_ (ONEWAY_DEFAULT)
	{}

	const DeadlineTime& policy_async () const noexcept
	{
		return policy_async_;
	}

	void policy_async (const DeadlineTime& dp) noexcept
	{
		policy_async_ = dp;
	}

	const DeadlineTime& policy_oneway () const noexcept
	{
		return policy_oneway_;
	}

	void policy_oneway (const DeadlineTime& dp) noexcept
	{
		policy_oneway_ = dp;
	}

private:
	DeadlineTime policy_async_;
	DeadlineTime policy_oneway_;
};

}
}

#endif
