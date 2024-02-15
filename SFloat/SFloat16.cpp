/*
* Nirvana SFloat module.
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
#include <Nirvana/Nirvana.h>
#include <CORBA/SFloat_s.h>
#include "Context.h"

extern "C" {
#include "softfloat.h"
}

namespace CORBA {
namespace Internal {

class Static_g_sfloat16 : public IDL::traits <SFloat16>::ServantStatic <Static_g_sfloat16>
{
public:
	static uint16_t from_double (const NativeDouble& x, FloatIEEE& f)
	{
		SFloatContext context;
		f64_to_f128M (reinterpret_cast <const float64_t&> (x), &reinterpret_cast <float128_t&> (f));
		return context.exception_flags ();
	}

	static uint16_t from_long_double (const NativeLongDouble& x, FloatIEEE& f)
	{
		SFloatContext context;
		extF80M_to_f128M (&reinterpret_cast <const extFloat80_t&> (x), &reinterpret_cast <float128_t&> (f));
		return context.exception_flags ();
	}

	static uint16_t from_long (int32_t x, FloatIEEE& f)
	{
		SFloatContext context;
		i32_to_f128M (x, &reinterpret_cast <float128_t&> (f));
		return context.exception_flags ();
	}

	static uint16_t from_unsigned_long (unsigned long x, FloatIEEE& f)
	{
		SFloatContext context;
		ui32_to_f128M (x, &reinterpret_cast <float128_t&> (f));
		return context.exception_flags ();
	}

	static uint16_t from_long_long (long long x, FloatIEEE& f)
	{
		SFloatContext context;
		i64_to_f128M (x, &reinterpret_cast <float128_t&> (f));
		return context.exception_flags ();
	}

	static uint16_t from_unsigned_long_long (unsigned long long x, FloatIEEE& f)
	{
		SFloatContext context;
		ui64_to_f128M (x, &reinterpret_cast <float128_t&> (f));
		return context.exception_flags ();
	}

	static uint16_t to_double (const FloatIEEE& f, NativeDouble& x)
	{
		SFloatContext context;
		reinterpret_cast <float64_t&> (x) = f128M_to_f64 (&reinterpret_cast <const float128_t&> (f));
		return context.exception_flags ();
	}

	static uint16_t to_long_double (const FloatIEEE& f, NativeLongDouble& x)
	{
		SFloatContext context;
		f128M_to_extF80M (&reinterpret_cast <const float128_t&> (f), &reinterpret_cast <extFloat80_t&> (x));
		return context.exception_flags ();
	}

	static uint16_t to_long (const FloatIEEE& f, int32_t& x)
	{
		SFloatContext context;
		x = f128M_to_i32_r_minMag (&reinterpret_cast <const float128_t&> (f), false);
		return context.exception_flags ();
	}

	static uint16_t to_long_long (const FloatIEEE& f, int64_t& x)
	{
		SFloatContext context;
		x = f128M_to_i64_r_minMag (&reinterpret_cast <const float128_t&> (f), false);
		return context.exception_flags ();
	}

	static uint16_t add (FloatIEEE& x, const FloatIEEE& y)
	{
		SFloatContext context;
		f128M_add (&reinterpret_cast <const float128_t&> (x), &reinterpret_cast <const float128_t&> (y),
			&reinterpret_cast <float128_t&> (x));
		return context.exception_flags ();
	}

	static uint16_t sub (FloatIEEE& x, const FloatIEEE& y)
	{
		SFloatContext context;
		f128M_sub (&reinterpret_cast <const float128_t&> (x), &reinterpret_cast <const float128_t&> (y),
			&reinterpret_cast <float128_t&> (x));
		return context.exception_flags ();
	}

	static uint16_t mul (FloatIEEE& x, const FloatIEEE& y)
	{
		SFloatContext context;
		f128M_mul (&reinterpret_cast <const float128_t&> (x), &reinterpret_cast <const float128_t&> (y),
			&reinterpret_cast <float128_t&> (x));
		return context.exception_flags ();
	}

	static uint16_t div (FloatIEEE& x, const FloatIEEE& y)
	{
		SFloatContext context;
		f128M_div (&reinterpret_cast <const float128_t&> (x), &reinterpret_cast <const float128_t&> (y),
			&reinterpret_cast <float128_t&> (x));
		return context.exception_flags ();
	}

	static uint16_t eq (const FloatIEEE& x, const FloatIEEE& y, bool& res)
	{
		SFloatContext context;
		res = f128M_eq (&reinterpret_cast <const float128_t&> (x), &reinterpret_cast <const float128_t&> (y));
		return context.exception_flags ();
	}

	static uint16_t le (const FloatIEEE& x, const FloatIEEE& y, bool& res)
	{
		SFloatContext context;
		res = f128M_le (&reinterpret_cast <const float128_t&> (x), & reinterpret_cast <const float128_t&> (y));
		return context.exception_flags ();
	}

	static uint16_t lt (const FloatIEEE& x, const FloatIEEE& y, bool& res)
	{
		SFloatContext context;
		res = f128M_lt (&reinterpret_cast <const float128_t&> (x), &reinterpret_cast <const float128_t&> (y));
		return context.exception_flags ();
	}
};

}
}

NIRVANA_EXPORT_PSEUDO (_exp_CORBA_Internal_g_sfloat16, CORBA::Internal::Static_g_sfloat16)

