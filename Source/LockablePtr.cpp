/// \file
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2025 Igor Popov.
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
#include "LockablePtr.h"
#include "BackOff.h"

namespace Nirvana {
namespace Core {

void LockablePtrImpl::assign (uintptr_t src, IntegralType lock_mask_inv) noexcept
{
	assert ((src & ~lock_mask_inv) == 0);
	IntegralType p = ptr_.load (std::memory_order_acquire) & lock_mask_inv;
	while (!ptr_.compare_exchange_weak (p, src)) {
		p &= lock_mask_inv;
	}
}

bool LockablePtrImpl::compare_exchange (uintptr_t& cur, uintptr_t to, IntegralType lock_mask_inv) noexcept
{
	IntegralType tcur = cur;
	assert ((tcur & ~lock_mask_inv) == 0);
	assert ((to & ~lock_mask_inv) == 0);
	IntegralType newcur = tcur;
	while (!ptr_.compare_exchange_weak (newcur, to)) {
		newcur &= lock_mask_inv;
		if (newcur != tcur) {
			cur = (uintptr_t)newcur;
			return false;
		}
	}
	return true;
}

uintptr_t LockablePtrImpl::lock (IntegralType lock_mask, IntegralType inc) noexcept
{
	for (BackOff bo;;) {
		IntegralType cur = ptr_.load (std::memory_order_acquire);
		while ((cur & lock_mask) != lock_mask) {
			if (ptr_.compare_exchange_weak (cur, cur + inc))
				return (uintptr_t)(cur & ~lock_mask);
		}
		bo ();
	}
}

}
}
