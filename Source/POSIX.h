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
#ifndef NIRVANA_CORE_POSIX_H_
#define NIRVANA_CORE_POSIX_H_

#include <Nirvana/Nirvana.h>
#include <Nirvana/POSIX_s.h>
#include <signal.h>
#include "Signals.h"
#include "FileDescriptors.h"
#include "CurrentDir.h"
#include "NameService/FileSystem.h"
#include "ExecDomain.h"
#include "TimerEvent.h"
#include "unrecoverable_error.h"

namespace Nirvana {

class Static_the_posix :
	public IDL::traits <POSIX>::ServantStatic <Static_the_posix>
{
public:
	static int* error_number ()
	{
		return Core::RuntimeGlobal::current ().error_number ();
	}

	static void raise (int signal)
	{
		Core::Thread* th = Core::Thread::current_ptr ();
		if (th) {
			Core::ExecDomain* ed = th->exec_domain ();
			if (ed) {
				if (Core::Signals::is_supported (signal))
					ed->raise (signal);
				else
					throw_BAD_PARAM ();
			}
			return;
		}
		unrecoverable_error (signal);
	}

	static void sigaction (int signal, const struct sigaction* act, struct sigaction* oldact)
	{
		throw_NO_IMPLEMENT ();
	}

	static void srand (unsigned seed)
	{
		Core::RuntimeGlobal::current ().srand (seed);
	}

	static int rand ()
	{
		return Core::RuntimeGlobal::current ().rand ();
	}

	static void append_path (CosNaming::Name& name, const IDL::String& path, bool absolute)
	{
		IDL::String translated;
		const IDL::String* ppath;
		if (Core::FileSystem::translate_path (path, translated))
			ppath = &translated;
		else
			ppath = &path;

		if (name.empty () && absolute && !Core::FileSystem::is_absolute (*ppath))
			name = get_current_dir_name ();
		Core::FileSystem::append_path (name, *ppath);
	}

	static CosNaming::Name get_current_dir_name ()
	{
		return Core::CurrentDir::current_dir ();
	}

	static void chdir (const IDL::String& path)
	{
		Core::CurrentDir::chdir (path);
	}

	static uint16_t fd_add (Access::_ptr_type access)
	{
		return (uint16_t)Core::FileDescriptors::fd_add (access);
	}

	static void close (unsigned fd)
	{
		Core::FileDescriptors::close (fd);
	}

	static size_t read (unsigned fd, void* p, size_t size)
	{
		return Core::FileDescriptors::read (fd, p, size);
	}

	static void write (unsigned fd, const void* p, size_t size)
	{
		Core::FileDescriptors::write (fd, p, size);
	}

	static FileSize seek (unsigned fd, const FileOff& offset, uint_fast16_t whence)
	{
		return Core::FileDescriptors::seek (fd, offset, whence);
	}

	static int_fast16_t fcntl (unsigned fd, int_fast16_t cmd, uintptr_t arg)
	{
		return Core::FileDescriptors::fcntl (fd, cmd, arg);
	}

	static void flush (unsigned fd)
	{
		Core::FileDescriptors::flush (fd);
	}

	static void dup2 (unsigned src, unsigned dst)
	{
		Core::FileDescriptors::dup2 (src, dst);
	}

	static bool isatty (unsigned fd)
	{
		return Core::FileDescriptors::isatty (fd);
	}

	static void ungetc (unsigned fd, int c)
	{
		Core::FileDescriptors::push_back (fd, c);
	}

	static bool ferror (unsigned fd)
	{
		return Core::FileDescriptors::ferror (fd);
	}

	static bool feof (unsigned fd)
	{
		return Core::FileDescriptors::feof (fd);
	}

	static void clearerr (unsigned fd)
	{
		Core::FileDescriptors::clearerr (fd);
	}

	static FileDescriptors get_inherited_files (unsigned std_creation_mask)
	{
		return Core::MemContext::current ().get_inherited_files (std_creation_mask);
	}

	static void sleep (TimeBase::TimeT period100ns)
	{
		if (!period100ns)
			Core::ExecDomain::reschedule ();
		else {
			Core::TimerEvent timer;
			timer.set (0, period100ns, 0);
			timer.wait ();
		}
	}

};

}

#endif
