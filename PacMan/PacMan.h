/*
* Nirvana package manager.
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
#ifndef NIRVANA_PACMAN_PACMAN_H_
#define NIRVANA_PACMAN_PACMAN_H_
#pragma once

#include <CORBA/Server.h>
#include <CORBA/CosEventComm.h>
#include <Nirvana/Domains_s.h>
#include <Nirvana/posix_defs.h>
#include "Connection.h"

class PacMan :
	public CORBA::servant_traits <Nirvana::PacMan>::Servant <PacMan>,
	public Connection
{
public:
	PacMan (Nirvana::SysDomain::_ptr_type sys_domain, CosEventComm::PushConsumer::_ptr_type completion) :
		sys_domain_ (sys_domain),
		name_service_ (CosNaming::NamingContextExt::_narrow (
			CORBA::the_orb->resolve_initial_references ("NameService"))),
		completion_ (completion)
	{}

	void commit ()
	{
		check_active ();
		Connection::commit ();
		complete ();
	}

	void rollback ()
	{
		check_active ();
		Connection::rollback ();
		complete ();
	}

	uint16_t register_binary (const CosNaming::Name& path, const IDL::String& module_name, unsigned flags)
	{
		Nirvana::AccessDirect::_ref_type binary;

		{
			CORBA::Object::_ref_type obj = name_service_->resolve (path);
			Nirvana::File::_ref_type file = Nirvana::File::_narrow (obj);
			binary = Nirvana::AccessDirect::_narrow (file->open (O_RDONLY | O_DIRECT, 0)->_to_object ());
		}

		uint16_t platform = Nirvana::Core::Binary::get_platform (binary);

		/*
		if (std::find (Port::SystemInfo::supported_platforms (),
			Port::SystemInfo::supported_platforms () + Port::SystemInfo::SUPPORTED_PLATFORM_CNT,
			platform) == Port::SystemInfo::supported_platforms () + Port::SystemInfo::SUPPORTED_PLATFORM_CNT) {
			Nirvana::BindError::throw_message ("Unsupported platform");
		}*/

		Nirvana::ProtDomain::_ref_type bind_domain;
		if (Nirvana::Core::SINGLE_DOMAIN)
			bind_domain = sys_domain_->prot_domain ();
		else
			bind_domain = sys_domain_->provide_manager ()->create_prot_domain (platform);

		Nirvana::ModuleBindings module_bindings =
			Nirvana::ProtDomainCore::_narrow (bind_domain)->get_module_bindings (binary);

		if (!Nirvana::Core::SINGLE_DOMAIN)
			bind_domain->shutdown (0);

		return platform;
	}

	void unregister (const IDL::String& module_name)
	{
		Nirvana::throw_NO_IMPLEMENT ();
	}

private:
	void complete ();
	void check_active () const;

private:
	Nirvana::SysDomain::_ref_type sys_domain_;
	CosNaming::NamingContextExt::_ref_type name_service_;
	CosEventComm::PushConsumer::_ref_type completion_;
};

#endif
