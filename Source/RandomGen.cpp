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
#include <atomic>

namespace Nirvana {
namespace Core {

using namespace std;

inline
uint16_t RandomGen::xorshift (uint16_t x)
{
	x ^= x << 7;
	x ^= x >> 9;
	x ^= x << 8;
	return x;
}

inline
uint32_t RandomGen::xorshift (uint32_t x)
{
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return x;
}

inline
uint64_t RandomGen::xorshift (uint64_t x)
{
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	return x;
}

RandomGen::result_type RandomGen::operator () ()
{
	return state_ = xorshift (state_);
}

RandomGen::result_type RandomGenAtomic::operator () ()
{
	volatile atomic <RandomGen::result_type>* ps = (volatile atomic<RandomGen::result_type>*) & state_;
	assert (atomic_is_lock_free (ps));
	RandomGen::result_type s = atomic_load(ps);
	RandomGen::result_type x;
	do {
		x = xorshift (s);
	} while (!atomic_compare_exchange_weak (ps, &s, x));
	return x;
}

}
}
