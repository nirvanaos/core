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
#ifndef NIRVANA_CORE_SYSDOMAIN_H_
#define NIRVANA_CORE_SYSDOMAIN_H_
#pragma once

#include <CORBA/Server.h>
#include "SharedObject.h"
#include <Port/SysDomain.h>
#include "Nirvana/Domains_s.h"

namespace Nirvana {
namespace Core {

/// System domain.
class SysDomain :
	public CORBA::servant_traits <Nirvana::SysDomain>::Servant <SysDomain>,
	private Port::SysDomain
{
public:
	Port::SysDomain& port ()
	{
		return *this;
	}

	void get_bind_info (const std::string& obj_name, BindInfo& bind_info)
	{
		if (obj_name == "Nirvana/g_dec_calc")
			bind_info.module_name ("DecCalc.olf");
		else
			bind_info.module_name ("TestModule.olf");
	}

	class ProtDomainInfo :
		private Port::SysDomain::ProtDomainInfo
	{
	public:
		Port::SysDomain::ProtDomainInfo& port ()
		{
			return *this;
		}

		template <typename T>
		ProtDomainInfo (T p) :
			Port::SysDomain::ProtDomainInfo (p)
		{}
	};
};

}
}

#endif
