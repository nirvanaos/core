// Random number generator
// Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs"

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
#include "RandomGen.h"
#include <assert.h>
#include <atomic>

namespace Nirvana {
namespace Core {

RandomGen::result_type RandomGenAtomic::operator () () noexcept
{
	volatile std::atomic <RandomGen::result_type>* ps = (volatile std::atomic<RandomGen::result_type>*) & state_;
	assert (std::atomic_is_lock_free (ps));
	RandomGen::result_type s = std::atomic_load(ps);
	RandomGen::result_type x;
	do {
		x = xorshift (s);
	} while (!std::atomic_compare_exchange_weak (ps, &s, x));
	return x;
}

}
}
