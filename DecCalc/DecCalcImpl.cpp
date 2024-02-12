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
#include <Nirvana/Nirvana.h>
#include <Nirvana/DecCalc_s.h>

// All calculations are performed with double precision 62 digits
#define DECNUMDIGITS 62
extern "C" {
#include <decNumber/decPacked.h>
}

using namespace Nirvana;

static_assert (sizeof (decNumber) == sizeof (DecCalc::Number), "Check decNumber library definitions.");

struct Context : decContext
{
	Context (int32_t digits = DECNUMDIGITS)
	{
		// Check for correct DECLITEND macro setting in the decNumber build.
		assert (0 == decContextTestEndian (1));
		decContextDefault (this, DEC_INIT_BASE);
		traps = 0;
		this->digits = digits;
	}

	void check () const;
};

void Context::check () const
{
	if (status) {
		if ((DEC_Division_by_zero | DEC_Division_undefined) & status)
			throw_ARITHMETIC_ERROR (FPE_FLTDIV);
		else if (DEC_Overflow & status)
			throw_ARITHMETIC_ERROR (FPE_FLTOVF);
		else if (DEC_Underflow & status)
			throw_ARITHMETIC_ERROR (FPE_FLTUND);
		else if (DEC_Inexact & status)
			throw_ARITHMETIC_ERROR (FPE_FLTRES);
		else
			throw_BAD_PARAM ();
	}
}

namespace Nirvana {

class Static_g_dec_calc :
	public IDL::traits <DecCalc>::ServantStatic <Static_g_dec_calc>
{
public:
	static void from_long (Number& n, int32_t l)
	{
		decNumberFromInt32 ((decNumber*)&n, l);
	}

	static void from_ulong (Number& n, uint32_t u)
	{
		decNumberFromUInt32 ((decNumber*)&n, u);
	}

	static void from_longlong (Number& n, int64_t l)
	{
		decNumberFromInt64 ((decNumber*)&n, l);
	}

	static void from_ulonglong (Number& n, uint64_t u)
	{
		decNumberFromUInt64 ((decNumber*)&n, u);
	}

	static int64_t to_longlong (const Number& n)
	{
		Context ctx;
		int64_t ret;
		if (n.exponent ()) {
			decNumber integral;
			decNumberToIntegralValue (&integral, (const decNumber*)&n, &ctx);
			if (ctx.status)
				throw_DATA_CONVERSION ();
			ret = decNumberToInt64 (&integral, &ctx);
		} else
			ret = decNumberToInt64 ((const decNumber*)&n, &ctx);
		if (ctx.status)
			throw_DATA_CONVERSION ();
		return ret;
	}

	static void from_string (Number& n, const char* s)
	{
		Context ctx;
		decNumberFromString ((decNumber*)&n, s, &ctx);
		if (ctx.status)
			throw_DATA_CONVERSION ();
	}

	static std::string to_string (const Number& n)
	{
		char buf [DECNUMDIGITS + 14];
		assert (n.exponent () <= 0);
		decNumberToEngString ((const decNumber*)&n, buf);
		return buf;
	}

	static void from_BCD (Number& n, unsigned short digits, short scale, const uint8_t* bcd)
	{
		int32_t s = scale;
		if (!decPackedToNumber (bcd, (digits + 2) / 2, &s, (decNumber*)&n))
			throw_DATA_CONVERSION ();
		assert (s == scale);
		if (s < 0) {
			decNumber exp;
			decNumberZero (&exp);
			Context ctx (DECNUMDIGITS);
			decNumberRescale ((decNumber*)&n, (const decNumber*)&n, &exp, &ctx);
			ctx.check ();
			assert (n.exponent () <= 0);
		}
	}

	static void to_BCD (const Number& n, unsigned short digits, short scale, uint8_t* bcd)
	{
		int32_t s;
		if (n.digits () == digits && n.exponent () == -scale)
			decPackedFromNumber (bcd, (digits + 2) / 2, &s, (const decNumber*)&n);
		else {
			Context ctx (digits);
			decNumber exp;
			decNumberZero (&exp);
			exp.digits = digits;
			exp.exponent = -scale;
			decNumber tmp;
			memset (tmp.lsu, 0, sizeof (tmp.lsu));
			decNumberQuantize (&tmp, (const decNumber*)&n, &exp, &ctx);
			if (ctx.status)
				throw_DATA_CONVERSION ();
			assert (tmp.digits <= digits);
			tmp.digits = digits;
			decPackedFromNumber (bcd, (digits + 2) / 2, &s, &tmp);
		}
		assert (s == scale);
	}

	static void round (Number& n, short scale)
	{
		int trim = -n.exponent () - scale;
		if (trim > 0) {
			Context ctx (n.digits () - trim);
			decNumberReduce ((decNumber*)&n, (const decNumber*)&n, &ctx);
		}
	}

	static void truncate (Number& n, short scale)
	{
		int trim = -n.exponent () - scale;
		if (trim > 0) {
			decNumber rem = (const decNumber&)n;
			rem.digits = trim;
			Context ctx (n.digits () - trim);
			decNumberSubtract ((decNumber*)&n, (const decNumber*)&n, &rem, &ctx);
		}
	}

	static void add (Number& n, const Number& x)
	{
		Context ctx;
		decNumberAdd ((decNumber*)&n, (decNumber*)&n, (const decNumber*)&x, &ctx);
		ctx.check ();
	}

	static void subtract (Number& n, const Number& x)
	{
		Context ctx;
		decNumberSubtract ((decNumber*)&n, (decNumber*)&n, (const decNumber*)&x, &ctx);
		ctx.check ();
	}

	static void multiply (Number& n, const Number& x)
	{
		Context ctx;
		decNumberMultiply ((decNumber*)&n, (decNumber*)&n, (const decNumber*)&x, &ctx);
		ctx.check ();
	}

	static void divide (Number& n, const Number& x)
	{
		Context ctx;
		decNumberDivide ((decNumber*)&n, (decNumber*)&n, (const decNumber*)&x, &ctx);
		ctx.check ();
	}

	static void minus (Number& n)
	{
		Context ctx;
		decNumberMinus ((decNumber*)&n, (const decNumber*)&n, &ctx);
		ctx.check ();
	}

	static bool is_zero (const Number& n)
	{
		decNumber z;
		decNumberZero (&z);
		Context ctx;
		decNumber res;
		decNumberCompare (&res, (const decNumber*)&n, &z, &ctx);
		ctx.check ();
		return decNumberToInt32 (&res, &ctx) == 0;
	}

	static int16_t compare (const Number& lhs, const Number& rhs)
	{
		Context ctx;
		decNumber res;
		decNumberCompare (&res, (const decNumber*)&lhs, (const decNumber*)&rhs, &ctx);
		ctx.check ();
		return (int16_t)decNumberToInt32 (&res, &ctx);
	}
};

}

NIRVANA_EXPORT_PSEUDO (_exp_Nirvana_g_dec_calc, Nirvana::Static_g_dec_calc)
