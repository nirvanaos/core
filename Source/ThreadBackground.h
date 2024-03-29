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
#ifndef NIRVANA_CORE_THREADBACKGROUND_H_
#define NIRVANA_CORE_THREADBACKGROUND_H_
#pragma once

#include "Heap.h"
#include <Port/ThreadBackground.h>

namespace Nirvana {
namespace Core {

class NIRVANA_NOVTABLE ThreadBackground :
	public SharedObject,
	public Port::ThreadBackground
{
	DECLARE_CORE_INTERFACE

	friend class Port::ThreadBackground;
	typedef Port::ThreadBackground Base;
public:
	static Ref <ThreadBackground> spawn ()
	{
		Ref <ThreadBackground> ref = Ref <ThreadBackground>::create <ImplDynamic <ThreadBackground> > ();
		ref->start ();
		return ref;
	}

	void execute (ExecDomain& exec_domain)
	{
		Thread::exec_domain (exec_domain);
		Base::resume ();
	}

	void finish () noexcept
	{
		Base::finish ();
	}

protected:
	ThreadBackground ()
	{}

	~ThreadBackground ()
	{}

	void start ();

	// Called from port.
	virtual void on_thread_proc_end () noexcept;

private:
	// Called from port.
	inline void execute () noexcept;

};

}
}

#endif
