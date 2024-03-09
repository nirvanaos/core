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
#ifndef NIRVANA_CORE_CURRENTDIR_H_
#define NIRVANA_CORE_CURRENTDIR_H_
#pragma once

#include "MemContextUser.h"

namespace Nirvana {
namespace Core {

class CurrentDir
{
public:
	static CosNaming::Name current_dir ()
	{
		CosNaming::Name dir;
		MemContextUser* mc = MemContext::current ().user_context ();
		if (mc) {
			const CurrentDirContext* ctx = mc->current_dir_ptr ();
			if (ctx) {
				dir = ctx->current_dir ();
			}
		}
		if (dir.empty ())
			dir = CurrentDirContext::default_dir ();
		return dir;
	}

	static void chdir (const IDL::String& path)
	{
		MemContextUser& mc = MemContextUser::current ();
		CurrentDirContext* ctx;
		if (!path.empty ())
			ctx = &mc.current_dir ();
		else
			ctx = mc.current_dir_ptr ();
		if (ctx)
			ctx->chdir (path);
	}

};

}
}

#endif
