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
#ifndef NIRVANA_ORB_CORE_REFCNT_H_
#define NIRVANA_ORB_CORE_REFCNT_H_
#pragma once

#include <CORBA/Server.h>
#include <CORBA/RefCnt_s.h>
#include "../MemContextObject.h"
#include "../AtomicCounter.h"

namespace CORBA {
namespace Core {

class RefCnt :
	public servant_traits <CORBA::Internal::RefCnt>::Servant <RefCnt>,
	public LifeCycleNoCopy <RefCnt>,
	public Nirvana::Core::MemContextObject
{
public:
	void add_ref () noexcept
	{
		ref_cnt_.increment ();
	}

	void remove_ref () noexcept
	{
		if (!ref_cnt_.decrement ())
			delete_object ();
	}

	uint32_t refcount_value () const noexcept
	{
		return ref_cnt_;
	}

	RefCnt (CORBA::Internal::DynamicServant::_ptr_type deleter) :
		deleter_ (deleter)
	{
		if (!deleter)
			throw BAD_PARAM ();
	}

	~RefCnt ()
	{
		// Delete leaked object on MemContextUser cleanup
		delete_object ();
	}

private:
	void delete_object () noexcept;

private:
	Nirvana::Core::RefCounter ref_cnt_;
	CORBA::Internal::DynamicServant::_ptr_type deleter_;
};

}
}

#endif
