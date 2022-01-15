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
#ifndef NIRVANA_LEGACY_CORE_PROCESS_H_
#define NIRVANA_LEGACY_CORE_PROCESS_H_
#pragma once

#include "ThreadBackground.h"
#include "CoreObject.h"
#include "MemContextProcess.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

/// Legacy process.
class NIRVANA_NOVTABLE Process :
	public ThreadBackground,
	public MemContextProcess
{
public:
	static Nirvana::Core::CoreRef <Process> spawn (Nirvana::Core::Runnable& runnable);

protected:
	Process () :
		ThreadBackground (true)
	{}

	~Process ()
	{}

	friend class Nirvana::Core::CoreRef <Process>;
	virtual void _add_ref () NIRVANA_NOEXCEPT = 0;
	virtual void _remove_ref () NIRVANA_NOEXCEPT = 0;
};

}
}
}

#endif
