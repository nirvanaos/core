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
#include "ObjectKey.h"
#include "StreamInEncap.h"
#include <Nirvana/Hash.h>
#include "POA_Root.h"

using namespace Nirvana::Core;
using namespace CORBA::Core;

namespace PortableServer {
namespace Core {

ObjectKey::ObjectKey (POA_Base& adapter) :
	object_id_ (adapter.generate_object_id ())
{
	adapter.get_path (adapter_path_);
}

ObjectKey::ObjectKey (const POA_Base& adapter, const ObjectId& oid) :
	object_id_ (oid)
{
	adapter.get_path (adapter_path_);
}

ObjectKey::ObjectKey (const POA_Base& adapter, ObjectId&& oid) :
	object_id_ (std::move (oid))
{
	adapter.get_path (adapter_path_);
}

ObjectKey::ObjectKey (const IOP::ObjectKey& object_key)
{
	if (object_key.size () < 8)
		object_id_ = object_key;
	else {
		ImplStatic <StreamInEncap> stm (std::ref (object_key), true);
		stm.read_seq (object_id_);
		size_t size = stm.read_size ();
		adapter_path_.resize (size);
		for (auto& name : adapter_path_) {
			stm.read_string (name);
		}
		if (stm.end () != 0)
			throw CORBA::INV_OBJREF ();
	}
}

ObjectKey::operator IOP::ObjectKey () const
{
	if (adapter_path_.empty () && object_id_.size () < 4)
		return object_id_;
	else {
		ImplStatic <CORBA::Core::StreamOutEncap> stm (true);
		stm.write_seq (object_id_);
		stm.write_size (adapter_path_.size ());
		for (const auto& name : adapter_path_) {
			stm.write_string_c (name);
		}
		return std::move (stm.data ());
	}
}

ObjectId ObjectKey::get_object_id (const IOP::ObjectKey& object_key)
{
	if (object_key.size () < 8)
		return object_key;
	else {
		size_t size = *(uint32_t*)object_key.data ();
		return ObjectId (object_key.data () + 4, object_key.data () + size + 4);
	}
}

AdapterPath ObjectKey::get_adapter_path (const IOP::ObjectKey& object_key)
{
	ImplStatic <StreamInEncap> stm (std::ref (object_key), true);
	size_t size = stm.read_size ();
	stm.read (1, 1, 1, size, nullptr);
	size = stm.read_size ();
	AdapterPath path (size);
	for (auto& name : path) {
		stm.read_string (name);
	}
	if (stm.end () != 0)
		throw CORBA::INV_OBJREF ();
	return path;
}

}
}
