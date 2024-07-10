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

namespace Nirvana {

class Static_regmod :
	public CORBA::servant_traits <Nirvana::Cmdlet>::ServantStatic <Static_regmod>
{
public:
	static int run (StringSeq& argv)
	{
		static const char usage [] = "Usage: regmod <module path>\n";
		if (argv.size () < 1 || argv.size () > 2) {
			print (1, usage);
			return -1;
		}

		CosNaming::Name bin_path;
		the_posix->append_path (bin_path, argv [0], true);

		const std::string* pname;
		if (argv.size () > 1)
			pname = &argv [1];
		else
			pname = &bin_path.back ().id ();

		Packages::_ref_type packages = SysDomain::_narrow (
			CORBA::the_orb->resolve_initial_references ("SysDomain")
		)->provide_packages ();

		PacMan::_ref_type pacman = packages->manage ();
		if (!pacman) {
			print (2, "Installation session is already started, try later.\n");
			return -1;
		}

		int ret = 0;

		try {
			pacman->register_binary (bin_path, *pname, 0);
		} catch (const BindError::Error& ex) {
			ret = -1;
			print (pacman, ex.info ());
			for (auto it = ex.stack ().crbegin (), end = ex.stack ().crend (); it != end; ++it) {
				print (pacman, *it);
			}
		}

		return ret;
	}

private:
	static void print (int fd, const char* s);
	static void print (int fd, const std::string& s);
	static void print (int fd, int d);
	static void println (int fd);
	static void print (PackagesDB::_ptr_type packages, const BindError::Info& err);
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

void Static_regmod::print (PackagesDB::_ptr_type packages, const BindError::Info& err)
{
	switch (err._d ()) {
	case BindError::Type::ERR_TEXT:
		print (2, err.message ());
		break;

	case BindError::Type::ERR_OBJ_NOT_FOUND:
		print (2, "Object not found: ");
		print (2, err.obj_name ());
		break;

	case BindError::Type::ERR_ITF_NOT_FOUND:
		print (2, "Interface ");
		print (2, err.itf_info ().itf_id ());
		print (2, " not available for object ");
		print (2, err.itf_info ().obj_name ());
		break;

	case BindError::Type::ERR_MOD_LOAD:
		print (2, "Module load failed: ");
		print (2, packages->get_module_name (err.mod_info ().module_id ()));
		break;

	case BindError::Type::ERR_SYSTEM: {
		CORBA::UNKNOWN se;
		err.system_exception () >>= se;
		print (2, se.what ());
		print (2, " (");
		print (2, se.minor ());
		print (2, ")");
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

}

NIRVANA_EXPORT (_exp_regmod, "Nirvana/regmod", Nirvana::Cmdlet, Nirvana::Static_regmod)
