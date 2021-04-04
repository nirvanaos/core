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
#include "BackOff.h"
#include <random>
#if defined (_MSVC_LANG)
#include <intrin.h>
#endif

namespace Nirvana {
namespace Core {

inline void BackOff::cpu_relax ()
{
#if defined (_MSVC_LANG)
	_mm_pause ();
#elif defined (__GNUG__) || defined (__clang__)
	asm ("pause");
#endif
}

void BackOff::operator () ()
{
	if ((iterations_ <<= 1) > ITERATIONS_MAX)
		iterations_ = ITERATIONS_MAX;

	// TODO: We can try other distributions: geometric, exponential...
	typedef std::uniform_int_distribution <UWord> Dist;

	UWord iters = Dist (1U, iterations_)(rndgen_);
	if (ITERATIONS_MAX <= ITERATIONS_YIELD || iters < ITERATIONS_YIELD) {
		volatile UWord cnt = iters;
		do
			cpu_relax ();
		while (--cnt);
	} else
		Port::BackOff::yield (iters);
}

}
}
