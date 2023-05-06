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
#ifndef NIRVANA_ORB_CORE_CORESERVANT_H_
#define NIRVANA_ORB_CORE_CORESERVANT_H_
#pragma once

#include <CORBA/Server.h>
#include "ServantProxyBase.h"

namespace CORBA {
namespace Core {

/// \brief Core implementation of ServantBase and LocalObject.
/// 
/// \tparam S Servant implementation derived from this.
/// \tparam Proxy Proxy type.
template <class S, class Proxy>
class NIRVANA_NOVTABLE CoreServant :
	public Internal::ServantTraits <S>,
	public Internal::ValueImplBase <S, typename Proxy::ServantInterface>
{
public:
	typedef typename Proxy::ServantInterface PrimaryInterface;

	// Reference duplication is not allowed
	template <class I>
	static Internal::Interface* __duplicate (Internal::Interface* itf, Internal::Interface* env)
	{
		Internal::set_NO_IMPLEMENT (env);
		return nullptr;
	}

	// On release reference we delete the proxy
	template <class I>
	static void __release (Internal::Interface* itf)
	{
		S::_implementation (static_cast <Internal::Bridge <I>*> (itf)).proxy ().delete_proxy ();
	}

	Internal::I_ptr <PrimaryInterface> _get_ptr () NIRVANA_NOEXCEPT
	{
		return Internal::I_ptr <PrimaryInterface> (&static_cast <PrimaryInterface&> (
			static_cast <Internal::Bridge <PrimaryInterface>&> (*this)));
	}

	Proxy& proxy () NIRVANA_NOEXCEPT
	{
		return proxy_;
	}

	// Implementation of the servant object life cycle

	static void __delete_object (Internal::Bridge <PrimaryInterface>* obj, Internal::Interface* env)
	{
		assert (false);
	}

	void _add_ref () NIRVANA_NOEXCEPT
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () NIRVANA_NOEXCEPT
	{
		if (0 == ref_cnt_.decrement_seq ()) {
			try {
				proxy_.servant ()->_delete_object ();
			} catch (...) {
				assert (false); // TODO: Swallow exception or log
			}
		}
	}

	ULong _refcount_value () const NIRVANA_NOEXCEPT
	{
		Nirvana::Core::RefCounter::IntegralType ucnt = ref_cnt_;
		return ucnt > std::numeric_limits <ULong>::max () ? std::numeric_limits <ULong>::max () : (ULong)ucnt;
	}

	Internal::Interface* _query_interface (const IDL::String& type_id) const
	{
		return proxy_._query_interface (type_id);
	}

protected:
	CoreServant (Proxy& proxy) :
		proxy_ (proxy)
	{}

private:
	Nirvana::Core::RefCounter ref_cnt_;
	Proxy& proxy_;
};

}
}

#endif
