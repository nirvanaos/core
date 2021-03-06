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
#include "Module.h"
#include "Binder.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

Module::Module (const StringView& name, bool singleton) :
	Binary (name),
	singleton_ (singleton),
	ref_cnt_ (0),
	initial_ref_cnt_ (0),
	entry_point_ (nullptr)
{}

void Module::_remove_ref () NIRVANA_NOEXCEPT
{
	if (ref_cnt_.decrement () == initial_ref_cnt_) {
		if (ref_cnt_.increment () == initial_ref_cnt_ + 1) {
			release_time_ = Chrono::steady_clock ();
			ref_cnt_.decrement ();
		}
	}
}

void Module::initialize (ModuleInit::_ptr_type entry_point, AtomicCounter <false>::IntegralType initial_ref_cnt)
{
	initial_ref_cnt_ = initial_ref_cnt;
	if (entry_point) {
		entry_point->initialize ();
		entry_point_ = entry_point;
	}
}

void Module::terminate () NIRVANA_NOEXCEPT
{
	if (entry_point_) {
		ExecDomain& ed = ExecDomain::current ();
		ed.restricted_mode (ExecDomain::RestrictedMode::MODULE_TERMINATE);
		try {
			entry_point_->terminate ();
		} catch (...) {
			// TODO: Log
		}
		ed.restricted_mode (ExecDomain::RestrictedMode::NO_RESTRICTIONS);
	}
}

void Module::raise_exception (CORBA::SystemException::Code code, unsigned minor)
{
	CORBA::Internal::Bridge <ModuleInit>* br = static_cast <CORBA::Internal::Bridge <ModuleInit>*> (&entry_point_);
	br->_epv ().epv.raise_exception (br, (short)code, (unsigned short)minor, nullptr);
}

}
}
