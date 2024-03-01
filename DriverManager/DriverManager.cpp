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

inline void lowercase (std::string& s)
{
	std::transform (s.begin (), s.end (), s.begin (), tolower);
}

template <typename T>
void lowercase (std::vector <T>& v)
{
	for (auto& t : v) {
		lowercase (t.name ());
	}
}

template <> inline
void lowercase (std::vector <std::string>& v)
{
	for (auto& s : v) {
		lowercase (s);
	}
}

template <typename T>
struct NamePred
{
	bool operator () (const T& l, const T& r) const noexcept
	{
		return l.name () < r.name ();
	}

	bool operator () (const T& l, const std::string& r) const noexcept
	{
		return l.name () < r;
	}

	bool operator () (const std::string& l, const T& r) const noexcept
	{
		return l < r.name ();
	}

};

template <typename T>
void sort (std::vector <T>& v)
{
	std::sort (v.begin (), v.end (), NamePred <T> ());
}

template <> inline
void sort (std::vector <std::string>& v)
{
	std::sort (v.begin (), v.end ());
}

template <typename T>
typename std::vector <T>::const_iterator find (const std::vector <T>& v, const std::string& s)
{
	typename std::vector <T>::const_iterator it = std::lower_bound (v.begin (), v.end (), s, NamePred <T> ());
	if (it != v.end () && it->name () == s)
		return it;
	return v.end ();
}

template <> inline
typename std::vector <std::string>::const_iterator find (const std::vector <std::string>& v, const std::string& s)
{
	std::vector <std::string>::const_iterator it = std::lower_bound (v.begin (), v.end (), s);
	if (it != v.end () && *it == s)
		return it;
	return v.end ();
}

class Static_manager :
	public servant_traits <DriverManager>::ServantStatic <Static_manager>
{
public:
	static DataSource::_ref_type getDataSource (IDL::String& url)
	{
		return manager_.getDataSource (url);
	}

	static Connection::_ref_type getConnection (IDL::String& url, const IDL::String& user,
		const IDL::String& pwd)
	{
		return manager_.getConnection (url, user, pwd);
	}

	InstalledDrivers getDrivers () const
	{
		return manager_.getDrivers ();
	}

	IDL::String getDriverVersion (IDL::String& id) const
	{
		return manager_.getDriverVersion (id);
	}

	PropertyInfo getPropertyInfo (IDL::String& id) const
	{
		return manager_.getPropertyInfo (id);
	}

private:
	class Manager
	{
	public:
		Manager () :
			sqlite_ (load_sqlite ())
		{}

		~Manager ()
		{}

		DataSource::_ref_type getDataSource (IDL::String& url)
		{
			auto id = get_driver_id (url);
			const DriverInfo& driver = get_driver_info (id);
			return driver.driver->getDataSource (url);
		}

		Connection::_ref_type getConnection (IDL::String& url, const IDL::String& user,
			const IDL::String& pwd)
		{
			auto id = get_driver_id (url);
			const DriverInfo& driver = get_driver_info (id);

			NDBC::Properties props;

			size_t params = url.find ('?');
			if (params != IDL::String::npos) {
				for (size_t param = params + 1;;) {
					size_t end = url.find ('&', param);
					std::string s = url.substr (param, end);
					lowercase (s);
					size_t eq = s.find ('=');
					if (eq == std::string::npos)
						raise ("Invalid parameter: " + s);
					Property prop (s.substr (0, eq), s.substr (eq + 1));
					auto it = find (driver.prop_info.properties (), prop.name ());
					if (it == driver.prop_info.properties ().end ())
						raise ("Invalid property: " + prop.name ());
					if (!it->choices ().empty () && find (it->choices (), prop.value ()) == it->choices ().end ())
						raise ("Invalid value \'" + prop.value () + " for property \'" + prop.name () + "\'");
					props.push_back (std::move (prop));

					if (end == IDL::String::npos)
						break;
				}
				url.resize (params);
			}
			if (!user.empty ()) {
				auto it = find (driver.prop_info.properties (), "user");
				if (it != driver.prop_info.properties ().end ())
					props.emplace_back ("user", user);
			}
			if (!pwd.empty ()) {
				auto it = find (driver.prop_info.properties (), "password");
				if (it != driver.prop_info.properties ().end ())
					props.emplace_back ("password", pwd);
			}
			sort (props);
			for (const auto& p : driver.prop_info.properties ()) {
				if (p.required () && find (props, p.name ()) == props.end ())
					raise ("Property \'" + p.name () + "\' is required");
			}
			return driver.driver->connect (url, props);
		}

		InstalledDrivers getDrivers () const
		{
			InstalledDrivers list;
			list.emplace_back ("sqlite", sqlite_.prop_info.version ());
			return list;
		}

		IDL::String getDriverVersion (IDL::String& id) const
		{
			const DriverInfo* di = find_driver_info (id);
			IDL::String ver;
			if (di)
				ver = di->prop_info.version ();
			return ver;
		}

		PropertyInfo getPropertyInfo (IDL::String& id) const
		{
			return get_driver_info (id).prop_info;
		}

	private:
		struct DriverInfo
		{
			DriverInfo (Object::_ptr_type obj)
			{
				if (!(driver = Driver::_narrow (obj))) {
					DriverFactory::_ref_type df = DriverFactory::_narrow (obj);
					if (!df)
						throw INITIALIZE ();
					driver = df->getDriver ();
					if (!driver)
						throw INITIALIZE ();
				}
				prop_info = driver->getPropertyInfo ();
				for (auto& p : prop_info.properties ()) {
					lowercase (p.choices ());
					sort (p.choices ());
				}
				lowercase (prop_info.properties ());
				sort (prop_info.properties ());
			}

			~DriverInfo ()
			{
				try {
					driver->close ();
				} catch (...) {}
			}

			PropertyInfo prop_info;
			Driver::_ref_type driver;
		};

		static std::string get_driver_id (IDL::String& url)
		{
			size_t id_len = url.find (':');
			if (id_len == IDL::String::npos)
				raise ("Invalid url: " + url);
			IDL::String id = url.substr (0, id_len);
			url.erase (0, id_len + 1);
			return id;
		}

		const DriverInfo* find_driver_info (IDL::String& id) const
		{
			lowercase (id);
			if (id != "sqlite")
				return nullptr;
			return &sqlite_;
		}

		const DriverInfo& get_driver_info (IDL::String& id) const
		{
			const DriverInfo* di = find_driver_info (id);
			if (!di)
				raise ("Unknown driver: " + id);
			return *di;
		}

		static Object::_ref_type load_sqlite ()
		{
			ProtDomainCore::_ref_type prot_domain = ProtDomainCore::_narrow (
				SysDomain::_narrow (g_ORB->resolve_initial_references ("SysDomain"))->prot_domain ());

			return prot_domain->load_and_bind (5, "/sbin/SQLite.olf", false, "NDBC/sqlite");
		}

	private:
		DriverInfo sqlite_;
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
