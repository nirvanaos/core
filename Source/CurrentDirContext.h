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
#ifndef NIRVANA_CORE_CURRENTDIRCONTEXT_H_
#define NIRVANA_CORE_CURRENTDIRCONTEXT_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <CORBA/CosNaming.h>

namespace Nirvana {
namespace Core {

class CurrentDirContext
{
public:
	const CosNaming::Name& current_dir () const
	{
		return current_dir_;
	}

	void chdir (const IDL::String& path);

	static CosNaming::Name default_dir ();

private:
	CosNaming::Name get_name_from_path (const IDL::String& path) const;
	static CosNaming::NamingContext::_ref_type name_service ();

private:
	CosNaming::Name current_dir_;
};


}
}

#endif
