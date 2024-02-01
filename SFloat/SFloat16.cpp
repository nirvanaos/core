#include "pch.h"
#include <CORBA/SFloat_s.h>
#include "softfloat.h"

namespace CORBA {
namespace Internal {

class Static_g_sfloat16 : public IDL::traits <SFloat16>::ServantStatic <Static_g_sfloat16>
{
public:
	static void from_double (const NativeDouble& x, FloatIEEE16& f);

	static void from_long_double (const NativeLongDouble& x, FloatIEEE16& f)
	{
		extF80M_to_f128M (&reinterpret_cast <const extFloat80_t&> (x), &reinterpret_cast <float128_t&> (f));
	}

	static void from_long (long x, FloatIEEE16& f);
	static void from_unsigned_long (unsigned long x, FloatIEEE16& f);
	static void from_long_long (long long x, FloatIEEE16& f);
	static void from_unsigned_long_long (unsigned long long x, FloatIEEE16& f);

	static void to_double (const FloatIEEE16& f, NativeDouble& x);
	static void to_long_double (const FloatIEEE16& f, NativeLongDouble& x);
	static int32_t to_long (const FloatIEEE16& f);
	static int64_t to_long_long (const FloatIEEE16& f);

	static void sum (FloatIEEE16& x, const FloatIEEE16& y);
	static void sub (FloatIEEE16& x, const FloatIEEE16& y);
	static void mul (FloatIEEE16& x, const FloatIEEE16& y);
	static void div (FloatIEEE16& x, const FloatIEEE16& y);

	static int compare (const FloatIEEE16& x, const FloatIEEE16& y);
};

}
}
