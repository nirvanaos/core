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
#include "OutgoingRequests.h"
#include "RequestOut.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

StaticallyAllocated <OutgoingRequests> OutgoingRequests::singleton_;

servant_reference <RequestOut> OutgoingRequests::remove_request_internal (uint32_t request_id) NIRVANA_NOEXCEPT
{
	RequestMap::NodeVal* p = map_.find_and_delete (request_id);
	servant_reference <RequestOut> ret;
	if (p) {
		ret = std::move (p->value ().request);
		map_.release_node (p);
	}
	return ret;
}

}
}
