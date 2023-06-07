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
#ifndef NIRVANA_CORE_SYNCCONTEXT_H_
#define NIRVANA_CORE_SYNCCONTEXT_H_
#pragma once

#include "CoreInterface.h"
#include "Thread.h"
#include "StaticallyAllocated.h"

namespace Nirvana {
namespace Core {

class Heap;
class ExecDomain;
class SyncDomain;
class MemContext;
class Module;

/// Synchronization context pure interface.
/// Synchronization context may be associated with:
///   - Synchronization domain
///   - Free synchronization context for core stateless objects
///   - Free synchronization context for class library
///   - Legacy::Executable
class NIRVANA_NOVTABLE SyncContext
{
DECLARE_CORE_INTERFACE
public:
	/// \returns Current synchronization context.
	static SyncContext& current () NIRVANA_NOEXCEPT;

	/// \returns SyncDomain associated with this context.
	/// Returns `nullptr` if there is not synchronization domain.
	virtual SyncDomain* sync_domain () NIRVANA_NOEXCEPT;

	/// \returns `true` if this context is free sync context.
	/// May be used for proxy optimization.
	/// When we marshal `in` parameters from free context we haven't to copy data
	/// because all data are in stack or the execution domain heap and can not be changed
	/// by other threads during the synchronous call.
	bool is_free_sync_context () const NIRVANA_NOEXCEPT
	{
		return const_cast <SyncContext&> (*this).stateless_memory () != nullptr;
	}

	/// \returns Free sync context returns pointer to the stateless objects heap.
	///          Other sync contexts return `nullptr`.
	virtual Heap* stateless_memory () NIRVANA_NOEXCEPT;

	/// Returns the Module object associated with this context.
	/// 
	/// \returns Pointer to the Module object.
	///          Core and legacy executable contexts return `nullptr`.
	virtual Module* module () NIRVANA_NOEXCEPT = 0;

	/// Raise system exception in the binary object (Module or Executable).
	/// 
	/// \code  System exception code.
	/// \minor System exception minor code.
	virtual void raise_exception (CORBA::SystemException::Code code, unsigned minor) = 0;
};

/// Free (not synchronized) sync context.
class NIRVANA_NOVTABLE SyncContextFree :
	public SyncContext
{
public:
	virtual Heap* stateless_memory () NIRVANA_NOEXCEPT = 0;
};

/// Core free sync context.
class SyncContextCore :
	public SyncContextFree
{
public:
	virtual Heap* stateless_memory () NIRVANA_NOEXCEPT;
	virtual Module* module () NIRVANA_NOEXCEPT;
	virtual void raise_exception (CORBA::SystemException::Code code, unsigned minor);
};

extern StaticallyAllocated <ImplStatic <SyncContextCore> > g_core_free_sync_context;

}
}

#endif
