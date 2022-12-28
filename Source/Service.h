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
#ifndef NIRVANA_CORE_SERVICE_H_
#define NIRVANA_CORE_SERVICE_H_
#pragma once

#include "SyncDomain.h"
#include "Synchronized.h"
#include "UserObject.h"

namespace Nirvana {
namespace Core {

/// The core object which works in the own SyncDomain but does not derive from CORBA::Object.
class Service :
	public UserObject // Created in the sync domain memory context
{
public:
	SyncDomain& sync_domain () NIRVANA_NOEXCEPT
	{
		return *sync_domain_;
	}

	MemContext& memory () NIRVANA_NOEXCEPT
	{
		return sync_domain_->mem_context ();
	}

protected:
	Service () :
		sync_domain_ (&SyncDomain::enter ())
	{}

	virtual ~Service ()
	{}

private:
	template <class> friend class Nirvana::Core::Ref;
	template <class> friend class CORBA::servant_reference;

	void _add_ref () NIRVANA_NOEXCEPT
	{
		ref_cnt_.increment ();
	}

	void _remove_ref () NIRVANA_NOEXCEPT
	{
		if (0 == ref_cnt_.decrement ()) {
			Synchronized _sync_frame (sync_domain (), nullptr);
			delete this;
		}
	}

private:
	RefCounter ref_cnt_;
	Ref <SyncDomain> sync_domain_;
};

}
}

#endif
