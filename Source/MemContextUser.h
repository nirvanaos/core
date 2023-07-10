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

class MemContextObject;

/// \brief Memory context full implementation.
class MemContextUser : public MemContext
{
	friend class CORBA::servant_reference <MemContext>;

public:
	/// Create MemContextUser object.
	/// 
	/// \returns MemContext reference.
	static Ref <MemContext> create ()
	{
		return Ref <MemContext>::create <MemContextUser> ();
	}

	/// Search map for runtime proxy for object \p obj.
	/// If proxy exists, returns it. Otherwise creates a new one.
	/// 
	/// \param obj Pointer used as a key.
	/// \returns RuntimeProxy for obj.
	virtual RuntimeProxy::_ref_type runtime_proxy_get (const void* obj);

	/// Remove runtime proxy for object \p obj.
	/// 
	/// \param obj Pointer used as a key.
	virtual void runtime_proxy_remove (const void* obj) noexcept;

	/// Add object to list.
	/// 
	/// \param obj New object.
	virtual void on_object_construct (MemContextObject& obj) noexcept;

	/// Remove object from list.
	/// 
	/// \param obj Object.
	virtual void on_object_destruct (MemContextObject& obj) noexcept;

	virtual CosNaming::Name get_current_dir_name () const;
	virtual void chdir (const IDL::String& path);

protected:
	MemContextUser ();
	MemContextUser (Heap& heap) noexcept;
	~MemContextUser ();

	void clear () noexcept
	{
		current_dir_.clear ();
		object_list_.clear ();
		runtime_support_.clear ();
	}

private:
	RuntimeSupportImpl runtime_support_;
	MemContextObjectList object_list_;
	CosNaming::Name current_dir_;
};

inline MemContextUser* MemContext::user_context () noexcept
{
	if (user_)
		return static_cast <MemContextUser*> (this);
	else
		return nullptr;
}

}
}

#endif
