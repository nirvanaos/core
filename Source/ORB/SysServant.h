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
#ifndef NIRVANA_ORB_CORE_SYSSERVANT_H_
#define NIRVANA_ORB_CORE_SYSSERVANT_H_
#pragma once

#include "SysObject.h"
#include <CORBA/I_var.h>

namespace CORBA {
namespace Core {

class SysObjectLink : public Internal::LocalObjectLink
{
protected:
	SysObjectLink (const Internal::Bridge <CORBA::LocalObject>::EPV& epv) :
		Internal::LocalObjectLink (epv)
	{
	}

	void _construct (Octet id)
	{
		core_object_ = Internal::I_var <CORBA::LocalObject> (SysObject::create (
			&static_cast <CORBA::LocalObject&> (static_cast <Internal::Bridge <CORBA::LocalObject>&> (*this)), id));
	}
};

//! \brief Implementation of system object
//! \tparam S Servant class implementing operations.
template <class S>
class SysServant :
	public Internal::Skeleton <S, CORBA::LocalObject>,
	public Internal::LifeCycleRefCnt <S>,
	public Internal::ServantTraits <S>,
	public SysObjectLink
{
public:
	void _delete_object () NIRVANA_NOEXCEPT
	{
		delete& static_cast <S&> (*this);
	}

	static Bridge <AbstractBase>* _get_abstract_base (Internal::Type <IDL::String>::ABI_in iid,
		Internal::Interface* env) NIRVANA_NOEXCEPT
	{
		assert (false);
		return nullptr;
	}

protected:
	SysServant () :
		SysObjectLink (Internal::Skeleton <S, CORBA::LocalObject>::epv_)
	{}

	SysServant (const SysServant&) = delete;

	SysServant& operator = (const SysServant&) NIRVANA_NOEXCEPT
	{
		return *this; // Do nothing
	}

};

template <class S, Octet id, class Primary, class ... Bases>
class SysServantImpl :
	public Internal::InterfaceImpl <S, Primary>,
	public Internal::InterfaceImpl <S, Bases>...,
	public SysServant <S>,
	public Internal::ServantMemory
{
public:
	typedef Primary PrimaryInterface;

	Internal::Interface* _query_interface (const IDL::String& id)
	{
		return Internal::FindInterface <Primary, Bases...>::find (static_cast <S&> (*this), id);
	}

	Internal::I_ref <Primary> _this () const
	{
		return SysObjectLink::_get_proxy ().template downcast <Primary> ();
	}

protected:
	SysServantImpl ()
	{
		SysObjectLink::_construct (id);
	}
};

}
}

#endif
