#include "pch.h"
#include <CORBA/SFloat_s.h>

namespace CORBA {
namespace Internal {

class Static_g_sfloat16 : public IDL::traits <SFloat16>::ServantStatic <Static_g_sfloat16>
{
public:
	void from_double (const NativeDouble& x, FloatIEEE16& f);
	void from_long_double (const NativeLongDouble& x, FloatIEEE16& f);
	void from_long (long x, FloatIEEE16& f);
	void from_unsigned_long (unsigned long x, FloatIEEE16& f);
	void from_long_long (long long x, FloatIEEE16& f);
	void from_unsigned_long_long (unsigned long long x, FloatIEEE16& f);

	void to_double (const FloatIEEE16& f, NativeDouble& x);
	void to_long_double (const FloatIEEE16& f, NativeLongDouble& x);
	void to_long (const FloatIEEE16& f, long& x);
	void to_long_long (const FloatIEEE16& f, long long& x);

	void sum (FloatIEEE16& x, const FloatIEEE16& y);
	void sub (FloatIEEE16& x, const FloatIEEE16& y);
	void mul (FloatIEEE16& x, const FloatIEEE16& y);
	void div (FloatIEEE16& x, const FloatIEEE16& y);

	int compare (const FloatIEEE16& x, const FloatIEEE16& y);
};

}
}
