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
#ifndef PACMAN_PACKAGES_H_
#define PACMAN_PACKAGES_H_
#pragma once

#include <CORBA/Server.h>
#include <Nirvana/Domains_s.h>
#include "PacMan.h"
#include "factory.h"

class Packages :
	public CORBA::servant_traits <Nirvana::PM::Packages>::Servant <Packages>
{
	typedef CORBA::servant_traits <Nirvana::PM::Packages>::Servant <Packages> Servant;

	static const unsigned POOL_MAX_CACHE = 10;
	static const unsigned POOL_MAX_CREATE = 1000;

public:
	Packages (CORBA::Object::_ptr_type component);

	~Packages ()
	{
		if (event_channel_)
			event_channel_->destroy ();
	}

	void get_binding (const IDL::String& name, Nirvana::PlatformId platform,
		Nirvana::PM::Binding& binding)
	{
		Connection (pool_).get_binding (name, platform, binding);
	}

	IDL::String get_module_name (Nirvana::ModuleId id)
	{
		return Connection (pool_).get_module_name (id);
	}

	Nirvana::PM::Packages::Modules get_module_dependencies (Nirvana::ModuleId id)
	{
		return Connection (pool_).get_module_dependencies (id);
	}

	Nirvana::PM::Packages::Modules get_dependent_modules (Nirvana::ModuleId id)
	{
		return Connection (pool_).get_dependent_modules (id);
	}

	void get_module_bindings (Nirvana::ModuleId id, Nirvana::PM::ModuleBindings& metadata)
	{
		Connection (pool_).get_module_bindings (id, metadata);
	}

	Nirvana::PM::PacMan::_ref_type manage ()
	{
		if (manager_completion_) {
			bool completed = false;
			manager_completion_->try_pull (completed);
			if (completed) {
				manager_completion_->disconnect_pull_supplier ();
				manager_completion_ = nullptr;
			} else
				return nullptr;
		}

		if (!event_channel_)
			event_channel_ = CORBA::the_orb->create_event_channel ();

		CosEventChannelAdmin::ProxyPullSupplier::_ref_type supplier = event_channel_->for_consumers ()->obtain_pull_supplier ();
		supplier->connect_pull_consumer (nullptr);

		Nirvana::PM::PacMan::_ref_type pacman = Nirvana::PM::pacman_factory->create (sys_domain (),
			event_channel_->for_suppliers ()->obtain_push_consumer ());

		manager_completion_ = std::move (supplier);

		return pacman;
	}

	Nirvana::SysDomain::_ref_type sys_domain ()
	{
		return Nirvana::SysDomain::_narrow (_this ()->_get_component ());
	}

private:
	static void create_database ();

private:
	NDBC::ConnectionPool::_ref_type pool_;
	CosEventChannelAdmin::EventChannel::_ref_type event_channel_;
	CosEventComm::PullSupplier::_ref_type manager_completion_;

	static const char* const db_script_ [];
};

#endif
