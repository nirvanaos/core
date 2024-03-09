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
#include "TLS.h"

namespace Nirvana {
namespace Core {

TLS::BitmapWord TLS::bitmap_ [BITMAP_SIZE];
Deleter TLS::deleters_ [USER_TLS_INDEXES_END];
uint16_t TLS::free_count_;

void TLS_Context::Entry::destruct () noexcept
{
	if (deleter_ && ptr_) {
		try {
			(*deleter_) (ptr_);
		} catch (...) {
			// TODO: Log
		}
	}
}

}
}
