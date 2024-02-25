/*
* Nirvana SQLite driver.
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
#ifndef SQLITE_DRIVER_H_
#define SQLITE_DRIVER_H_
#pragma once

#include "Activator.h"
#include "Global.h"
#include <fnctl.h>

namespace SQLite {

class Driver : public CORBA::servant_traits <NDBC::Driver>::Servant <Driver>
{
public:
	Driver ()
	{
		PortableServer::POA::_ref_type root = PortableServer::POA::_narrow (
			CORBA::g_ORB->resolve_initial_references ("RootPOA"));

		CORBA::PolicyList policies;
		policies.push_back (root->create_lifespan_policy (PortableServer::LifespanPolicyValue::PERSISTENT));
		policies.push_back (root->create_id_assignment_policy (PortableServer::IdAssignmentPolicyValue::USER_ID));
		policies.push_back (root->create_request_processing_policy (PortableServer::RequestProcessingPolicyValue::USE_SERVANT_MANAGER));
		policies.push_back (root->create_id_uniqueness_policy (PortableServer::IdUniquenessPolicyValue::MULTIPLE_ID));
		PortableServer::POAManager::_ref_type manager = root->the_POAManager ();
		PortableServer::POA::_ref_type adapter = root->create_POA ("sqlite", manager, policies);
		adapter->set_servant_manager (CORBA::make_stateless <Activator> ()->_this ());
		adapter_ = std::move (adapter);
		adapter_manager_ = std::move (manager);
	}

	~Driver ()
	{}

	NDBC::DataSource::_ref_type getDataSource (IDL::String& url) const
	{
		size_t col = url.find (':');
		if (col != IDL::String::npos) {
			if (url.compare (0, col, "file") == 0)
				url.erase (0, col + 1);
			else
				throw_exception ("Invalid URL");
		}
		// Create file if not exists
		Nirvana::File::_ref_type file = global.open_file (url, O_CREAT)->file ();

		// Get file ID and create reference
		return NDBC::DataSource::_narrow (
			adapter_->create_reference_with_id (file->id (), NDBC::_tc_DataSource->id ()));
	}

	static NDBC::PropertyInfo getPropertyInfo ()
	{
		NDBC::PropertyInfo props {
			NDBC::DriverProperty {
				"mode",
				"Open mode",
				"",
				{"ro", "rw", "rwc", "memory"},
				false
			},
			NDBC::DriverProperty {
				"immutable",
				"Read-only media",
				"",
				{"1"},
				false
			}
		};

		return props;
	}

	NDBC::Connection::_ref_type connect (IDL::String& url, const NDBC::Properties& props) const
	{
		return getDataSource (url)->getConnection (props);
	}

	static IDL::String version ()
	{
		return sqlite3_version;
	}

	void close ()
	{
		if (adapter_manager_->get_state () == PortableServer::POAManager::State::ACTIVE) {
			adapter_->destroy (true, true);
			deactivate_servant (this);
		}
	}

private:
	PortableServer::POA::_ref_type adapter_;
	PortableServer::POAManager::_ref_type adapter_manager_;
};

}

#endif
