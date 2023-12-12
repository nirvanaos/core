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
#ifndef NIRVANA_CORE_PROTDOMAIN_H_
#define NIRVANA_CORE_PROTDOMAIN_H_
#pragma once

#include "ORB/SysServantStatic.h"
#include <Nirvana/CoreDomains_s.h>
#include <Port/ProtDomain.h>
#include "Binder.h"
#include "ORB/Services.h"
#include "ORB/system_services.h"

namespace Nirvana {
namespace Core {

/// Protection domain.
class ProtDomain :
	public CORBA::Core::SysServantStaticImpl <ProtDomain, ProtDomainCore, Nirvana::ProtDomain>,
	private Port::ProtDomain
{
public:
	static IDL::String user ()
	{
		return Port::ProtDomain::user ();
	}

	static CORBA::Object::_ref_type bind (const IDL::String& name)
	{
		return Binder::bind (name);
	}

	static Nirvana::SysDomain::_ref_type sys_domain ()
	{
		return Nirvana::SysDomain::_narrow (
			CORBA::Core::Services::bind (CORBA::Core::Services::SysDomain));
	}

	static void shutdown (unsigned flags)
	{
		Scheduler::shutdown ();
	}

	static void request (const CORBA::OctetSeq& data_in, CORBA::OctetSeq& data_out)
	{
		throw_NO_IMPLEMENT ();
	}

	static void request_oneway (const CORBA::OctetSeq& data)
	{
		throw_NO_IMPLEMENT ();
	}

	static IDL::String binary_dir ();
};

}
}

#endif
