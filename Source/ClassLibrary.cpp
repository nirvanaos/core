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

void ClassLibrary::initialize (ModuleInit::_ptr_type entry_point)
{
	ExecDomain& ed = ExecDomain::current ();
	ExecDomain::RestrictedMode rm = ed.restricted_mode ();

	ed.restricted_mode (ExecDomain::RestrictedMode::CLASS_LIBRARY_INIT);
	try {
		Module::initialize (entry_point);
	} catch (...) {
		ed.restricted_mode (rm);
		throw;
	}
	if (Port::Memory::FLAGS & Memory::ACCESS_CHECK) {

		// Make global data read-only

		if (mem_context_)
			mem_context_->heap ().change_protection (true);

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
	if (Port::Memory::FLAGS & Memory::ACCESS_CHECK) {
		try {

			// Make global data read-write

			if (mem_context_)
				mem_context_->heap ().change_protection (false);

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

Heap* ClassLibrary::stateless_memory ()
{
	return &mem_context_create ().heap ();
}

Module* ClassLibrary::module () noexcept
{
	return this;
}

void ClassLibrary::raise_exception (CORBA::SystemException::Code code, unsigned minor)
{
	Module::raise_exception (code, minor);
}

void ClassLibrary::atexit (AtExitFunc f)
{
	ExecDomain& ed = ExecDomain::current ();
	if (!mem_context_) {
		if (ed.restricted_mode () == ExecDomain::RestrictedMode::CLASS_LIBRARY_INIT)
			mem_context_ = &ed.mem_context ();
		else
			mem_context_ = MemContextUser::create ();
	}
	at_exit_.atexit (mem_context_->heap (), f);
}

void ClassLibrary::execute_atexit () noexcept
{
	at_exit_.execute ();
}

}
}
