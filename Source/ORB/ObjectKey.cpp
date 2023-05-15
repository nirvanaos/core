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

ObjectKey::ObjectKey (const IOP::ObjectKey& object_key)
{
	ImplStatic <StreamInEncap> stm (std::ref (object_key), true);
	unmarshal (stm);
	if (stm.end () != 0)
		throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
}

void ObjectKey::unmarshal (StreamIn& in)
{
	size_t size = in.read_size ();
	adapter_path_.resize (size);
	for (auto& name : adapter_path_) {
		in.read_string (name);
	}
	in.read_seq (object_id_);
}

}
}
