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
struct ObjectKey
{
	AdapterPath adapter_path;
	ObjectId object_id;

	void marshal (CORBA::Core::StreamOut& out) const
	{
		out.write_size (adapter_path.size ());
		for (const auto& name : adapter_path) {
			out.write_string (const_cast <IDL::String&> (name), false);
		}
		out.write_seq (const_cast <ObjectId&> (object_id), false);
	}

	void unmarshal (CORBA::Core::StreamIn& in);
	void unmarshal (const IOP::ObjectKey& object_key);
};


}
}

#endif
