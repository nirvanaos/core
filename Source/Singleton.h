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
#ifndef NIRVANA_CORE_SINGLETON_H_
#define NIRVANA_CORE_SINGLETON_H_

#include "Module.h"
#include "Binder.h"

namespace Nirvana {
namespace Core {

class Singleton : public Module
{
public:
	Singleton (const std::string& name) :
		Module (name, true)
	{
		Binder::bind (*this);
	}

	~Singleton ()
	{
		assert (!bound ());
		terminate ();
		Binder::unbind (*this);
	}

	SyncDomain& sync_domain ()
	{
		return sync_domain_;
	}

	void initialize (ModuleInit::_ptr_type entry_point)
	{
		SYNC_BEGIN (&sync_domain ());
		Module::initialize (entry_point);
		SYNC_END ();
	}

	void terminate () NIRVANA_NOEXCEPT;

private:
	ImplStatic <SyncDomain> sync_domain_;
};

}
}

#endif
