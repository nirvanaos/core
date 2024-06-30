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
#ifndef NIRVANA_CORE_BINDER_INL_
#define NIRVANA_CORE_BINDER_INL_
#pragma once

#include "Binder.h"
#include "ClassLibrary.h"
#include "Singleton.h"
#include "Executable.h"

namespace Nirvana {
namespace Core {

inline
Main::_ptr_type Binder::bind (Executable& mod)
{
	const ModuleStartup* startup = nullptr;
	SYNC_BEGIN (singleton_->sync_domain_, nullptr);
	startup = singleton_->module_bind (mod._get_ptr (), mod.metadata (), nullptr);
	try {
		if (!startup || !startup->startup)
			invalid_metadata ();
		singleton_->binary_map_.add (mod);
	} catch (...) {
		release_imports (mod._get_ptr (), mod.metadata ());
		throw;
	}
	SYNC_END ();
	return Main::_check (startup->startup);
}

inline
void Binder::unbind (Executable& mod) noexcept
{
	SYNC_BEGIN (singleton_->sync_domain_, nullptr);
	singleton_->binary_map_.remove (mod);
	SYNC_END ();
	release_imports (mod._get_ptr (), mod.metadata ());
}

}
}

#endif
