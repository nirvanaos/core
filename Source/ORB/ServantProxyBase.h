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
#ifndef NIRVANA_ORB_CORE_SERVANTPROXYBASE_H_
#define NIRVANA_ORB_CORE_SERVANTPROXYBASE_H_
#pragma once

#include <CORBA/Server.h>
#include "../ExecDomain.h"
#include "../Chrono.h"
#include "ProxyManager.h"
#include "RefCntProxy.h"
#include "offset_ptr.h"

namespace CORBA {
namespace Core {

/// \brief Base for server-side proxies.
class ServantProxyBase :
	public ProxyManager
{
	class GC;
public:
	virtual Internal::IORequest::_ref_type create_request (OperationIndex op, UShort flags) override;

	/// Returns synchronization context for the target object.
	Nirvana::Core::SyncContext& sync_context () const NIRVANA_NOEXCEPT
	{
		return *sync_context_;
	}

	/// Returns synchronization context for the specific operation.
	/// For some Object operations may return free context.
	virtual Nirvana::Core::SyncContext& get_sync_context (Internal::IOReference::OperationIndex op)
		const NIRVANA_NOEXCEPT override
	{
		if (ProxyManager::is_object_op (op))
			return ProxyManager::get_sync_context (op);
		return *sync_context_;
	}

	Nirvana::Core::MemContext* memory () const NIRVANA_NOEXCEPT;

	virtual void _add_ref () override;
	virtual void _remove_ref () NIRVANA_NOEXCEPT override;

	RefCntProxy::IntegralType _refcount_value () const NIRVANA_NOEXCEPT
	{
		return ref_cnt_.load ();
	}

protected:
	template <class I>
	ServantProxyBase (Internal::I_ptr <I> servant) :
		ProxyManager (get_primary_interface_id (servant)),
		ref_cnt_ (0),
		sync_context_ (&Nirvana::Core::SyncContext::current ())
	{
		size_t offset = offset_ptr ();
		servant_ = offset_ptr (static_cast <Internal::Interface::_ptr_type> (servant), offset);
		// Fill implementation pointers
		for (InterfaceEntry* ie = interfaces ().begin (); ie != interfaces ().end (); ++ie) {
			if (!ie->implementation) {
				Internal::Interface::_ptr_type impl = servant->_query_interface (ie->iid);
				if (!impl)
					throw OBJ_ADAPTER (); // Implementation not found. TODO: Log
				ie->implementation = offset_ptr (impl, offset);
			}
		}
	}

	~ServantProxyBase ();

	Internal::Interface::_ptr_type servant () const NIRVANA_NOEXCEPT
	{
		return servant_;
	}

	virtual void add_ref_1 ();

	RefCntProxy::IntegralType remove_ref_proxy () NIRVANA_NOEXCEPT;

	inline void run_garbage_collector () const NIRVANA_NOEXCEPT;

	static void collect_garbage (Internal::Interface::_ptr_type servant) NIRVANA_NOEXCEPT;

private:
	template <class I>
	static const Char* get_primary_interface_id (Internal::I_ptr <I> servant)
	{
		Internal::Interface::_ptr_type primary = servant->_query_interface (nullptr);
		if (!primary)
			throw OBJ_ADAPTER (); // TODO: Log
		return primary->_epv ().interface_id;
	}

private:
	Internal::Interface::_ptr_type servant_;
	RefCntProxy ref_cnt_;
	Nirvana::Core::CoreRef <Nirvana::Core::SyncContext> sync_context_;
};

}
}

#endif
