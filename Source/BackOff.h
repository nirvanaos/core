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
#ifndef NIRVANA_CORE_BACKOFF_H_
#define NIRVANA_CORE_BACKOFF_H_

#include <Port/BackOff.h>
#include "RandomGen.h"

namespace Nirvana {
namespace Core {

class BackOff :
	private Port::BackOff
{
public:
	BackOff () :
		iterations_ (1)
	{}

	~BackOff ()
	{}

	void operator () ();

private:
	/// A number of iterations when we should call yield ().
	using Port::BackOff::ITERATIONS_MAX;

	/// Maximal number of iterations.
	using Port::BackOff::ITERATIONS_YIELD;

	/// If ITERATIONS_MAX == ITERATIONS_YIELD we never call yield ().
	static_assert (ITERATIONS_MAX >= ITERATIONS_YIELD, "ITERATIONS_MAX >= ITERATIONS_YIELD");

	inline void cpu_relax ();

private:
	unsigned iterations_;
	RandomGen rndgen_;
};

}
}

#endif
