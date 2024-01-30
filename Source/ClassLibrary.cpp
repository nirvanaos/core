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
#include "pch.h"
#include "ClassLibrary.h"

namespace Nirvana {
namespace Core {

void ClassLibrary::initialize (ModuleInit::_ptr_type entry_point, AtomicCounter <false>::IntegralType initial_ref_cnt)
{
	ExecDomain& ed = ExecDomain::current ();
	assert (MemContext::is_current (this));
	ExecDomain::RestrictedMode rm = ed.restricted_mode ();
	ed.restricted_mode (ExecDomain::RestrictedMode::CLASS_LIBRARY_INIT);
	try {
		Module::initialize (entry_point, initial_ref_cnt);
	} catch (...) {
		ed.restricted_mode (rm);
		throw;
	}
	if (Port::Memory::FLAGS & Memory::ACCESS_CHECK) {
		get_data_sections (data_sections_);
		heap ().change_protection (true);
		for (auto it = data_sections_.cbegin (); it != data_sections_.cend (); ++it) {
			void* p = const_cast <void*> (it->address);
			size_t cb = it->size;
			Port::Memory::copy (p, p, cb, Memory::READ_ONLY | Memory::EXACTLY);
		}
	}
	ed.restricted_mode (rm);
}

void ClassLibrary::terminate () noexcept
{
	assert (MemContext::is_current (this));
	if (Port::Memory::FLAGS & Memory::ACCESS_CHECK) {
		try {
			heap ().change_protection (false);
			for (auto it = data_sections_.cbegin (); it != data_sections_.cend (); ++it) {
				void* p = const_cast <void*> (it->address);
				size_t cb = it->size;
				Port::Memory::copy (p, p, cb, Memory::READ_WRITE | Memory::EXACTLY);
			}
		} catch (...) {
			// TODO: Log
			return;
		}
	}
	data_sections_.clear ();
	Module::terminate ();
}

SyncContext::Type ClassLibrary::sync_context_type () const noexcept
{
	switch (ExecDomain::current ().restricted_mode ()) {
	case ExecDomain::RestrictedMode::CLASS_LIBRARY_INIT:
		return FREE_MODULE_INIT;

	case ExecDomain::RestrictedMode::MODULE_TERMINATE:
		return FREE_MODULE_TERM;

	default:
		return FREE;
	}
}

Heap* ClassLibrary::stateless_memory () noexcept
{
	return &heap ();
}

Module* ClassLibrary::module () noexcept
{
	return this;
}

void ClassLibrary::raise_exception (CORBA::SystemException::Code code, unsigned minor)
{
	Module::raise_exception (code, minor);
}

}
}
