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
#include <Nirvana/SimpleList.h>
#include "../TLS.h"

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

	/// When we run in the legacy subsystem, every thread is a ThreadLegacy instance.
	static ThreadBase& current () NIRVANA_NOEXCEPT
	{
		Nirvana::Core::Thread& base = Base::current ();
		return static_cast <ThreadBase&> (base);
	}

	Nirvana::Core::TLS& get_TLS () NIRVANA_NOEXCEPT
	{
		return TLS_;
	}

	virtual Process& process () NIRVANA_NOEXCEPT = 0;

protected:
	ThreadBase () = default;

private:
	Nirvana::Core::TLS TLS_;
};

}
}
}

#endif
