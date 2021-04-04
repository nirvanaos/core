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

#include "../core.h"
#include <CORBA/Server.h>
#include <CORBA/LifeCycleNoCopy.h>

namespace CORBA {
namespace Nirvana {
namespace Core {

/// \brief Core implementation of ServantBase and LocalObject.
template <class T, class I, class Proxy>
class CoreImpl :
	public Proxy,
	public LifeCycleNoCopy <T>,
	public ServantTraits <T>,
	public InterfaceImplBase <T, I>
{
public:
	using ServantTraits <T>::_implementation;
	using LifeCycleNoCopy <T>::__duplicate;
	using LifeCycleNoCopy <T>::__release;
	using Skeleton <T, I>::__non_existent;
	using ServantTraits <T>::_wide_object;

	template <class Base, class Derived>
	static Bridge <Base>* _wide (Bridge <Derived>* derived, String_in id, Interface* env)
	{
		return ServantTraits <T>::template _wide <Base, Derived> (derived, id, env);
	}

	template <>
	static Bridge <ReferenceCounter>* _wide <ReferenceCounter, I> (Bridge <I>* derived, String_in id, Interface* env)
	{
		return nullptr; // ReferenceCounter base is not implemented, return nullptr.
	}

	I_ptr <I> _get_ptr ()
	{
		return I_ptr <I> (&static_cast <I&> (static_cast <Bridge <I>&> (*this)));
	}

protected:
	template <class ... Args>
	CoreImpl (Args ... args) :
		Proxy (std::forward <Args> (args)...)
	{}
};

}
}
}

#endif
