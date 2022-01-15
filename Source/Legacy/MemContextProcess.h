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
#ifndef NIRVANA_LEGACY_CORE_MEMCONTEXTPROCESS_H_
#define NIRVANA_LEGACY_CORE_MEMCONTEXTPROCESS_H_
#pragma once

#include "../MemContextEx.h"
#include <mutex> // TODO: Replace with own implementation.

namespace Nirvana {
namespace Legacy {
namespace Core {

class NIRVANA_NOVTABLE MemContextProcess :
	public Nirvana::Core::MemContextEx
{
	typedef Nirvana::Core::MemContextEx Base;
public:
	virtual RuntimeProxy::_ref_type runtime_proxy_get (const void* obj);
	virtual void runtime_proxy_remove (const void* obj);

	virtual Memory::_ref_type create_heap (uint16_t granularity);

protected:
	MemContextProcess ();
	~MemContextProcess ();

private:
	std::mutex mutex_; // TODO Replace with own mutex!
};

}
}
}

#endif
