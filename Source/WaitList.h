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
#ifndef NIRVANA_CORE_WAITLIST_H_
#define NIRVANA_CORE_WAITLIST_H_

#include "ExecDomain.h"
#include <exception>

namespace Nirvana {
namespace Core {

class ExecDomain;

class WaitListImpl :
	public CoreObject
{
public:
	WaitListImpl ();

	void wait ();
	void on_exception () NIRVANA_NOEXCEPT;
	void finish () NIRVANA_NOEXCEPT;

private:
#ifdef _DEBUG
	bool finished_;
#endif
	//ExecDomain& worker_;
	ExecDomain::Impl* wait_list_;
	std::exception_ptr exception_;
};

typedef ImplDynamicSync <WaitListImpl> WaitList;

}
}

#endif
