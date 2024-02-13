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
#include "pch.h"
#include "SysDomain.h"
#include "ORB/ORB.h"
#include "SysManager.h"

using namespace CORBA;

namespace Nirvana {
namespace Core {

inline
SysDomain::SysDomain () :
	packages_ (*this),
	fs_locator_ (*this)
{
	// Parent component
	Object::_ptr_type comp = _object ();

	// Create stateful
	SYNC_BEGIN (g_core_free_sync_context, nullptr)
	manager_ = make_reference <SysManager> (comp)->_this ();
	SYNC_END ()
}

SysDomain::~SysDomain ()
{}

Object::_ref_type create_SysDomain ()
{
	if (ESIOP::is_system_domain ())
		return make_stateless <SysDomain> ()->_this ();
	else
		return CORBA::Core::ORB::string_to_object (
			"corbaloc::1.1@/%00", CORBA::Internal::RepIdOf <Nirvana::SysDomain>::id);
}

SysDomain& SysDomain::implementation ()
{
	assert (ESIOP::is_system_domain ());
	Object::_ref_type obj = CORBA::Core::Services::bind (CORBA::Core::Services::SysDomain);
	return *static_cast <SysDomain*> (get_implementation (obj));
}

}
}
