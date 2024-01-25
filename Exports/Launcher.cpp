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

// Using the precompiled header with CLang in this file causes error:
// The OLF static symbols not exported/imported.

#include <CORBA/Server.h>
#include <Nirvana/Legacy/Legacy_Process_s.h>
#include <Nirvana/Launcher.h>
#include "../Source/Legacy/Process.h"

namespace Nirvana {
namespace Legacy {

class Launcher :
	public CORBA::servant_traits <ProcessFactory>::ServantStatic <Launcher>
{
public:
	static Legacy::Process::_ref_type spawn (AccessDirect::_ptr_type file,
		StringSeq& argv, StringSeq& envp, IDL::String& work_dir, InheritedFiles& inherit,
		ProcessCallback::_ref_type callback)
	{
		return Core::Process::spawn (file, argv, envp, work_dir, inherit, callback);
	}
};

}
}

#if !DISABLE_LEGACY_SUPPORT ()
NIRVANA_STATIC_EXP (Nirvana, Legacy, Launcher)
#endif
