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

#ifndef NIRVANA_CORE_SHELL_H_
#define NIRVANA_CORE_SHELL_H_
#pragma once

#include <CORBA/Server.h>
#include <Nirvana/Shell_s.h>
#include "Nirvana_Process.h"

namespace Nirvana {

class Static_the_shell :
	public CORBA::servant_traits <Shell>::ServantStatic <Static_the_shell>
{
public:
	static Process::_ref_type spawn (AccessDirect::_ptr_type file,
		StringSeq& argv, IDL::String& work_dir, FileDescriptors& files,
		ProcessCallback::_ref_type callback)
	{
		return Core::Process::spawn (file, argv, work_dir, files, callback);
	}

	static short run_cmdlet (const IDL::String& name, StringSeq& argv,
		IDL::String& work_dir, FileDescriptors& files)
	{
		throw_NO_IMPLEMENT ();
	}

	static void create_pipe (AccessChar::_ref_type& pipe_out, AccessChar::_ref_type& pipe_in)
	{
		throw_NO_IMPLEMENT ();
	}

};

}

#endif