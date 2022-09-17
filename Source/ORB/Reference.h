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
#ifndef NIRVANA_ORB_CORE_REFERENCE_H_
#define NIRVANA_ORB_CORE_REFERENCE_H_
#pragma once

#include "ProxyManager.h"
#include "RefCntProxy.h"

namespace CORBA {
namespace Core {

class ReferenceLocal;

class Reference :
	public ProxyManager
{
public:
	Reference (const IDL::String& primary_iid, bool garbage_collection) :
		CORBA::Core::ProxyManager (primary_iid),
		ref_cnt_ (1),
		garbage_collection_ (garbage_collection)
	{}

	Reference (const ProxyManager& proxy, bool garbage_collection) :
		CORBA::Core::ProxyManager (proxy),
		ref_cnt_ (1),
		garbage_collection_ (garbage_collection)
	{}

	/// Marshal reference to stream.
	/// 
	/// \param out Stream.
	virtual void marshal (StreamOut& out) const = 0;

	virtual ReferenceRef get_reference () override;

	/// Dymamic cast to ReferenceLocal. We do not use RTTI and dynamic_cast<>.
	virtual ReferenceLocal* local_reference ();

	RefCntProxy::IntegralType _refcount_value () const NIRVANA_NOEXCEPT
	{
		return ref_cnt_.load ();
	}

protected:
	RefCntProxy ref_cnt_;
	bool garbage_collection_;
};

}
}

#endif
