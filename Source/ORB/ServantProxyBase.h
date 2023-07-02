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

namespace CORBA {
namespace Core {

/// \brief Base for server-side proxies.
class NIRVANA_NOVTABLE ServantProxyBase :
	public ProxyManager
{
	class GC;
public:
	virtual Internal::IORequest::_ref_type create_request (OperationIndex op, unsigned flags,
		Internal::RequestCallback::_ptr_type callback) override;

	/// Returns synchronization context for the target object.
	Nirvana::Core::SyncContext& sync_context () const noexcept
	{
		return *sync_context_;
	}

	/// Returns synchronization context for the specific operation.
	/// For some Object operations may return free context.
	virtual Nirvana::Core::SyncContext& get_sync_context (Internal::IOReference::OperationIndex op)
		const noexcept override
	{
		if (ProxyManager::is_object_op (op))
			return ProxyManager::get_sync_context (op);
		return *sync_context_;
	}

	Nirvana::Core::MemContext* memory () const noexcept;

#ifdef _DEBUG
	RefCntProxy::IntegralType _refcount_value () const noexcept
	{
		return ref_cnt_.load ();
	}
#endif

	virtual void _remove_ref () override;

	void delete_proxy () noexcept
	{
		// If ref_cnt_ is not zero here, the servant implementation has a bug
		// with _add_ref()/_remove_ref() calls.
		assert (!ref_cnt_.load ());
		if (!ref_cnt_.load ())
			delete this;
	}

	void reset_servant () noexcept;

	Internal::Interface::_ptr_type servant () const noexcept
	{
		return servant_;
	}

protected:
	template <class I>
	ServantProxyBase (Internal::I_ptr <I> servant) :
		ProxyManager (get_primary_interface_id (servant), true),
		ref_cnt_ (0),
		sync_context_ (&Nirvana::Core::SyncContext::current ())
	{
		size_t offset = offset_ptr ();
		servant_ = offset_ptr (static_cast <Internal::Interface::_ptr_type> (servant), offset);
		set_servant (servant, offset);
	}

	virtual ~ServantProxyBase ();

	void run_garbage_collector () const noexcept;

	void add_ref_servant () const;

	static void collect_garbage (Internal::Interface::_ptr_type servant) noexcept;

private:
	template <class I>
	static const Char* get_primary_interface_id (Internal::I_ptr <I> servant)
	{
		Internal::Interface::_ptr_type primary = servant->_query_interface (nullptr);
		if (!primary)
			throw OBJ_ADAPTER (); // TODO: Log
		return primary->_epv ().interface_id;
	}

protected:
	RefCntProxy ref_cnt_;
	Internal::Interface::_ptr_type servant_;
	servant_reference <Nirvana::Core::SyncContext> sync_context_;
};

inline
const CORBA::Core::ServantProxyBase* object2proxy_base (CORBA::Object::_ptr_type obj) noexcept
{
	return static_cast <const ServantProxyBase*> (ProxyManager::cast (obj));
}

}
}

#endif
