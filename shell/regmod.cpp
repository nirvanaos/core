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
		if (argv.size () != 1) {
			the_posix->write (1, usage, sizeof (usage) - 1);
			return -1;
		}

		CosNaming::Name name;
		the_posix->append_path (name, argv [0], true);

		Packages::_ref_type packages = SysDomain::_narrow (
			CORBA::the_orb->resolve_initial_references ("SysDomain")
		)->provide_packages ();

		packages->register_module (name, 0);

		return 0;
	}
};

}

NIRVANA_EXPORT (_exp_regmod, "Nirvana/regmod", Nirvana::Cmdlet, Nirvana::Static_regmod)
