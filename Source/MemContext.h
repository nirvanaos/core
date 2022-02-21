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
#ifndef NIRVANA_CORE_MEMCONTEXT_H_
#define NIRVANA_CORE_MEMCONTEXT_H_
#pragma once

#include "HeapUser.h"
#include "CoreInterface.h"

namespace Nirvana {
namespace Core {

class MemContextObject;
class TLS;

/// Memory context.
/// Contains heap and some other stuff.
class NIRVANA_NOVTABLE MemContext
{
	DECLARE_CORE_INTERFACE

	MemContext (const MemContext&) = delete;
	MemContext& operator = (const MemContext&) = delete;

public:
	/// \returns Current memory context.
	static MemContext& current ();

	static bool is_current (MemContext* context);

	/// \returns User heap.
	Heap& heap () NIRVANA_NOEXCEPT
	{
		return heap_;
	}

	/// Search map for runtime proxy for object \p obj.
	/// If proxy exists, returns it. Otherwise creates a new one.
	/// 
	/// \param obj Pointer used as a key.
	/// \returns RuntimeProxy for obj.
	virtual RuntimeProxy::_ref_type runtime_proxy_get (const void* obj) = 0;

	/// Remove runtime proxy for object \p obj.
	/// 
	/// \param obj Pointer used as a key.
	virtual void runtime_proxy_remove (const void* obj) NIRVANA_NOEXCEPT = 0;

	/// Add object to list.
	/// 
	/// \param obj New object.
	virtual void on_object_construct (MemContextObject& obj) NIRVANA_NOEXCEPT = 0;

	/// Remove object from list.
	/// 
	/// \param obj Object.
	virtual void on_object_destruct (MemContextObject& obj) NIRVANA_NOEXCEPT = 0;

	/// \return Reference to TLS.
	virtual TLS& get_TLS () NIRVANA_NOEXCEPT = 0;

	/// Global class initialization.
	static void initialize ();

	/// Global class temination.
	static void terminate () NIRVANA_NOEXCEPT;

protected:
	MemContext ();
	~MemContext ();

	void clear () NIRVANA_NOEXCEPT
	{
		heap_.cleanup ();
		// TODO: Detect and log memory leaks
	}

protected:
	HeapUser heap_;
};

}
}

#endif
