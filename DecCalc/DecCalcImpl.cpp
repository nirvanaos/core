#include "pch.h"
#include "DecCalc_s.h"

using namespace Nirvana;
using namespace CORBA;
using namespace std;

#if DECNUMDIGITS != 62
#error decNumber DECNUMDIGITS must be 62.
#endif

static_assert (sizeof (decNumber) == sizeof (DecCalc::Number), "Check decNumber library definitions.");

struct Context : decContext
{
	Context (int32_t digits = DECNUMDIGITS)
	{
		decContextDefault (this, DEC_INIT_BASE);
		traps = 0;
		this->digits = digits;
	}
};

class DecCalcImpl :
	public servant_traits <DecCalc>::ServantStatic <DecCalcImpl>
{
public:
	void from_long (Number& n, int32_t l)
	{
		decNumberFromInt32 ((decNumber*)&n, l);
	}

	void from_ulong (Number& n, uint32_t u)
	{
		decNumberFromUInt32 ((decNumber*)&n, u);
	}

	void from_longlong (Number& n, int64_t l)
	{
		decNumberFromInt64 ((decNumber*)&n, l);
	}

	void from_ulonglong (Number& n, uint64_t u)
	{
		decNumberFromUInt64 ((decNumber*)&n, u);
	}

	int64_t to_longlong (const Number& n)
	{
		Context ctx;
		int64_t ret = decNumberToInt64 ((const decNumber*)&n, &ctx);
		if (ctx.status)
			throw_DATA_CONVERSION ();
		return ret;
	}

	void from_string (Number& n, const char* s)
	{
		Context ctx;
		decNumberFromString ((decNumber*)&n, s, &ctx);
		if (ctx.status)
			throw_DATA_CONVERSION ();
	}

	string to_string (const Number& n)
	{
		char buf [DECNUMDIGITS + 14];
		decNumberToEngString ((const decNumber*)&n, buf);
		return buf;
	}

	void from_BCD (Number& n, short digits, int32_t scale, const uint8_t* bcd)
	{
		if (!decPackedToNumber (bcd, (digits + 2) / 2, &scale, (decNumber*)&n))
			throw_DATA_CONVERSION ();
	}

	void to_BCD (const Number& n, short digits, short scale, uint8_t* bcd)
	{
		Context ctx (digits);
		decNumber rounded;
		decNumberReduce (&rounded, (const decNumber*)&n, &ctx);
		int32_t s;
		decPackedFromNumber (bcd, (digits + 2) / 2, &s, &rounded);
		assert (s == scale);
	}

	void round (Number& n, short scale)
	{
		int trim = -n.exponent () - scale;
		if (trim > 0) {
			Context ctx (n.digits () - trim);
			decNumberReduce ((decNumber*)&n, (const decNumber*)&n, &ctx);
		}
	}

	void truncate (Number& n, short scale)
	{
		int trim = -n.exponent () - scale;
		if (trim > 0) {
			decNumber rem = (const decNumber&)n;
			rem.digits = trim;
			Context ctx (n.digits () - trim);
			decNumberSubtract ((decNumber*)&n, (const decNumber*)&n, &rem, &ctx);
		}
	}

};

NIRVANA_EXPORT (_exp_Nirvana_g_dec_calc, "Nirvana/g_dec_calc", DecCalc, DecCalcImpl)
