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
#ifndef NIRVANA_CORE_CLASSLIBRARY_H_
#define NIRVANA_CORE_CLASSLIBRARY_H_

#include "Module.h"
#include "Binder.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

class ClassLibrary : public Module
{
public:
	ClassLibrary (const std::string& name) :
		Module (name, false)
	{
		entry_point_ = Binder::bind (*this);
	}

	~ClassLibrary ()
	{
		terminate (entry_point_);
	}

	void initialize (ModuleInit::_ptr_type entry_point)
	{
		ImplStatic <SyncDomain> init_sd;
		SYNC_BEGIN (&init_sd);
		ExecDomain* ed = Thread::current ().exec_domain ();
		assert (ed);
		ed->heap_replace (readonly_heap_);
		try {
			entry_point->initialize ();
		} catch (...) {
			ed->heap_restore ();
			throw;
		}
		ed->heap_restore ();
		SYNC_END ();
	}

	void terminate (ModuleInit::_ptr_type entry_point) NIRVANA_NOEXCEPT;

private:
	HeapUser readonly_heap_;
};

}
}

#endif
