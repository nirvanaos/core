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
#ifndef NIRVANA_ORB_CORE_SYSSERVANTSTATIC_H_
#define NIRVANA_ORB_CORE_SYSSERVANTSTATIC_H_
#pragma once

#include "SysObject.h"
#include "SysObjectKey.h"

namespace CORBA {
namespace Core {

//! Static implementation of system object
//! \tparam S Servant class.
template <class S>
class SysServantStatic :
	public Internal::InterfaceStaticBase <S, CORBA::LocalObject>,
	public Internal::ServantTraitsStatic <S>,
	public Internal::LifeCycleStatic,
	public Internal::LocalObjectStaticDummy
{
public:
	static Internal::Bridge <Object>* _get_object (Internal::Type <IDL::String>::ABI_in iid,
		Internal::Interface* env) noexcept
	{
		return Internal::get_object_from_core (core_object (), iid, env);
	}

	using LocalObjectStaticDummy::__add_ref;
	using LocalObjectStaticDummy::__remove_ref;
	using LocalObjectStaticDummy::__refcount_value;
	using LocalObjectStaticDummy::__delete_object;

	static void initialize (const Octet* id, size_t id_len)
	{
		SysObject* sys_obj = SysObject::create (&static_cast <CORBA::LocalObject&> (
			*Internal::InterfaceStaticBase <S, CORBA::LocalObject>::_bridge ()), id, id_len);
		core_object_ = static_cast <CORBA::LocalObject*> (&CORBA::LocalObject::_ptr_type (sys_obj));
	}

	static void terminate () noexcept
	{
		Internal::interface_release (core_object_);
	}

protected:
	static Internal::Interface::_ref_type _get_proxy ()
	{
		return get_proxy (core_object ());
	}

private:
	static CORBA::LocalObject::_ptr_type core_object () noexcept
	{
		return core_object_;
	}

public:
	// We can't use `static const` here, because it causes the redundant optimization in CLang.
	static CORBA::LocalObject* core_object_;
};

template <class S> CORBA::LocalObject* SysServantStatic <S>::core_object_;

template <class S, class Primary, class ... Bases>
class SysServantStaticImpl :
	public Internal::InterfaceStatic <S, Primary>,
	public Internal::InterfaceStatic <S, Bases>...,
	public SysServantStatic <S>
{
public:
	typedef Primary PrimaryInterface;

	Internal::Interface* _query_interface (const IDL::String& iid)
	{
		return Internal::FindInterface <Primary, Bases...>::find (*(S*)0, iid);
	}

	static Internal::I_ref <Primary> _this ()
	{
		return SysServantStatic <S>::_get_proxy ().template downcast <Primary> ();
	}

	static void initialize ()
	{
		SysServantStatic <S>::initialize (SysObjectKey <S>::key, std::size (SysObjectKey <S>::key));
	}
};

}
}

#endif
