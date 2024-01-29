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
#ifndef NIRVANA_CORE_STARTUP_H_
#define NIRVANA_CORE_STARTUP_H_
#pragma once

#include <CORBA/CORBA.h>
#include "Runnable.h"
#include "ORB/SystemExceptionHolder.h"

namespace Nirvana {
namespace Core {

/// Startup runnable base.
class NIRVANA_NOVTABLE Startup : public Runnable
{
public:
	/// Called once by a host/kernel in a neutral execution context
	/// on the protection domain launch.
	/// All life in the domain started from this asynchronous call.
	/// 
	/// \param deadline Domain startup deadline.
	void launch (DeadlineTime deadline);

	virtual void run ();

	void check () const
	{
		exception_.check ();
	}

	int ret () const noexcept
	{
		return ret_;
	}

protected:
	Startup (int argc, char* argv [], char* envp []);

	~Startup ()
	{}

	bool initialize () noexcept;

	virtual void on_crash (const siginfo& signal) noexcept;
	void on_exception (const CORBA::Exception& ex) noexcept;

protected:
	int argc_;
	char** argv_;
	char** envp_;
	int ret_;

private:
	CORBA::Core::SystemExceptionHolder exception_;
};

}
}

#endif
