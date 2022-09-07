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
#ifndef NIRVANA_CORE_EVENT_H_
#define NIRVANA_CORE_EVENT_H_
#pragma once

#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

/// Implements wait list for asynchronous operations.
class Event :
	private Stack <ExecDomain>
{
public:
	/// Suspend current execution domain until the event will be signalled.
	void wait () NIRVANA_NOEXCEPT {
		if (!signalled_)
			run_in_neutral_context (wait_op_);
	}

	/// Tests event state without wait.
	/// \returns 'true' if object is already in the signalled state.
	bool signalled () const NIRVANA_NOEXCEPT
	{
		return signalled_;
	}

	/// Sets object into the signalled state.
	void signal () NIRVANA_NOEXCEPT;

protected:
	Event () NIRVANA_NOEXCEPT :
		wait_op_ (std::ref (*this)),
		signalled_ (false)
	{}

	~Event ()
	{
		assert (signalled_);
	}

private:
	class WaitOp : public Runnable
	{
	public:
		WaitOp (Event& obj) :
			obj_ (obj)
		{}

	private:
		virtual void run ();

	private:
		Event& obj_;
	};

	ImplStatic <WaitOp> wait_op_;
	volatile bool signalled_;
};

}
}

#endif
