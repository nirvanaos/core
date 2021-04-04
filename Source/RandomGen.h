/// \file
/// Atomic pseudorandom number generator.
/// 
/// We use fast and non-cryptographically secure Xorshift algorithm.
/// http://www.jstatsoft.org/v08/i14/paper

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
#ifndef NIRVANA_CORE_RANDOMGEN_H_
#define NIRVANA_CORE_RANDOMGEN_H_

#include <Nirvana/Nirvana.h>
#include <limits>

namespace Nirvana {
namespace Core {

/// Random-number generator.
class RandomGen
{
public:
	typedef UWord result_type;

	RandomGen () : // Use `this` as seed value
		state_ ((result_type)reinterpret_cast <uintptr_t> (this))
	{}

	RandomGen (result_type seed) :
		state_ (seed)
	{}

	static result_type min ()
	{
		return 0;
	}

	static result_type max ()
	{
		return std::numeric_limits <result_type>::max ();
	}

	result_type operator () ();

protected:
	static uint16_t xorshift (uint16_t x);
	static uint32_t xorshift (uint32_t x);
	static uint64_t xorshift (uint64_t x);

protected:
	result_type state_;
};

/// Atomic random-number generator.
class RandomGenAtomic :
	public RandomGen
{
public:
	result_type operator () ();
};

}
}

#endif

