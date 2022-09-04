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

#include "StreamOut.h"
#include "SharedObject.h"

namespace CORBA {
namespace Core {
class StreamIn;
}
}

namespace PortableServer {

typedef CORBA::OctetSeq ObjectId;

namespace Core {

typedef IDL::Sequence <IDL::String> AdapterPath;

/// ObjectKey internal structure.
class ObjectKey
{
public:
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
		out.write_size (adapter_path_.size ());
		for (const auto& name : adapter_path_) {
			out.write_string (const_cast <IDL::String&> (name), false);
		}
		out.write_seq (const_cast <ObjectId&> (object_id_), false);
	}

	void unmarshal (CORBA::Core::StreamIn& in);
	void unmarshal (const IOP::ObjectKey& object_key);

private:
	AdapterPath adapter_path_;
	ObjectId object_id_;
};

class ObjectKeyShared :
	public Nirvana::Core::SharedObject
{
public:
	const AdapterPath& adapter_path () const NIRVANA_NOEXCEPT
	{
		return key_.adapter_path ();
	}

	const ObjectId& object_id () const NIRVANA_NOEXCEPT
	{
		return key_.object_id ();
	}

	void marshal (CORBA::Core::StreamOut& out) const
	{
		key_.marshal (out);
	}

	operator const ObjectKey& () const NIRVANA_NOEXCEPT
	{
		return key_;
	}

protected:
	ObjectKeyShared (ObjectKey&& key) NIRVANA_NOEXCEPT;
	~ObjectKeyShared ();

private:
	ObjectKey key_;
	Nirvana::Core::CoreRef <Nirvana::Core::MemContext> memory_;
};

typedef Nirvana::Core::ImplDynamic <ObjectKeyShared> ObjectKeyImpl;
typedef Nirvana::Core::CoreRef <ObjectKeyImpl> ObjectKeyRef;

}
}

#endif
