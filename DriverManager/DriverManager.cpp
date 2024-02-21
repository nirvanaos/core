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
#include <Nirvana/Nirvana.h>
#include <Nirvana/NDBC_s.h>
#include "../Source/Nirvana/CoreDomains.h"

using namespace CORBA;
using namespace Nirvana;

namespace NDBC {

class Static_manager :
	public servant_traits <DriverManager>::ServantStatic <Static_manager>
{
public:
	static Driver::_ref_type getDriver (IDL::String& id)
	{
		return manager_.getDriver (id);
	}

	static DataSource::_ref_type getDataSource (IDL::String& url)
	{
		return manager_.getDataSource (url);
	}

	static Connection::_ref_type getConnection (IDL::String& url, const IDL::String& user,
		const IDL::String& pwd)
	{
		return manager_.getConnection (url, user, pwd);
	}

private:
	class Manager
	{
	public:
		Manager ()
		{
			ProtDomainCore::_ref_type prot_domain = ProtDomainCore::_narrow (
				SysDomain::_narrow (g_ORB->resolve_initial_references ("SysDomain"))->prot_domain ());

			Object::_ref_type obj = prot_domain->load_and_bind (5, "/sbin/SQLite.olf", false, "NDBC/factory_sqlite");
			DriverFactory::_ref_type df = DriverFactory::_narrow (obj);
			if (!df)
				throw_INITIALIZE ();
			sqlite_ = df->getDriver ();
			if (!sqlite_)
				throw_INITIALIZE ();
		}

		~Manager ()
		{}

		Driver::_ref_type getDriver (IDL::String& id)
		{
			std::transform (id.begin (), id.end (), id.begin (), tolower);
			if (id != "sqlite")
				raise ("Unknown driver: " + id);
			return sqlite_;
		}

		DataSource::_ref_type getDataSource (IDL::String& url)
		{
			size_t id_len = url.find (':');
			if (id_len == IDL::String::npos)
				raise ("Invalid url: " + url);
			IDL::String id = url.substr (0, id_len);
			url.erase (0, id_len + 1);
			Driver::_ref_type driver = getDriver (id);
			return driver->getDataSource (url);
		}

		Connection::_ref_type getConnection (IDL::String& url, const IDL::String& user,
			const IDL::String& pwd)
		{
			NDBC::Properties props;

			size_t params = url.find ('?');
			if (params != IDL::String::npos) {
				for (size_t param = params + 1;;) {
					size_t end = url.find ('&', param);
					props.push_back (url.substr (param, end));
					if (end == IDL::String::npos)
						break;
				}
				url.resize (params);
			}
			DataSource::_ref_type ds = getDataSource (url);
			return ds->getConnection (props);
		}

	private:
		Driver::_ref_type sqlite_;
	};

	NIRVANA_NORETURN static void raise (IDL::String msg);

private:
	static Manager manager_;
};

Static_manager::Manager Static_manager::manager_;

NIRVANA_NORETURN void Static_manager::raise (IDL::String msg)
{
	throw NDBC::SQLException (NDBC::SQLWarning (0, std::move (msg)), NDBC::SQLWarnings ());
}

}

NIRVANA_EXPORT_OBJECT (_exp_NDBC_manager, NDBC::Static_manager)
