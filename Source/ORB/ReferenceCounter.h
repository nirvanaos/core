/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
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
#ifndef NIRVANA_ORB_CORE_REFERENCECOUNTER_H_
#define NIRVANA_ORB_CORE_REFERENCECOUNTER_H_

#include <CORBA/ReferenceCounter_s.h>
#include <CORBA/DynamicServant_s.h>
#include <CORBA/ImplementationPseudo.h>
#include "../AtomicCounter.h"
#include <limits>

namespace CORBA {
namespace Nirvana {
namespace Core {

class ReferenceCounter :
	public LifeCycleNoCopy <ReferenceCounter>,
	public ImplementationPseudo <ReferenceCounter, CORBA::Nirvana::ReferenceCounter>
{
public:
	ReferenceCounter (DynamicServant_ptr dynamic) :
		dynamic_ (dynamic)
	{}

	void _add_ref ()
	{
		ref_cnt_.increment ();
	}

	void _remove_ref ()
	{
		if (!ref_cnt_.decrement () && dynamic_) {
			try {
				dynamic_->_delete ();
			} catch (...) {
				assert (false); // TODO: Swallow exception or log
			}
		}
	}

	ULong _refcount_value () const
	{
		::Nirvana::Core::RefCounter::IntegralType ucnt = ref_cnt_;
		return ucnt > std::numeric_limits <ULong>::max () ? std::numeric_limits <ULong>::max () : (ULong)ucnt;
	}

private:
	::Nirvana::Core::RefCounter ref_cnt_;
	DynamicServant_ptr dynamic_;
};

}
}
}

#endif
