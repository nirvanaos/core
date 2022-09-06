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
#include "ExecDomain.h"

using namespace Nirvana::Core;
using namespace CORBA::Core;

namespace PortableServer {
namespace Core {

void ObjectKey::unmarshal (StreamIn& in)
{
	size_t size = in.read_size ();
	adapter_path_.resize (size);
	for (auto& name : adapter_path_) {
		in.read_string (name);
	}
	in.read_seq (object_id_);
}

void ObjectKey::unmarshal (const IOP::ObjectKey& object_key)
{
	ImplStatic <StreamInEncap> stm (std::ref (object_key));
	unmarshal (stm);
	if (stm.end () != 0)
		throw CORBA::MARSHAL (StreamIn::MARSHAL_MINOR_MORE);
}

ObjectKeyShared::ObjectKeyShared (ObjectKey&& key) NIRVANA_NOEXCEPT :
	key_ (std::move (key)),
	memory_ (&MemContext::current ())
{}

ObjectKeyShared::~ObjectKeyShared ()
{
	ExecDomain& ed = ExecDomain::current ();
	try {
		ed.mem_context_push (memory_);
		{
			ObjectKey tmp (std::move (key_));
		}
		ed.mem_context_pop ();
	} catch (...) {}
}

}
}
