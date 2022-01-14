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
#include "ClassLibrary.h"

namespace Nirvana {
namespace Core {

void ClassLibrary::initialize (ModuleInit::_ptr_type entry_point)
{
	ExecDomain* ed = Thread::current ().exec_domain ();
	assert (ed);
	assert (MemContext::is_current (this));
	ed->restricted_mode_ = ExecDomain::RestrictedMode::CLASS_LIBRARY_INIT;
	try {
		Module::initialize (entry_point);
	} catch (...) {
		ed->restricted_mode_ = ExecDomain::RestrictedMode::NO_RESTRICTIONS;
		throw;
	}
	if (Port::Memory::FLAGS & Memory::ACCESS_CHECK) {
		get_data_sections (data_sections_);
		heap ().change_protection (true);
		/* Temporary disable until CRT will ready 
		for (auto it = data_sections_.cbegin (); it != data_sections_.cend (); ++it) {
			void* p = const_cast <void*> (it->address);
			Port::Memory::copy (p, p, it->size, Memory::READ_ONLY | Memory::EXACTLY);
		}*/
	}
	ed->restricted_mode_ = ExecDomain::RestrictedMode::NO_RESTRICTIONS;
}

void ClassLibrary::terminate () NIRVANA_NOEXCEPT
{
	ExecDomain* ed = Thread::current ().exec_domain ();
	assert (ed);
	assert (MemContext::is_current (this));
	if (Port::Memory::FLAGS & Memory::ACCESS_CHECK) {
		try {
			heap ().change_protection (false);
			/* Temporary disable until CRT will ready
				for (auto it = data_sections_.cbegin (); it != data_sections_.cend (); ++it) {
				void* p = const_cast <void*> (it->address);
				Port::Memory::copy (p, p, it->size, Memory::READ_WRITE | Memory::EXACTLY);
			}*/
		} catch (...) {
			// TODO: Log
			return;
		}
	}
	data_sections_.clear ();
	Module::terminate ();
}

}
}
