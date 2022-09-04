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
#ifndef NIRVANA_ORB_CORE_COREIMPL_H_
#define NIRVANA_ORB_CORE_COREIMPL_H_
#pragma once

#include <CORBA/Server.h>
#include "ServantProxyBase.h"
#include "LifeCycleNoCopy.h"

namespace CORBA {
namespace Core {

/// \brief Core implementation of ServantBase and LocalObject.
/// 
/// \tparam S Servant implementation derived from this.
/// \tparam Proxy Proxy type.
template <class S, class Proxy>
class NIRVANA_NOVTABLE CoreServant :
	public LifeCycleNoCopy <S>,
	public Internal::ServantTraits <S>,
	public Internal::ValueImplBase <S, typename Proxy::ServantInterface>
{
public:
	typedef typename Proxy::ServantInterface PrimaryInterface;

	Internal::I_ptr <PrimaryInterface> _get_ptr () NIRVANA_NOEXCEPT
	{
		return Internal::I_ptr <PrimaryInterface> (&static_cast <PrimaryInterface&> (
			static_cast <Internal::Bridge <PrimaryInterface>&> (*this)));
	}

	Proxy& proxy () NIRVANA_NOEXCEPT
	{
		return proxy_;
	}

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

	virtual ~CoreServant ()
	{}

private:
	Nirvana::Core::RefCounter ref_cnt_;
	Proxy& proxy_;
};

/// \brief Core implementation of ServantBase and LocalObject.
template <class T, class I, class Proxy>
class CoreImpl :
	public Proxy,
	public LifeCycleNoCopy <T>,
	public Internal::ServantTraits <T>,
	public Internal::ValueImplBase <T, I>
{
public:
	typedef I PrimaryInterface;

	using Internal::ServantTraits <T>::_implementation;
	using Internal::ServantTraits <T>::_wide_object;
	using LifeCycleNoCopy <T>::__duplicate;
	using LifeCycleNoCopy <T>::__release;
	using Internal::Skeleton <T, I>::__non_existent;
	using Internal::Skeleton <T, I>::__query_interface;

	template <class Base, class Derived>
	static Internal::Bridge <Base>* _wide (Internal::Bridge <Derived>* derived, Internal::Type <IDL::String>::ABI_in id, Internal::Interface* env)
	{
		return Internal::ServantTraits <T>::template _wide <Base, Derived> (derived, id, env);
	}

	Internal::I_ptr <I> _get_ptr () NIRVANA_NOEXCEPT
	{
		return Internal::I_ptr <I> (&static_cast <I&> (static_cast <Internal::Bridge <I>&> (*this)));
	}

	static void __add_ref (Internal::Bridge <I>* obj, Internal::Interface* env)
	{
		try {
			T::_implementation (obj).add_ref_servant ();
		} catch (Exception& e) {
			set_exception (env, e);
		} catch (...) {
			set_unknown_exception (env);
		}
	}

	static void __remove_ref (Internal::Bridge <I>* obj, Internal::Interface* env)
	{
		try {
			T::_implementation (obj).remove_ref_servant ();
		} catch (Exception& e) {
			set_exception (env, e);
		} catch (...) {
			set_unknown_exception (env);
		}
	}

	static ULong __refcount_value (Internal::Bridge <I>* obj, Internal::Interface* env)
	{
		try {
			return T::_implementation (obj).refcount_servant ();
		} catch (Exception& e) {
			set_exception (env, e);
		} catch (...) {
			set_unknown_exception (env);
		}
		return 0;
	}

	static void __delete_object (Internal::Bridge <I>* obj, Internal::Interface* env)
	{}

	void add_ref_servant () NIRVANA_NOEXCEPT
	{
		ref_cnt_servant_.increment ();
	}

	void remove_ref_servant () NIRVANA_NOEXCEPT
	{
		if (0 == ref_cnt_servant_.decrement ()) {
			assert (&Nirvana::Core::SyncContext::current () == &ServantProxyBase::sync_context ());
			try {
				Proxy::servant ()->_delete_object ();
			} catch (...) {
				assert (false); // TODO: Swallow exception or log
			}
		}
	}

	ULong refcount_servant () NIRVANA_NOEXCEPT
	{
		Nirvana::Core::RefCounter::IntegralType ucnt = ref_cnt_servant_;
		return ucnt > std::numeric_limits <ULong>::max () ? std::numeric_limits <ULong>::max () : (ULong)ucnt;
	}

protected:
	template <class S>
	CoreImpl (S servant) :
		Proxy (servant)
	{}

private:
	Nirvana::Core::RefCounter ref_cnt_servant_;
};

}
}

#endif
