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
#include "ProxyObjectImplicit.h"
#include "POA_Root.h"

using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

ProxyObjectImplicit::~ProxyObjectImplicit ()
{
	PortableServer::Core::ServantBase& servant = core_servant ();

	try {
		SyncContext& adapter_context = local2proxy (Services::bind (Services::RootPOA))->sync_context ();

		// We don't need exception handling here because on_destruct_implicit is noexcept.
		// So we don't use SYNC_BEGIN/SYNC_END and just create the sync frame.
		Nirvana::Core::Synchronized _sync_frame (adapter_context, nullptr);

		if (references_.empty ()) {
			// If the set is empty, object can have one reference stored in reference_.
			ReferenceLocal* ref = reference_.load ();
			if (ref)
				ref->on_destruct_implicit (servant);
		} else {
			// If the set is not empty, it contains all the references include stored in reference_.
			for (void* p : references_) {
				reinterpret_cast <ReferenceLocal*> (p)->on_destruct_implicit (servant);
			}
			references_.clear ();
		}
	} catch (...) {
		assert (false);
	}
}

// Called in the servant synchronization context.
// Note that sync context may be out of synchronization domain
// for the stateless objects.
void ProxyObjectImplicit::add_ref_1 ()
{
	Base::add_ref_1 ();
	if (adapter_) {
		// Do only once
		PortableServer::POA::_ref_type tmp = std::move (adapter_);
		assert (!get_reference_local ());
		if (!get_reference_local ()) // Who knows...
			PortableServer::Core::POA_Base::implicit_activate (tmp, *this);
	}
}

}
}
