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
#include "Module.h"
#include "Binder.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

Module::Module (AccessDirect::_ptr_type file, bool singleton) :
	Binary (file),
	entry_point_ (nullptr),
	ref_cnt_ (0),
	initial_ref_cnt_ (0),
	singleton_ (singleton)
{}

void Module::_remove_ref () noexcept
{
	if (ref_cnt_.decrement_seq () == initial_ref_cnt_) {
		if (ref_cnt_.increment_seq () == initial_ref_cnt_ + 1) {
			release_time_ = Chrono::steady_clock ();
			ref_cnt_.decrement ();
		}
	}
}

void Module::initialize (ModuleInit::_ptr_type entry_point)
{
	if (entry_point) {
		entry_point->initialize ();
		entry_point_ = entry_point;
	}
}

void Module::terminate () noexcept
{
	if (entry_point_) {
		try {
			entry_point_->terminate ();
		} catch (...) {
			// TODO: Log
		}
		entry_point_ = nullptr;
	}
}

void Module::raise_exception (CORBA::SystemException::Code code, unsigned minor)
{
	CORBA::Internal::Bridge <ModuleInit>* br = static_cast <CORBA::Internal::Bridge <ModuleInit>*> (&entry_point_);
	if (br)
		br->_epv ().epv.raise_exception (br, (short)code, (unsigned short)minor, nullptr);
}

}
}
