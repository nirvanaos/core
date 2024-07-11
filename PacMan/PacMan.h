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
#include <CORBA/CosEventChannelAdmin.h>
#include <Nirvana/Domains_s.h>
#include <Nirvana/posix_defs.h>
#include "Connection.h"
#include "../Source/open_binary.h"

class PacMan :
	public CORBA::servant_traits <Nirvana::PacMan>::Servant <PacMan>,
	public Connection
{
public:
	PacMan (Nirvana::SysDomain::_ptr_type sys_domain, CosEventChannelAdmin::ProxyPushConsumer::_ptr_type completion) :
		Connection (open_rw ()),
		sys_domain_ (sys_domain),
		name_service_ (CosNaming::NamingContextExt::_narrow (
			CORBA::the_orb->resolve_initial_references ("NameService"))),
		busy_ (false)
	{
		completion->connect_push_supplier (nullptr);
		completion_ = std::move (completion);
		connection ()->setAutoCommit (false);
	}

	~PacMan ()
	{
		complete ();
	}

	// Does not allow parallel installation
	class Lock
	{
	public:
		Lock (PacMan& obj);

		~Lock ()
		{
			obj_.busy_ = false;
		}

	private:
		PacMan& obj_;
	};

	void commit ()
	{
		Lock lock (*this);
		Connection::commit ();
		complete ();
	}

	void rollback ()
	{
		Lock lock (*this);
		Connection::rollback ();
		complete ();
	}

	struct BindingLess
	{
		bool operator () (const Nirvana::ModuleBinding& l, const Nirvana::ModuleBinding& r) const noexcept
		{
			int cmp = l.name ().compare (r.name ());
			if (cmp < 0)
				return true;
			else if (cmp > 0)
				return false;
			if ((int32_t)l.version () < (int32_t)r.version ())
				return true;
			else if ((int32_t)l.version () > (int32_t)r.version ())
				return false;

			return (int)l.flags () < (int)r.flags ();
		}
	};

	uint16_t register_binary (const IDL::String& path, const IDL::String& module_name, unsigned flags)
	{
		Lock lock (*this);

		SemVer svname;
		if (!svname.from_string (module_name))
			Nirvana::BindError::throw_message ("Module name '" + module_name + "' is invalid");

		Nirvana::AccessDirect::_ref_type binary = Nirvana::Core::open_binary (name_service_, path);

		uint_fast16_t platform = Nirvana::Core::Binary::get_platform (binary);
		if (!Nirvana::Core::Binary::is_supported_platform (platform)) {
			Nirvana::BindError::Error err;
			err.info ().platform_id (platform);
			throw err;
		}

		Nirvana::ProtDomain::_ref_type bind_domain;
		if (Nirvana::Core::SINGLE_DOMAIN)
			bind_domain = sys_domain_->prot_domain ();
		else
			bind_domain = sys_domain_->provide_manager ()->create_prot_domain (platform);

		Nirvana::ModuleBindings metadata =
			Nirvana::ProtDomainCore::_narrow (bind_domain)->get_module_bindings (binary);

		if (!Nirvana::Core::SINGLE_DOMAIN)
			bind_domain->shutdown (0);

		try {

			Statement stm = get_statement ("SELECT id,flags FROM module WHERE name=? AND version=? AND prerelease=?");

			stm->setString (1, svname.name);
			stm->setBigInt (2, svname.version);
			stm->setString (3, svname.prerelease);

			NDBC::ResultSet::_ref_type rs = stm->executeQuery ();

			int32_t module_id;
			if (rs->next ()) {
				module_id = rs->getInt (1);
				if (metadata.flags () != rs->getSmallInt (2))
					Nirvana::BindError::throw_message ("Module type mismatch");

				std::sort (metadata.bindings ().begin (), metadata.bindings ().end (), BindingLess ());

				stm = get_statement ("SELECT name,version,flags FROM object WHERE module=? ORDER BY name,version,flags");
				stm->setInt (1, module_id);

				rs = stm->executeQuery ();

				bool mismatch = false;
				for (auto it = metadata.bindings ().cbegin (), end = metadata.bindings ().cend (); it != end; ++it) {
					if (!rs->next ()) {
						mismatch = true;
						break;
					}
					if (rs->getString (1) != it->name ()) {
						mismatch = true;
						break;
					}
					if (rs->getInt (2) != it->version ()) {
						mismatch = true;
						break;
					}
					if (rs->getInt (3) != it->flags ()) {
						mismatch = true;
						break;
					}
				}

				if (!mismatch && rs->next ())
					mismatch = true;

				if (mismatch)
					Nirvana::BindError::throw_message ("Metadata mismatch");

			} else {
				stm = get_statement ("INSERT INTO module(name,version,prerelease,flags)VALUES(?,?,?,?)RETURNING id");
				stm->setString (1, svname.name);
				stm->setBigInt (2, svname.version);
				stm->setString (3, svname.prerelease);
				stm->setInt (4, metadata.flags ());
				rs = stm->executeQuery ();
				rs->next ();
				module_id = rs->getInt (1);

				stm = get_statement ("INSERT INTO object VALUES(?,?,?,?)");
				stm->setInt (3, module_id);
				for (auto b : metadata.bindings ()) {
					stm->setString (1, b.name ());
					stm->setInt (2, b.version ());
					stm->setInt (4, b.flags ());
					stm->executeUpdate ();
				}
			}
			stm = get_statement ("INSERT OR REPLACE INTO binary VALUES(?,?,?)");
			stm->setInt (1, module_id);
			stm->setInt (2, platform);
			stm->setString (3, path);
			stm->executeUpdate ();

		} catch (NDBC::SQLException& ex) {
			on_sql_exception (ex);
		}

		return platform;
	}

	void unregister (const IDL::String& module_name)
	{
		Nirvana::throw_NO_IMPLEMENT ();
	}

private:
	void complete () noexcept;

private:
	Nirvana::SysDomain::_ref_type sys_domain_;
	CosNaming::NamingContextExt::_ref_type name_service_;
	CosEventComm::PushConsumer::_ref_type completion_;
	bool busy_;
};

#endif
