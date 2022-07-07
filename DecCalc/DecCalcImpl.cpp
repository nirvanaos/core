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
	Context ()
	{
		decContextDefault (this, DEC_INIT_BASE);
		this->digits = DECNUMDIGITS;
	}

	void check () const;
};

void Context::check () const
{
	if (status)
		throw runtime_error (decContextStatusToString (this));
}

class DecCalcImpl :
	public servant_traits <DecCalc>::ServantStatic <DecCalcImpl>
{
public:
	void from_string (Number& n, const string& s)
	{
		Context ctx;
		decNumberFromString ((decNumber*)&n, s.c_str (), &ctx);
	}

	string to_string (const Number& n);
	void from_BCD (Number& n, short digits, short scale, const void* bcd);
	void to_BCD (const Number& n, short digits, short scale, void* bcd);
};
