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
#ifndef NIRVANA_ORB_CORE_OBJECTKEY_H_
#define NIRVANA_ORB_CORE_OBJECTKEY_H_
#pragma once

#include <CORBA/Server.h>
#include <CORBA/IOP.h>
#include "StreamOutEncap.h"

namespace CORBA {
namespace Core {

class StreamIn;

}
}

namespace PortableServer {
namespace Core {

typedef IDL::Sequence <IDL::String> AdapterPath;

class POA_Base;

/// Object key internal structure.
class ObjectKey
{
public:
	ObjectKey ()
	{}

	ObjectKey (POA_Base& adapter);
	ObjectKey (const POA_Base& adapter, const ObjectId& oid);

	const AdapterPath& adapter_path () const NIRVANA_NOEXCEPT
	{
		return adapter_path_;
	}

	AdapterPath& adapter_path () NIRVANA_NOEXCEPT
	{
		return adapter_path_;
	}

	const ObjectId& object_id () const NIRVANA_NOEXCEPT
	{
		return object_id_;
	}

	void object_id (const ObjectId& oid)
	{
		object_id_ = oid;
	}

	void object_id (ObjectId&& oid) NIRVANA_NOEXCEPT
	{
		object_id_ = std::move (oid);
	}

	void marshal (CORBA::Core::StreamOut& out) const
	{
		if (!adapter_path_.empty ()) {
			Nirvana::Core::ImplStatic <CORBA::Core::StreamOutEncap> encap;
			encap.write_size (adapter_path_.size ());
			for (const auto& name : adapter_path_) {
				encap.write_string_c (name);
			}
			encap.write_seq (object_id_);
			out.write_seq (encap.data ());
		} else {
			out.write_size (object_id_.size () + 8);
			out.write_size (0);
			out.write_seq (object_id_);
		}
	}

	void unmarshal (const IOP::ObjectKey& object_key);
	void unmarshal (CORBA::Core::StreamIn& in);

	size_t hash () const NIRVANA_NOEXCEPT;

	bool operator == (const ObjectKey& other) const NIRVANA_NOEXCEPT;

private:
	AdapterPath adapter_path_;
	ObjectId object_id_;
};

}
}

namespace std {

template <>
struct hash <PortableServer::Core::ObjectKey>
{
	size_t operator () (const PortableServer::Core::ObjectKey& key) const
	{
		return key.hash ();
	}
};

}

#endif
