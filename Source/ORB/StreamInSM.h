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
#ifndef NIRVANA_ORB_CORE_STREAMINSM_H_
#define NIRVANA_ORB_CORE_STREAMINSM_H_
#pragma once

#include "StreamIn.h"

namespace CORBA {
namespace Core {

/// \brief Shared memory input stream
class NIRVANA_NOVTABLE StreamInSM :
	public StreamIn,
	public Nirvana::Core::SharedObject
{
public:
	virtual void read (size_t align, size_t element_size, size_t CDR_element_size,
		size_t element_count, void* buf) override;
	virtual void* read (size_t align, size_t element_size, size_t CDR_element_size,
		size_t element_count, size_t& size) override;
	virtual void set_size (size_t size) override;
	virtual size_t end () override;
	virtual size_t position () const override;
	virtual size_t chunk_tail () const override;
	virtual CORBA::Long skip_chunks () override;

protected:
	StreamInSM (void* mem) noexcept;
	StreamInSM (const StreamInSM&) = delete;
	StreamInSM (StreamInSM&&) = delete;

	~StreamInSM ();

private:
	struct BlockHdr
	{
		BlockHdr* next;
		size_t size;
	};

	struct Segment
	{
		Segment* next;
		void* pointer;
		size_t size;
	};

	struct StreamHdr : BlockHdr
	{
		Segment* segments;
	};

	void next_block ();
	const Segment* get_segment (size_t align, size_t size);
	inline void physical_read (size_t& align, size_t& size, void* buf);
	inline static void release_block (BlockHdr* block);
	void inc_position (size_t align, size_t cb);

private:
	BlockHdr* cur_block_;
	const uint8_t* cur_ptr_;
	Segment* segments_;
	size_t position_;
	size_t chunk_end_;
};

}
}

#endif
