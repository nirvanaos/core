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

	void from_string (Number& n, const string& s)
	{
		Context ctx (31);
		decNumberFromString ((decNumber*)&n, s.c_str (), &ctx);
		if (ctx.status)
			throw_DATA_CONVERSION ();
	}

	string to_string (const Number& n)
	{
		char buf [DECNUMDIGITS + 14];
		decNumberToEngString ((const decNumber*)&n, buf);
		return buf;
	}

	void from_BCD (Number& n, short digits, int32_t scale, const void* bcd)
	{
		if (!decPackedToNumber ((const uint8_t*)bcd, (digits + 2) / 2, &scale, (decNumber*)&n))
			throw_DATA_CONVERSION ();
	}

	void to_BCD (const Number& n, short digits, short scale, void* bcd)
	{
		Context ctx (digits);
		decNumber rounded;
		decNumberReduce (&rounded, (const decNumber*)&n, &ctx);
		int32_t s;
		decPackedFromNumber ((uint8_t*)bcd, (digits + 2) / 2, &s, &rounded);
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
