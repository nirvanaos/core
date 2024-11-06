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
#ifndef PACMAN_PACMAN_H_
#define PACMAN_PACMAN_H_
#pragma once

#include <CORBA/Server.h>
#include <CORBA/CosEventChannelAdmin.h>
#include <Nirvana/Domains_s.h>
#include <Nirvana/posix_defs.h>
#include "Connection.h"
#include "version.h"

class PacMan :
	public CORBA::servant_traits <Nirvana::PM::PacMan>::Servant <PacMan>,
	public Connection
{
public:
	PacMan (Nirvana::SysDomain::_ptr_type sys_domain, CosEventChannelAdmin::ProxyPushConsumer::_ptr_type completion) :
		Connection (Connection::connect_rw),
		sys_domain_ (sys_domain),
		busy_ (false)
	{
		completion->connect_push_supplier (nullptr);
		completion_ = completion;
		connection ()->setAutoCommit (false);
	}

	~PacMan ()
	{
		complete ();
	}

	// Does not allow parallel installations
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

	static long compare_name (const Nirvana::PM::ObjBinding& l, const Nirvana::PM::ObjBinding& r) noexcept;

	struct Less
	{
		bool operator () (const Nirvana::PM::ObjBinding& l, const Nirvana::PM::ObjBinding& r) const noexcept
		{
			long cmp = compare_name (l, r);
			if (cmp < 0)
				return true;
			else if (cmp > 0)
				return false;

			return l.itf_id () < r.itf_id ();
		}

		bool operator () (const Nirvana::PM::Export& l, const Nirvana::PM::Export& r) const noexcept
		{
			return compare_name (l.binding (), r.binding ()) < 0;
		}
	};

	struct Equal
	{
		bool operator () (const Nirvana::PM::ObjBinding& l, const Nirvana::PM::ObjBinding& r)
			const noexcept
		{
			return l.name () == r.name () && l.major () == r.major () && l.minor () == r.minor ()
				&& l.itf_id () == r.itf_id ();
		}

		bool operator () (const Nirvana::PM::Export& l, const Nirvana::PM::Export& r) const noexcept
		{
			return operator () (l.binding (), r.binding ()) && l.type () == r.type ();
		}
	};

	uint16_t register_binary (const IDL::String& path, const IDL::String& module_name, unsigned flags)
	{
		Lock lock (*this);

		SemVer svname (module_name);

		Nirvana::PM::ModuleBindings metadata;
		Nirvana::BinaryInfo bi = sys_domain_->get_module_bindings (path, metadata);

		try {

			Statement stm = get_statement ("SELECT id,flags FROM module WHERE name=? AND version=? AND prerelease=?");

			stm->setString (1, svname.name ());
			stm->setBigInt (2, svname.version ());
			stm->setString (3, svname.prerelease ());

			NDBC::ResultSet::_ref_type rs = stm->executeQuery ();

			int32_t module_id;
			if (rs->next ()) {

				module_id = rs->getInt (1);
				if (bi.module_flags () != rs->getSmallInt (2))
					Nirvana::BindError::throw_message ("Module type mismatch");

				std::sort (metadata.imports ().begin (), metadata.imports ().end (), Less ());
				std::sort (metadata.exports ().begin (), metadata.exports ().end (), Less ());

				Nirvana::PM::ModuleBindings cur_md;
				get_module_bindings (module_id, cur_md);

				check_match (metadata, cur_md);

			} else {

				stm = get_statement ("INSERT INTO module(name,version,prerelease,flags)VALUES(?,?,?,?)"
					"RETURNING id");
				stm->setString (1, svname.name ());
				stm->setBigInt (2, svname.version ());
				stm->setString (3, svname.prerelease ());
				stm->setInt (4, bi.module_flags ());
				rs = stm->executeQuery ();
				rs->next ();
				module_id = rs->getInt (1);

				stm = get_statement ("INSERT INTO export(module,name,major,minor,type,interface)"
					"VALUES(?,?,?,?,?,?)");
				stm->setInt (1, module_id);
				for (const auto& e : metadata.exports ()) {
					stm->setString (2, e.binding ().name ());
					stm->setInt (3, e.binding ().major ());
					stm->setInt (4, e.binding ().minor ());
					stm->setInt (5, (int)e.type ());
					stm->setString (6, e.binding ().itf_id ());
					stm->executeUpdate ();
				}

				stm = get_statement ("INSERT INTO import(module,name,version,interface)"
					"VALUES(?,?,?,?)");
				stm->setInt (1, module_id);
				for (const auto& b : metadata.imports ()) {
					stm->setString (2, b.name ());
					stm->setInt (3, version (b.major (), b.minor ()));
					stm->setString (4, b.itf_id ());
					stm->executeUpdate ();
				}
			}

			stm = get_statement ("INSERT OR REPLACE INTO binary VALUES(?,?,?)");
			stm->setInt (1, module_id);
			stm->setInt (2, bi.platform ());
			stm->setString (3, path);
			stm->executeUpdate ();

		} catch (NDBC::SQLException& ex) {
			on_sql_exception (ex);
		}

		return bi.platform ();
	}

	void unregister (const IDL::String& module_name)
	{
		Nirvana::throw_NO_IMPLEMENT ();
	}

private:
	void complete () noexcept;

	static void check_match (const Nirvana::PM::ModuleBindings& l, const Nirvana::PM::ModuleBindings& r)
	{
		if (!
			std::equal (l.exports ().begin (), l.exports ().end (),
				r.exports ().begin (), r.exports ().end (), Equal ())
			&&
			std::equal (l.imports ().begin (), l.imports ().end (),
				r.imports ().begin (), r.imports ().end (), Equal ())
			)
			Nirvana::BindError::throw_message ("Metadata mismatch");
	}

private:
	Nirvana::SysDomain::_ref_type sys_domain_;
	CosEventComm::PushConsumer::_ref_type completion_;
	bool busy_;
};

#endif
