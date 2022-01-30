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
#ifndef NIRVANA_LEGACY_CORE_PROCESS_H_
#define NIRVANA_LEGACY_CORE_PROCESS_H_
#pragma once

#include "Executable.h"
#include "ExecDomain.h"
#include "MemContextProcess.h"
#include "Console.h"

namespace Nirvana {
namespace Legacy {
namespace Core {

/// Legacy process.
class NIRVANA_NOVTABLE Process :
	public Nirvana::Core::CoreObject,
	public Executable,
	public Nirvana::Core::Runnable
{
public:
	void spawn ()
	{
		Nirvana::Core::ExecDomain::start_legacy_thread (*this, mem_context_);
	}

	int ret () const
	{
		return ret_;
	}

protected:
	Process (const Nirvana::Core::StringView& file,
		const std::vector <Nirvana::Core::StringView>& argv,
		const std::vector <Nirvana::Core::StringView>& envp) :
		Executable (file),
		ret_ (-1)
	{
		copy_vec (argv, argv_);
		copy_vec (envp, envp_);
	}

	~Process ()
	{}

	virtual void run ();
	virtual void on_exception () NIRVANA_NOEXCEPT;
	virtual void on_crash (int error_code) NIRVANA_NOEXCEPT;

private:
	static void copy_vec (const std::vector <Nirvana::Core::StringView>& src, std::vector <std::string>& dst);
	static void copy_vec (std::vector <std::string>& src, std::vector <char*>& dst);

private:
	Nirvana::Core::ImplStatic <MemContextProcess> mem_context_;
	Nirvana::Core::Console console_;
	std::vector <std::string> argv_, envp_;
	int ret_;
};

}
}
}

#endif
