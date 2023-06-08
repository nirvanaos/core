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

using namespace CORBA;
using namespace CORBA::Core;

namespace PortableServer {
namespace Core {

POA_DGC::POA_DGC ()
{
	servant_reference <PolicyMapShared> pols;
	if (DGC_DEFAULT == DGC_policy ()) {
		if (object_policies_) {
			pols = make_reference <PolicyMapShared> (std::ref (*object_policies_));
			pols->insert <FT::HEARTBEAT_ENABLED_POLICY> (true);
		} else if (root_)
			pols = root_->default_DGC_policies ();
		else {
			pols = CORBA::make_reference <CORBA::Core::PolicyMapShared> ();
			pols->insert <FT::HEARTBEAT_ENABLED_POLICY> (true);
		}
	} else
		pols = object_policies_;
	DGC_policies_ = std::move (pols);
}

PolicyMapShared* POA_DGC::get_policies (unsigned flags) const NIRVANA_NOEXCEPT
{
	if (flags & CORBA::Core::Reference::GARBAGE_COLLECTION)
		return DGC_policies_;
	else
		return object_policies_;
}

}
}
