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
#include "StreamInEncap.h"
#include "../ExecDomain.h"
#include "../virtual_copy.h"

using namespace Nirvana;
using namespace Nirvana::Core;

namespace CORBA {
namespace Core {

StreamInEncap::StreamInEncap (const Octet* begin, const Octet* end, bool skip_endian) :
	cur_ptr_ (begin),
	begin_ (begin),
	end_ (end)
{
	assert ((uintptr_t)begin % 8 == 0);
	if (!skip_endian) {
		if (begin_ >= end_)
			throw MARSHAL (MARSHAL_MINOR_FEWER);
		little_endian (*begin_);
		cur_ptr_ = begin_ + 1;
	}
}

void StreamInEncap::prepare (size_t align, size_t size, Range& range) const
{
	const Octet* p = round_up (cur_ptr_, align);
	const Octet* end = p + size;
	if (end > end_)
		throw MARSHAL (MARSHAL_MINOR_FEWER);
	range.first = p;
	range.second = end;
}

void StreamInEncap::read (const Range& range, size_t element_size, size_t CDR_element_size,
	size_t element_count, void* buf) noexcept
{
	if (element_count == 1 || element_size == CDR_element_size)
		virtual_copy (range.first, range.second, (Octet*)buf);
	else {
		const Octet* src = range.first;
		Octet* dst = (Octet*)buf;
		while (element_count--) {
			real_copy (src, src + CDR_element_size, dst);
			dst += element_size;
			src += CDR_element_size;
		}
	}
	cur_ptr_ = range.second;
}

void StreamInEncap::read (size_t align, size_t element_size, size_t CDR_element_size,
	size_t element_count, void* buf)
{
	size_t size = CDR_element_size * element_count;
	if (!size)
		return;

	Range range;
	prepare (align, size, range);
	if (buf)
		read (range, element_size, CDR_element_size, element_count, buf);
	else
		cur_ptr_ = range.second;
}

void* StreamInEncap::read (size_t align, size_t element_size, size_t CDR_element_size,
	size_t element_count, size_t& size)
{
	size = element_size * element_count;
	if (!size)
		return nullptr;

	Range range;
	prepare (align, CDR_element_size * element_count, range);
	void* p = MemContext::current ().heap ().allocate (nullptr, size, 0);
	read (range, element_size, CDR_element_size, element_count, p);
	return p;
}

void StreamInEncap::set_size (size_t size)
{
	assert (false);
}

size_t StreamInEncap::end ()
{
	return end_ - cur_ptr_;
}

size_t StreamInEncap::position () const
{
	return cur_ptr_ - begin_;
}

}
}
