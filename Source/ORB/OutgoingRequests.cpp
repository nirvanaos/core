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

using namespace std;
using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

StaticallyAllocated <OutgoingRequests::RequestMap> OutgoingRequests::map_;
StaticallyAllocated <OutgoingRequests::IdGen> OutgoingRequests::last_id_;

void OutgoingRequests::reply (uint32_t request_id, CoreRef <StreamIn>&& data)
{
	RequestMap::NodeVal* p = map_->find_and_delete (request_id);
	assert (p);
	if (p) {
		p->value ().request->reply (move (data));
		map_->release_node (p);
	}
}

}
}