/// \file
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
#pragma once

#include "Binary.h"
#include <Nirvana/ModuleInit.h>
#include "AtomicCounter.h"
#include "MemContextUser.h"
#include "Chrono.h"

namespace Nirvana {
namespace Core {

class SyncContext;

/// Loadable module
class NIRVANA_NOVTABLE Module :
	public UserObject,
	public Binary,
	public MemContextUser,
	public CORBA::servant_traits <Nirvana::Module>::Servant <Module>,
	public CORBA::Internal::LifeCycleRefCnt <Module>
{
public:
	/// Derived ClassLibrary and Singleton classes must have virtual destructors.
	virtual ~Module ()
	{}

	/// \returns Synchronization context.
	///   If module is ClassLibrary, returned context is free context.
	///   If module is Singleton, returned context is the singleton synchronization domain.
	virtual SyncContext& sync_context () NIRVANA_NOEXCEPT = 0;

	/// Called on the module static object _add_ref().
	void _add_ref () NIRVANA_NOEXCEPT
	{
		ref_cnt_.increment ();
	}

	/// Called on the module static object _remove_ref().
	void _remove_ref () NIRVANA_NOEXCEPT;

	/// \returns Current reference count.
	AtomicCounter <false>::IntegralType _refcount_value () const NIRVANA_NOEXCEPT
	{
		return ref_cnt_;
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

	virtual void initialize (ModuleInit::_ptr_type entry_point, AtomicCounter <false>::IntegralType initial_ref_cnt);
	virtual void terminate () NIRVANA_NOEXCEPT;

	void raise_exception (CORBA::SystemException::Code code, unsigned minor);

	using UserObject::operator delete;
	using UserObject::operator new;

protected:
	Module (const StringView& name, bool singleton);

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
