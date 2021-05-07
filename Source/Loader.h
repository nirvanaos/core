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
#ifndef NIRVANA_CORE_LOADER_H_
#define NIRVANA_CORE_LOADER_H_

#include <Nirvana/Nirvana.h>
#include "WaitList.h"
#include "Module.h"
#include "SyncDomain.h"
#include "Synchronized.h"
#include "parallel-hashmap/parallel_hashmap/phmap.h"

namespace Nirvana {
namespace Core {

class Loader
{
public:
	static CoreRef <Module> load (const std::string& name, bool singleton);

private:
	typedef phmap::flat_hash_map <CoreString, WaitableRef <Module>, phmap::Hash <CoreString>, phmap::EqualTo <CoreString>, 
		CoreAllocator <std::pair <CoreString, WaitableRef <Module> > > > Map;

	ImplStatic <SyncDomain> sync_domain_;
	Map map_;

	static Loader singleton_;
};

}
}

#endif
