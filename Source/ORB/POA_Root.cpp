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

#include "POA_Root.h"
#include "Services.h"

using namespace Nirvana::Core;
using namespace CORBA;

namespace PortableServer {
namespace Core {

POA* POA_Root::root_ = nullptr;

POA::_ref_type POA_Root::get_root ()
{
	if (!root_) {
		Object::_ref_type svc = CORBA::Core::Services::bind (CORBA::Core::Services::RootPOA);
		POA::_ref_type adapter = POA::_narrow (svc);
		if (!adapter)
			throw INV_OBJREF ();
		root_ = static_cast <POA*> (&(POA::_ptr_type)adapter);
		return adapter;
	} else
		return POA::_ptr_type (root_);
}

void POA_Root::invoke (CORBA::Core::RequestInBase& request) NIRVANA_NOEXCEPT
{
	try {
		ExecDomain& ed = ExecDomain::current ();
		CoreRef <MemContext> memory (ed.mem_context_ptr ());
		const CORBA::Core::ProxyLocal* proxy = CORBA::Core::local2proxy (get_root ());
		POA_Base* adapter = get_implementation (proxy);
		assert (adapter);

		SYNC_BEGIN (proxy->sync_context (), nullptr);

		const ObjectKey& object_key = request.object_key ();
		for (const auto& name : object_key.adapter_path ()) {
			adapter = &adapter->find_child (name, true);
		}
		adapter->invoke (request, std::move (memory));

		_sync_frame.return_to_caller_context ();

		SYNC_END ();

	} catch (Exception& e) {
		request.set_exception (std::move (e));
	}
}

}
}
