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
#include "pch.h"
#include "Security.h"
#include "ExecDomain.h"

namespace Nirvana {
namespace Core {

const Security::Context& Security::Context::current () noexcept
{
	const Thread* pt = Thread::current_ptr ();
	if (pt) {
		const ExecDomain* ped = pt->exec_domain ();
		if (ped) {
			const Security::Context& impersonated = ped->impersonation_context ();
			if (!impersonated.empty ())
				return impersonated;
		}
	}
	return prot_domain_context ();
}

}
}
