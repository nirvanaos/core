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
#ifndef NIRVANA_LEGACY_CORE_THREADBACKGROUND_H_
#define NIRVANA_LEGACY_CORE_THREADBACKGROUND_H_
#pragma once

#include "../ThreadBackground.h"
#include <Nirvana/SimpleList.h>
#include "Process.h"
#include "../TLS.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

/// Background thread.
class NIRVANA_NOVTABLE ThreadLegacy :
	public Nirvana::Core::ThreadBackground,
	public SimpleList <ThreadLegacy>::Element
{
	typedef Nirvana::Core::ThreadBackground Base;
public:
	/// When we run in the legacy subsystem, every thread is a ThreadLegacy instance.
	static ThreadLegacy& current () NIRVANA_NOEXCEPT
	{
		Nirvana::Core::Thread& base = Base::current ();
		assert (base.is_legacy ());
		return static_cast <ThreadLegacy&> (base);
	}

	static Nirvana::Core::CoreRef <ThreadLegacy> create (Process& process, Nirvana::Core::ExecDomain& ed)
	{
		using namespace Nirvana::Core;
		CoreRef <ThreadLegacy> obj = CoreRef <ThreadLegacy>::create <
			ImplDynamic <ThreadLegacy> > (std::ref (process), std::ref (ed));

		process.on_thread_start (*obj);

		obj->start ();
		return obj;
	}

	virtual bool is_legacy () const NIRVANA_NOEXCEPT
	{
		return true;
	}

	Process& process () const NIRVANA_NOEXCEPT
	{
		return *process_;
	}

	Nirvana::Core::TLS& get_TLS () NIRVANA_NOEXCEPT
	{
		return TLS_;
	}

	void finish () NIRVANA_NOEXCEPT
	{
		TLS_.clear ();
		process_->on_thread_finish (*this);
	}

protected:
	ThreadLegacy (Process& process, Nirvana::Core::ExecDomain& ed) :
		Base (ed),
		process_ (&process)
	{}

private:
	void on_thread_proc_end ()
	{
		_remove_ref ();
	}

private:
	Nirvana::Core::CoreRef <Process> process_;
	Nirvana::Core::TLS TLS_;
};

}
}
}

#endif
