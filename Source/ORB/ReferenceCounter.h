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
#ifndef NIRVANA_ORB_CORE_REFERENCECOUNTER_H_
#define NIRVANA_ORB_CORE_REFERENCECOUNTER_H_
#pragma once

#include <CORBA/Server.h>
#include "IDL/ObjectFactory_s.h"
#include <CORBA/ImplementationPseudo.h>
#include "LifeCycleNoCopy.h"

namespace CORBA {
namespace Internal {
namespace Core {

class ReferenceCounter :
	public LifeCycleNoCopy <ReferenceCounter>,
	public ImplementationPseudo <ReferenceCounter, CORBA::Internal::ReferenceCounter>
{
public:
	ReferenceCounter (ValueBase::_ptr_type impl) :
		ref_cnt_ (1),
		value_base_ (impl)
	{}

	void add_ref () NIRVANA_NOEXCEPT
	{
		assert (ref_cnt_);
		++ref_cnt_;
	}

	void remove_ref () NIRVANA_NOEXCEPT
	{
		assert (ref_cnt_);
		if (value_base_ && !--ref_cnt_) {
			try {
				ValueBase::_ptr_type p = value_base_;
				value_base_ = ValueBase::_nil ();
				p->_delete_object ();
			} catch (...) {
				assert (false); // TODO: Swallow exception or log
			}
		}
	}

	ULong refcount_value () const NIRVANA_NOEXCEPT
	{
		return ref_cnt_;
	}

private:
	ULong ref_cnt_; // We don't need the atomic counter here
	ValueBase::_ptr_type value_base_;
};

}
}
}

#endif
