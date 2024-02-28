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
#ifndef NIRVANA_NAMESERVICE_NAMINGCONTEXTDEFAULT_H_
#define NIRVANA_NAMESERVICE_NAMINGCONTEXTDEFAULT_H_
#pragma once

#include "NamingContextImpl.h"
#include <CORBA/Server.h>
#include <CORBA/CosNaming_s.h>
#include "../deactivate_servant.h"

namespace CosNaming {
namespace Core {

/// Default implementation of the NamingContex interface.
class NamingContextDefault :
	public CORBA::servant_traits <NamingContext>::Servant <NamingContextDefault>,
	public NamingContextImpl
{
public:
	bool _non_existent () const
	{
		return destroyed ();
	}

	static CORBA::servant_reference <NamingContextDefault> create ();

	void destroy ()
	{
		if (!bindings_.empty ())
			throw NotEmpty ();

		if (links_.empty ()) {
			NamingContextImpl::destroy ();
			Nirvana::Core::deactivate_servant (this);
		}
	}

	virtual NamingContext::_ptr_type this_context () override
	{
		return _this ();
	}

	virtual void add_link (const NamingContextImpl& parent) override;
	virtual bool remove_link (const NamingContextImpl& parent) override;
	virtual bool is_cyclic (ContextSet& parents) const override;

private:
	ContextSet links_;
};

inline
NamingContext::_ref_type NamingContextImpl::new_context ()
{
	return NamingContextDefault::create ()->_this ();
}

}
}

#endif
