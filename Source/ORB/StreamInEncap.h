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
#ifndef NIRVANA_ORB_CORE_STREAMINENCAP_H_
#define NIRVANA_ORB_CORE_STREAMINENCAP_H_
#pragma once

#include "StreamIn.h"

namespace CORBA {
namespace Core {

/// Input stream for encapsulated data
class NIRVANA_NOVTABLE StreamInEncap :
	public CORBA::Core::StreamIn
{
public:
	virtual void read (size_t align, size_t size, void* buf) override;
	virtual void* read (size_t align, size_t& size) override;
	virtual void set_size (size_t size) override;
	virtual size_t end () override;

protected:
	StreamInEncap (const IDL::Sequence <Octet>& data) NIRVANA_NOEXCEPT :
		cur_ptr_ (data.data ()),
		end_ (data.data () + data.size ())
	{}

private:
	typedef std::pair <const Octet*, const Octet*> Range;

	void prepare (size_t align, size_t size, Range& range) const;
	void read (const Range& range, void* buf) NIRVANA_NOEXCEPT;

private:
	const Octet* cur_ptr_;
	const Octet* end_;
};

}
}

#endif