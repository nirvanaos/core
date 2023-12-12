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
#ifndef NIRVANA_LEGACY_CORE_EXECUTABLE_H_
#define NIRVANA_LEGACY_CORE_EXECUTABLE_H_
#pragma once

#include <CORBA/Server.h>
#include "../SyncContext.h"
#include "../Binary.h"
#include <Nirvana/Module_s.h>
#include <Nirvana/Legacy/Main.h>
#include "../ORB/LifeCycleStack.h"
#include "AtExit.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

class Executable :
	public Nirvana::Core::Binary,
	public Nirvana::Core::SyncContext,
	public CORBA::servant_traits <Nirvana::Module>::Servant <Executable>,
	public CORBA::Core::LifeCycleStack
{
public:
	Executable (AccessDirect::_ptr_type file);
	~Executable ();

	int main (int argc, char* argv [], char* envp [])
	{
		return (int)entry_point_->main (argc, argv, envp);
	}

	void cleanup ()
	{
		entry_point_->cleanup ();
	}

	virtual Nirvana::Core::Module* module () noexcept;
	virtual void raise_exception (CORBA::SystemException::Code code, unsigned minor);

	void unbind () noexcept;

	void atexit (AtExitFunc f)
	{
		at_exit_.atexit (Nirvana::Core::MemContext::current ().heap (), f);
	}

	void execute_atexit (Nirvana::Core::Heap& heap)
	{
		at_exit_.execute (heap);
	}

private:
	Main::_ptr_type entry_point_;
	Nirvana::Core::AtExit at_exit_;
	bool bound_;
};

}
}
}

#endif
