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
#include "Legacy/Executable.h"
#include "ORB/DomainProt.h"
#include "ORB/DomainRemote.h"

namespace Nirvana {
namespace Core {

inline
Legacy::Main::_ptr_type Binder::bind (Legacy::Core::Executable& mod)
{
	SYNC_BEGIN (singleton_->sync_domain_, nullptr);
	const ModuleStartup* startup = singleton_->module_bind (mod._get_ptr (), mod.metadata (), nullptr);
	try {
		if (!startup || !startup->startup)
			invalid_metadata ();
		return Legacy::Main::_check (startup->startup);
	} catch (...) {
		release_imports (mod._get_ptr (), mod.metadata ());
		throw;
	}
	SYNC_END ();
}

inline
void Binder::unbind (Legacy::Core::Executable& mod) NIRVANA_NOEXCEPT
{
	release_imports (mod._get_ptr (), mod.metadata ());
}

}
}

#endif
