/*
* Nirvana shell.
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
#include "pch.h"
#include <Nirvana/Domains.h>
#include <Nirvana/Packages.h>

namespace Nirvana {

class Static_regmod :
	public CORBA::servant_traits <Nirvana::Cmdlet>::ServantStatic <Static_regmod>
{
public:
	static int run (StringSeq& argv)
	{
		static const char usage [] = "Usage: regmod <binary path> <module name>\n";
		if (argv.size () != 2) {
			print (1, usage);
			return -1;
		}

		CosNaming::Name bin_path;
		the_posix->append_path (bin_path, argv [0], true);

		PM::Packages::_ref_type packages = SysDomain::_narrow (
			CORBA::the_orb->resolve_initial_references ("SysDomain")
		)->provide_packages ();

		PM::PacMan::_ref_type pacman = packages->manage ();
		if (!pacman) {
			print (2, "Installation session is already started, try later.\n");
			return -1;
		}

		int ret = -1;

		try {
			pacman->register_binary (the_system->to_string (bin_path), argv [1], 0);
			pacman->commit ();
			ret = 0;
		} catch (const BindError::Error& ex) {
			print (pacman, ex);
		} catch (const CORBA::SystemException& ex) {
			print (ex);
			println (2);
		}

		return ret;
	}

private:
	static void print (int fd, const char* s);
	static void print (int fd, const std::string& s);
	static void print (int fd, int d);
	static void println (int fd);
	static void print (PM::PackageDB::_ptr_type packages, const BindError::Error& err);
	static void print (PM::PackageDB::_ptr_type packages, const BindError::Info& err);
	static void print (const CORBA::SystemException& se);
};

void Static_regmod::print (int fd, const char* s)
{
	the_posix->write (fd, s, strlen (s));
}

void Static_regmod::print (int fd, const std::string& s)
{
	the_posix->write (fd, s.data (), s.size ());
}

void Static_regmod::println (int fd)
{
	const char n = '\n';
	the_posix->write (fd, &n, 1);
}

void Static_regmod::print (int fd, int d)
{
	print (fd, std::to_string (d));
}

void Static_regmod::print (PM::PackageDB::_ptr_type packages, const BindError::Error& err)
{
	print (packages, err.info ());
	for (auto it = err.stack ().cbegin (), end = err.stack ().cend (); it != end; ++it) {
		print (packages, *it);
	}
}

void Static_regmod::print (PM::PackageDB::_ptr_type packages, const BindError::Info& err)
{
	switch (err._d ()) {
	case BindError::Type::ERR_MESSAGE:
		print (2, err.s ());
		break;

	case BindError::Type::ERR_OBJ_NAME:
		print (2, "Error binding object: ");
		print (2, err.s ());
		break;

	case BindError::Type::ERR_ITF_NOT_FOUND:
		print (2, "Interface ");
		print (2, err.s ());
		print (2, " is not available for object");
		break;

	case BindError::Type::ERR_MOD_LOAD:
		print (2, "Module load: ");
		print (2, packages->get_module_name (err.mod_info ().module_id ()));
		break;

	case BindError::Type::ERR_SYSTEM: {
		CORBA::UNKNOWN se;
		err.system_exception () >>= se;
		print (se);
	} break;

	case BindError::Type::ERR_UNSUP_PLATFORM:
		print (2, "Unsupported platform: ");
		print (2, err.platform_id ());
		break;

	default:
		print (2, "Unknown error");
		break;
	}

	println (2);
}

void Static_regmod::print (const CORBA::SystemException& se)
{
	print (2, se.what ());
	int err = get_minor_errno (se.minor ());
	if (err) {
		print (2, " errno: ");
		print (2, err);
	}
}

}

NIRVANA_EXPORT (_exp_regmod, "Nirvana/regmod", Nirvana::Cmdlet, Nirvana::Static_regmod)
