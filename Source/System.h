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
#ifndef NIRVANA_CORE_SYSTEM_H_
#define NIRVANA_CORE_SYSTEM_H_

#include <Nirvana/Nirvana.h>
#include <Nirvana/System_s.h>
#include "ExecDomain.h"
#include <Port/SystemInfo.h>
#include "NameService/NameService.h"
#include "TLS.h"

namespace Nirvana {

class Static_the_system :
	public CORBA::servant_traits <System>::ServantStatic <Static_the_system>
{
public:
	static uint32_t _s__get_hardware_concurrency (CORBA::Internal::Bridge <Nirvana::System>* _b, CORBA::Internal::Interface* _env)
	{
		return Core::Port::SystemInfo::hardware_concurrency ();
	}

	static ContextType context_type ()
	{
		return (ContextType)Core::SyncContext::current ().sync_context_type ();
	}

	static uint16_t TLS_alloc (Deleter deleter)
	{
		return Core::TLS::allocate (deleter);
	}

	static void TLS_free (uint16_t idx)
	{
		Core::TLS::release (idx);
	}

	static void TLS_set (uint16_t idx, void* ptr)
	{
		Core::TLS::set (idx, ptr);
	}

	static void* TLS_get (uint16_t idx)
	{
		return Core::TLS::get (idx);
	}

	static IDL::String to_string (const CosNaming::Name& name)
	{
		CosNaming::Core::NamingContextRoot::check_name (name);
		return CosNaming::Core::NameService::to_string_unchecked (name);
	}

	static CosNaming::Name to_name (const IDL::String& sn)
	{
		return CosNaming::Core::NameService::to_name (sn);
	}

	static size_t exec_domain_id ()
	{
		return Core::ExecDomain::current ().id ();
	}
};

}

#endif
