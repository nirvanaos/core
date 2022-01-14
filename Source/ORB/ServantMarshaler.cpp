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
#include "ServantMarshaler.h"

namespace CORBA {
namespace Internal {
namespace Core {

using namespace ::Nirvana;

ServantMarshaler::Block* ServantMarshaler::clear_block (Tag* p)
{
	for (;;) {
		switch (*p) {
			case RT_END:
				return nullptr;

			case RT_MEMORY:
			{
				RecMemory* rec = (RecMemory*)(p + 1);
				if (rec->p)
					memory_->heap ().release (rec->p, rec->size);
				p = (Tag*)(rec + 1);
			}
			break;

			case RT_INTERFACE:
			{
				RecInterface* rec = (RecInterface*)(p + 1);
				interface_release (rec->p);
				p = (Tag*)(rec + 1);
			}
			break;

			default:
				return (Block*)(*p);
		}
	}
}

void* ServantMarshaler::add_record (RecordType tag, size_t record_size)
{
	if ((block_end (cur_ptr_) - cur_ptr_) * sizeof (Tag) < (record_size + sizeof (Tag))) {
		size_t bs = sizeof (Block);
		Block* next = (Block*)memory_->heap ().allocate (nullptr, bs, 0);
		*cur_ptr_ = (Tag)next;
		cur_ptr_ = *next;
	}
	*cur_ptr_ = tag;
	void* rec = cur_ptr_ + 1;
	cur_ptr_ = (Tag*)((Octet*)rec + record_size);
	*cur_ptr_ = RT_END;
	return rec;
}

void ServantMarshaler::move_next (size_t record_size)
{
	cur_ptr_ = (Tag*)((Octet*)cur_ptr_ + record_size + sizeof (Tag));
	Tag next = *cur_ptr_;
	if (next == RT_END || next > RT_INTERFACE) {
		// Next block
		if (cur_ptr_ >= block_ + countof (block_) || cur_ptr_ < block_) {
			Block* this_block = (Block*)round_down (cur_ptr_, BLOCK_SIZE);
			memory_->heap ().release (this_block, BLOCK_SIZE);
		}
		block_ [0] = next;
	}
}

}
}
}
