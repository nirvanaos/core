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
#include "pch.h"
#include "HeapUser.h"

namespace Nirvana {
namespace Core {

bool HeapUser::cleanup ()
{
	bool empty = true;
	part_list_ = nullptr;
	while (BlockList::NodeVal* node = block_list_.delete_min ()) {
		const MemoryBlock block = node->value ();
		block_list_.release_node (node);
		if (block.is_large_block ()) {
			Port::Memory::release (block.begin (), block.large_block_size ());
			empty = false;
		} else {
			Directory& dir = block.directory ();
			if (!dir.empty ())
				empty = false;
			Port::Memory::release (&dir, sizeof (Directory) + partition_size ());
		}
	}
	return empty;
}

}
}
