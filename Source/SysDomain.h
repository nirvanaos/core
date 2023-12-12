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
#include "ORB/SysServant.h"
#include "ORB/Services.h"
#include "ORB/system_services.h"
#include "ORB/ESIOP.h"
#include <Nirvana/CoreDomains_s.h>
#include <Port/SystemInfo.h>
#include <Port/SysDomain.h>
#include "MapUnorderedUnstable.h"
#include <NameService/FileSystem.h>
#include <fnctl.h>

namespace Nirvana {

typedef IDL::Sequence <CORBA::Octet> SecurityId;

namespace Core {

/// System domain
class SysDomain :
	public CORBA::Core::SysServantImpl <SysDomain, SysDomainCore, Nirvana::SysDomain>,
	private Port::SysDomain
{
public:
	SysDomain () :
		shutdown_started_ (false)
	{}

	~SysDomain ()
	{}

	static Version version ()
	{
		return { 0, 0, 0, 0 };
	}

	static Platforms supported_platforms ()
	{
		return Platforms (Port::SystemInfo::supported_platforms (),
			Port::SystemInfo::supported_platforms () + Port::SystemInfo::SUPPORTED_PLATFORM_CNT);
	}

	Nirvana::SysDomain::_ref_type connect (const IDL::String& user, const IDL::String& password)
	{
		return _this ();
	}

	static Nirvana::ProtDomain::_ref_type prot_domain ()
	{
		return Nirvana::ProtDomain::_narrow (
			CORBA::Core::Services::bind (CORBA::Core::Services::ProtDomain));
	}

	Nirvana::ProtDomain::_ref_type create_prot_domain (uint16_t platform)
	{
		return Nirvana::ProtDomain::_narrow (prot_domain_ref (
			Port::SysDomain::create_prot_domain (platform, IDL::String (), 0)));
	}

	static Nirvana::ProtDomain::_ref_type create_prot_domain_as_user (uint16_t platform,
		const IDL::String& user, const IDL::String& password)
	{
		throw_NO_IMPLEMENT ();
	}

	void get_bind_info (const IDL::String& obj_name, unsigned platform, BindInfo& bind_info)
	{
		// TODO:: Temporary solution, for testing
		IDL::String path = Port::SysDomain::get_platform_dir (platform);
		IDL::String translated;
		if (FileSystem::translate_path (path, translated))
			path = std::move (translated);
		auto ns = CosNaming::NamingContextExt::_narrow (CORBA::Core::Services::bind (CORBA::Core::Services::NameService));

		const char* module_id = (obj_name == "Nirvana/g_dec_calc") ? "DecCalc.olf" : "TestModule.olf";
		path += '/';
		path += module_id;
		CORBA::Object::_ref_type obj;
		try {
			obj = ns->resolve_str (path);
		} catch (const CORBA::Exception& ex) {
			const CORBA::SystemException* se = CORBA::SystemException::_downcast (&ex);
			if (se)
				se->_raise ();
			else
				throw_OBJECT_NOT_EXIST ();
		}
		File::_ref_type file = File::_narrow (obj);
		if (!file)
			throw_UNKNOWN (make_minor_errno (EISDIR));
		ModuleLoad& module_load = bind_info.module_load ();
		module_load.binary (AccessDirect::_narrow (file->open (O_RDONLY | O_DIRECT, 0)->_to_object ()));
		module_load.module_id (module_id);
	}

	CORBA::Object::_ref_type get_service (const IDL::String& id)
	{
		return CORBA::Core::Services::bind (id);
	}

	void shutdown (unsigned flags);

	/// Get context for asynchronous call from the port.
	/// 
	/// \param impl SysDomain servant reference.
	/// \param sync_context SyncContext reference.
	static void get_call_context (Ref <SysDomain>& impl, Ref <SyncContext>& sync_context);

	/// Called from the port level
	/// 
	/// \param domain_id Protection domain id.
	/// \param platform Platform id.
	/// \param user User id.
	void domain_created (ESIOP::ProtDomainId domain_id, uint_fast16_t platform, SecurityId&& user)
	{
		domains_.emplace (domain_id, platform, std::move (user));
		Port::SysDomain::on_domain_start (domain_id);
		if (shutdown_started_)
			ESIOP::send_shutdown (domain_id);
	}

	/// Called from the port level
	/// 
	/// \param domain_id Protection domain id.
	void domain_destroyed (ESIOP::ProtDomainId domain_id) noexcept
	{
		domains_.erase (domain_id);
		if (shutdown_started_ && domains_.empty ())
			Scheduler::shutdown ();
	}

private:
	static CORBA::Object::_ref_type prot_domain_ref (ESIOP::ProtDomainId domain_id);

	struct DomainInfo
	{
		SecurityId user;
		uint_fast16_t platform;

		DomainInfo (SecurityId&& _user, uint_fast16_t _platform) :
			user (std::move (_user)),
			platform (_platform)
		{}
	};

	typedef MapUnorderedUnstable <ESIOP::ProtDomainId, DomainInfo, std::hash <ESIOP::ProtDomainId>,
		std::equal_to <ESIOP::ProtDomainId>, UserAllocator> DomainMap;

	class DomainsImpl
	{
	public:
		void emplace (ESIOP::ProtDomainId domain_id, uint_fast16_t platform, SecurityId&& user)
		{
			domains_.emplace (std::piecewise_construct, std::forward_as_tuple (domain_id),
				std::forward_as_tuple (std::move (user), platform));
		}

		void erase (ESIOP::ProtDomainId domain_id) noexcept
		{
			domains_.erase (domain_id);
		}

		bool empty () const noexcept
		{
			return domains_.empty ();
		}

		void shutdown ()
		{
			for (const auto& d : domains_) {
				ESIOP::send_shutdown (d.first);
			}
		}

	private:
		DomainMap domains_;
	};

	class DomainsDummy
	{
	public:
		void emplace (ESIOP::ProtDomainId domain_id, uint_fast16_t platform, SecurityId&& user)
		{}

		void erase (ESIOP::ProtDomainId domain_id)
		{}

		static bool empty () noexcept
		{
			return true;
		}

		void shutdown ()
		{}
	};

	typedef std::conditional_t <SINGLE_DOMAIN, DomainsDummy, DomainsImpl> Domains;

	Domains domains_;
	bool shutdown_started_;
};

}
}

#endif
