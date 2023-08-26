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
#ifndef NIRVANA_CORE_EVENTSYNC_H_
#define NIRVANA_CORE_EVENTSYNC_H_
#pragma once

#include <assert.h>

namespace Nirvana {
namespace Core {

class ExecDomain;

/// \brief Implements wait list for synchronized operations.
class EventSync
{
public:
	/// Constructor.
	/// 
	/// \param signalled Initial state. Default is not signalled.
	EventSync (bool signalled = false);
	
	~EventSync ()
	{
		assert (!wait_list_);
	}

	/// Suspend current execution domain until the event will be signalled.
	void wait ();

	/// Tests event state without wait.
/// \returns 'true' if object is already in the signalled state.
	bool signalled () const noexcept
	{
		return signalled_;
	}

	/// Sets object into the signalled state.
	void signal () noexcept;

	/// Reset object into the non-signalled state.
	void reset () noexcept
	{
		signalled_ = false;
	}

private:
	ExecDomain* wait_list_;
	bool signalled_;
};

}
}

#endif
