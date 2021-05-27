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
#ifndef NIRVANA_CORE_MODULE_H_
#define NIRVANA_CORE_MODULE_H_

#include <CORBA/Server.h>
#include <generated/Module_s.h>
#include <Nirvana/ModuleInit.h>
#include "AtomicCounter.h"
#include <Port/Module.h>
#include "CoreObject.h"
#include "Chrono.h"

namespace Nirvana {
namespace Core {

class NIRVANA_NOVTABLE Module :
	public CoreObject,
	public Port::Module,
	public CORBA::Internal::Servant <Module, ::Nirvana::Module>,
	public CORBA::Internal::LifeCycleRefCnt <Module>
{
public:
	virtual ~Module ()
	{}

	const void* base_address () const
	{
		return Port::Module::address ();
	}

	void _add_ref () NIRVANA_NOEXCEPT
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () NIRVANA_NOEXCEPT
	{
		if (ref_cnt_.decrement () == initial_ref_cnt_) {
			if (ref_cnt_.increment () == initial_ref_cnt_ + 1) {
				release_time_ = Chrono::steady_clock ();
				ref_cnt_.decrement ();
			}
		}
	}

	/// \returns `true` if module is singleton library.
	bool singleton () const
	{
		return singleton_;
	}

	/// If bound () returns `false`, the module may be safely unloaded.
	/// 
	/// \returns `true` if some objects of this module are bound from other modules.
	bool bound () const
	{
		return initial_ref_cnt_ < ref_cnt_;
	}

	bool can_be_unloaded (const Chrono::Duration& t) const
	{
		return !bound () && release_time_ <= t;
	}

protected:
	Module (const std::string& name, bool singleton);

	void initialize (ModuleInit::_ptr_type entry_point)
	{
		initial_ref_cnt_ = ref_cnt_;
		entry_point->initialize ();
		entry_point_ = entry_point;
	}

	void terminate () NIRVANA_NOEXCEPT;

protected:
	bool singleton_;
	AtomicCounter <false> ref_cnt_;
	AtomicCounter <false>::IntegralType initial_ref_cnt_;
	ModuleInit::_ptr_type entry_point_;
	Chrono::Duration release_time_;
};

}
}

#endif
