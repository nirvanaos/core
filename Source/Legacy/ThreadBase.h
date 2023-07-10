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
#ifndef NIRVANA_LEGACY_CORE_THREADBASE_H_
#define NIRVANA_LEGACY_CORE_THREADBASE_H_
#pragma once

#include "../ThreadBackground.h"
#include "../RuntimeGlobal.h"
#include <Nirvana/SimpleList.h>
#include <memory>

namespace Nirvana {
namespace Legacy {
namespace Core {

class Process;

/// Legacy thread base.
class NIRVANA_NOVTABLE ThreadBase :
	public Nirvana::Core::ThreadBackground,
	public Nirvana::Core::UserObject,
	public SimpleList <ThreadBase>::Element,
	public Nirvana::Core::Runnable
{
	typedef Nirvana::Core::ThreadBackground Base;

public:
	using Nirvana::Core::UserObject::operator new;
	using Nirvana::Core::UserObject::operator delete;

	/// \returns Current thread if execution is in legacy mode.
	///          Otherwise returns `nullptr`.
	static ThreadBase* current_ptr () noexcept
	{
		if (Nirvana::Core::SyncContext::current ().is_legacy_mode ())
			return static_cast <ThreadBase*> (&Base::current ());
		else
			return nullptr;
	}

	/// When we run in the legacy subsystem, every thread is a ThreadLegacy instance.
	static ThreadBase& current () noexcept
	{
		ThreadBase* p = current_ptr ();
		assert (p);
		return *p;
	}

	virtual Process& process () noexcept = 0;

	Nirvana::Core::RuntimeGlobal& runtime_global () noexcept
	{
		return runtime_global_;
	}

protected:
	ThreadBase () = default;

private:
	Nirvana::Core::RuntimeGlobal runtime_global_;

};

}
}
}

#endif
