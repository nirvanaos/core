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
#ifndef NIRVANA_ORB_CORE_REFERENCEBASE_H_
#define NIRVANA_ORB_CORE_REFERENCEBASE_H_
#pragma once

#include "ObjectKey.h"
#include "../LockableRef.h"
#include "../Chrono.h"

namespace CORBA {
namespace Core {

/// Local reference. Holds the object key.
class ReferenceLocal
{
public:
	/// Called from the POA synchronization domain.
	void object_key (PortableServer::Core::ObjectKeyRef&& key) NIRVANA_NOEXCEPT
	{
		object_key_ = std::move (key);
	}

	PortableServer::Core::ObjectKeyRef object_key () const NIRVANA_NOEXCEPT
	{
		return object_key_.get ();
	}

	/// The unique stamp is assigned when object reference is created.
	/// The timestamp together with the ProxyObject pointer let to create
	/// the unique system id for the reference.
	typedef Nirvana::Core::AtomicCounter <false>::IntegralType Timestamp;

	Timestamp timestamp () const NIRVANA_NOEXCEPT
	{
		return timestamp_;
	}

	static void initialize () NIRVANA_NOEXCEPT
	{
		next_timestamp_.construct ((Timestamp)(Nirvana::Core::Chrono::UTC ().time () / TimeBase::SECOND));
	}

protected:
	ReferenceLocal () :
		timestamp_ (next_timestamp_->increment_seq ())
	{}

	void marshal (Internal::String_in primary_iid, StreamOut& out) const;

protected:
	Nirvana::Core::LockableRef <PortableServer::Core::ObjectKeyImpl> object_key_;

private:
	Timestamp timestamp_;
	static Nirvana::Core::StaticallyAllocated <Nirvana::Core::AtomicCounter <false> > next_timestamp_;
};

}
}

#endif
