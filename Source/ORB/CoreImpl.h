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
namespace Internal {
namespace Core {

/// \brief Core implementation of ServantBase and LocalObject.
template <class T, class I, class Proxy>
class CoreImpl :
	public Proxy,
	public LifeCycleNoCopy <T>,
	public ServantTraits <T>,
	public ValueImplBase <T, I>
{
public:
	typedef I PrimaryInterface;

	using ServantTraits <T>::_implementation;
	using LifeCycleNoCopy <T>::__duplicate;
	using LifeCycleNoCopy <T>::__release;
	using Skeleton <T, I>::__non_existent;
	using ServantTraits <T>::_wide_object;
	using Skeleton <T, I>::__query_interface;

	template <class Base, class Derived>
	static Bridge <Base>* _wide (Bridge <Derived>* derived, String_in id, Interface* env)
	{
		return ServantTraits <T>::template _wide <Base, Derived> (derived, id, env);
	}

	I_ptr <I> _get_ptr () NIRVANA_NOEXCEPT
	{
		return I_ptr <I> (&static_cast <I&> (static_cast <Bridge <I>&> (*this)));
	}

	virtual void _remove_ref () NIRVANA_NOEXCEPT
	{
		if (!ServantProxyBase::ref_cnt_servant_.decrement ()) {
			assert (&Nirvana::Core::SyncContext::current () == &ServantProxyBase::sync_context ());
			try {
				Proxy::servant ()->_delete_object ();
			} catch (...) {
				assert (false); // TODO: Swallow exception or log
			}
		}
	}

protected:
	template <class S>
	CoreImpl (S servant) :
		Proxy (servant)
	{}
};

}
}
}

#endif
