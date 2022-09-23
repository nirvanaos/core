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
#ifndef NIRVANA_ORB_CORE_DOMAIN_H_
#define NIRVANA_ORB_CORE_DOMAIN_H_
#pragma once

#include "CORBA/CORBA.h"
#include "../AtomicCounter.h"
#include "../Service.h"
#include "GarbageCollector.h"

namespace CORBA {
namespace Core {

class NIRVANA_NOVTABLE Domain : public SyncGC
{
	template <class D> friend class CORBA::servant_reference;

public:
	Nirvana::Core::Service& service () const NIRVANA_NOEXCEPT
	{
		return *service_;
	}

protected:
	Domain (Nirvana::Core::Service& service) :
		service_ (&service)
	{}

	virtual void _add_ref () NIRVANA_NOEXCEPT override;
	virtual void _remove_ref () NIRVANA_NOEXCEPT override;
	virtual void destroy () NIRVANA_NOEXCEPT = 0;

private:
	servant_reference <Nirvana::Core::Service> service_;

	class RefCnt : public Nirvana::Core::AtomicCounter <false>
	{
	public:
		RefCnt () :
			Nirvana::Core::AtomicCounter <false> (1)
		{}

		RefCnt (const RefCnt&) = delete;
		RefCnt& operator = (const RefCnt&) = delete;
	};

	RefCnt ref_cnt_;
};

}
}

#endif
