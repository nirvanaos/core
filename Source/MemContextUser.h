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

#include "MemContextCore.h"
#include "RuntimeSupport.h"
#include "MemContextObject.h"

namespace Nirvana {
namespace Core {

/// \brief Memory context full implementation.
class MemContextUser : public MemContextCore
{
	typedef MemContextCore Base;

	friend class Nirvana::Core::Ref <MemContext>;

public:
	/// Create MemContextUser object.
	/// 
	/// \returns MemContext reference.
	static Ref <MemContext> create ()
	{
		return Ref <MemContext>::create <MemContextUser> ();
	}

protected:
	MemContextUser ();
	~MemContextUser ();

private:
	// MemContext methods

	virtual RuntimeProxy::_ref_type runtime_proxy_get (const void* obj) override;
	virtual void runtime_proxy_remove (const void* obj) noexcept override;
	virtual void on_object_construct (MemContextObject& obj) noexcept override;
	virtual void on_object_destruct (MemContextObject& obj) noexcept override;

protected:
	RuntimeSupportImpl runtime_support_;
	MemContextObjectList object_list_;
};

}
}

#endif
