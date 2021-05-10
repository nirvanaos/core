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
#ifndef NIRVANA_CORE_STARTUP_H_
#define NIRVANA_CORE_STARTUP_H_

#include <CORBA/CORBA.h>
#include "Runnable.h"
#include <exception>

namespace Nirvana {

namespace Legacy {
namespace Core {
class Executable;
}
}

namespace Core {

class NIRVANA_NOVTABLE Startup : public ImplStatic <Runnable>
{
public:
	Startup (int argc, char* argv [], char* envp []);

	~Startup ()
	{}

	virtual void run ();
	virtual void on_exception () NIRVANA_NOEXCEPT;
	virtual void on_crash (int error_code) NIRVANA_NOEXCEPT;

	void check () const;

	int ret () const NIRVANA_NOEXCEPT
	{
		return ret_;
	}

	Nirvana::Legacy::Core::Executable* executable () const
	{
		return executable_;
	}

	bool process () const
	{
		return process_;
	}

protected:
	int argc_;
	char** argv_;
	char** envp_;
	int ret_;

private:
	bool process_;
	Nirvana::Legacy::Core::Executable* executable_;
	std::exception_ptr exception_;
	CORBA::SystemException::Code exception_code_;
};

}
}

#endif
