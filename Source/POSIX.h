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
#include "ExecDomain.h"
#include "TimerEvent.h"
#include "append_path.h"
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

	static IDL::String get_current_dir ()
	{
		return CosNaming::Core::NameService::to_string_unchecked (Core::CurrentDir::current_dir ());
	}

	static void chdir (const IDL::String& path)
	{
		Core::CurrentDir::chdir (path);
	}

	static uint16_t open (const IDL::String& path, unsigned oflag, unsigned mode)
	{
		if (!(oflag & O_CREAT))
			mode = 0;
		// Get full path name
		CosNaming::Name name;
		Core::append_path (name, path, true);
		// Remove root name
		name.erase (name.begin ());
		// Get file system root
		Nirvana::Dir::_ref_type root = Nirvana::Dir::_narrow (name_service ()->resolve (CosNaming::Name ()));
		// Open file
		Nirvana::Access::_ref_type access = root->open (name, oflag & ~O_DIRECT, mode);
		return (uint16_t)Core::FileDescriptors::fd_add (access);
	}

	uint16_t mkostemps (CharPtr tpl, unsigned suffix_len, unsigned flags)
	{
		CosNaming::Name dir_name;
		IDL::String file;
		size_t tpl_len;
		{
			IDL::String tpl_path (tpl);
			tpl_len = tpl_path.size ();
			Core::append_path (dir_name, tpl_path, true);
			CosNaming::Name file_name;
			file_name.push_back (std::move (dir_name.back ()));
			dir_name.pop_back ();
			file = CosNaming::Core::NameService::to_string_unchecked (file_name);
		}

		Nirvana::AccessBuf::_ref_type access = Nirvana::AccessBuf::_downcast (
			Nirvana::Dir::_narrow (name_service ()->resolve (dir_name))->
			mkostemps (file, (uint16_t)suffix_len, (uint16_t)flags, 0)->_to_value ());

		unsigned fd = Core::FileDescriptors::fd_add (access);
		size_t src_end = file.size () - suffix_len;
		size_t src_begin = src_end - 6;
		const char* src = file.c_str ();
		Nirvana::real_copy (src + src_begin, src + src_end, tpl + tpl_len - suffix_len - 6);
		return (uint16_t)fd;
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

	static void unlink (const IDL::String& path)
	{
		CosNaming::Name name;
		Core::append_path (name, path, true);
		name_service ()->unbind (name);
	}

	static void rmdir (const IDL::String& path)
	{
		CosNaming::Name name;
		Core::append_path (name, path, true);
		auto ns = name_service ();
		Nirvana::Dir::_ref_type dir = Nirvana::Dir::_narrow (ns->resolve (name));
		if (dir) {
			dir->destroy ();
			dir = nullptr;
			ns->unbind (name);
		} else
			throw_BAD_PARAM (make_minor_errno (ENOTDIR));
	}

	static void mkdir (const IDL::String& path, unsigned mode)
	{
		CosNaming::Name name;
		Core::append_path (name, path, true);
		// Remove root name
		name.erase (name.begin ());
		// Get file system root
		Nirvana::Dir::_ref_type root = Nirvana::Dir::_narrow (name_service ()->resolve (CosNaming::Name ()));
		// Create directory
		if (!root->mkdir (name, mode))
			throw_BAD_PARAM (make_minor_errno (EEXIST));
	}

	void stat (const IDL::String& path, FileStat& st)
	{
		CosNaming::Name name;
		Core::append_path (name, path, true);
		Nirvana::DirItem::_narrow (name_service ()->resolve (name))->stat (st);
	}

private:
	static CosNaming::NamingContextExt::_ref_type name_service ()
	{
		return CosNaming::NamingContextExt::_narrow (CORBA::Core::Services::bind (
			CORBA::Core::Services::NameService));
	}
};

}

#endif
