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
#ifndef NIRVANA_CORE_MEMCONTEXTEX_H_
#define NIRVANA_CORE_MEMCONTEXTEX_H_
#pragma once

#include "MemContext.h"
#include "RuntimeSupport.h"
#include "MemContextObject.h"

namespace Nirvana {
namespace Core {

/// Memory context with runtime support
class NIRVANA_NOVTABLE MemContextRS : public MemContext
{
protected:
	/// Search map for runtime proxy for object \p obj.
	/// If proxy exists, returns it. Otherwise creates a new one.
	/// 
	/// \param obj Pointer used as a key.
	/// \returns RuntimeProxy for obj.
	virtual RuntimeProxy::_ref_type runtime_proxy_get (const void* obj);

	/// Remove runtime proxy for object \p obj.
	/// Implemented in MemContextEx.
	/// 
	/// \param obj Pointer used as a key.
	virtual void runtime_proxy_remove (const void* obj);

private:
	RuntimeSupport runtime_support_;
};

/// Memory context full implementation.
/// Unlike the base MemContext, MemContextEx is not thread-safe and can not be shared by multiple domains.
class NIRVANA_NOVTABLE MemContextEx : 
	public std::conditional <RUNTIME_SUPPORT_DISABLE, MemContext, MemContextRS>::type
{
public:
	/// Create MemContextEx object.
	/// 
	/// \returns MemContext reference.
	static CoreRef <MemContext> create ()
	{
		return CoreRef <MemContext>::create <ImplDynamic <MemContextEx>> ();
	}

protected:
	MemContextEx ();
	~MemContextEx ();

	/// Add object to list.
	/// 
	/// \param obj New object.
	virtual void on_object_construct (MemContextObject& obj);

	/// Remove object from list.
	/// 
	/// \param obj Object.
	virtual void on_object_destruct (MemContextObject& obj);

protected:
	SimpleList <MemContextObject> object_list_;
};

}
}

#endif
