#include "ServantMarshaler.h"

namespace CORBA {
namespace Nirvana {
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
					memory_.release (rec->p, rec->size);
				p = (Tag*)(rec + 1);
			}
			break;

			case RT_OBJECT:
			case RT_TYPE_CODE:
			{
				RecObject* rec = (RecObject*)(p + 1);
				release (rec->p);
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
		Block* next = (Block*)memory_.allocate (nullptr, bs, 0);
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
	if (next == RT_END || next > RT_TYPE_CODE) {
		// Next block
		if (cur_ptr_ >= block_ + countof (block_) || cur_ptr_ < block_) {
			Block* this_block = (Block*)round_down (cur_ptr_, BLOCK_SIZE);
			memory_.release (this_block, BLOCK_SIZE);
		}
		block_ [0] = next;
	}
}

}
}
}
