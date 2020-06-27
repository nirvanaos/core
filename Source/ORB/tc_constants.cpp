#include <CORBA/CORBA.h>
#include <CORBA/Proxy/TypeCodeString.h>
#include "tc_impex.h"

namespace CORBA {

namespace Nirvana {

template <typename T, TCKind tk>
class TypeCodeScalar : public TypeCodeStatic <TypeCodeScalar <T, tk>, TypeCodeTK <tk>, TypeCodeOps <T> > {};

}

using namespace Nirvana;

class TC_TypeCode :
	public TypeCodeStatic <TC_TypeCode, TypeCodeTK <tk_TypeCode>, TypeCodeOps <TypeCode_var> >
{};

class TC_Object :
	public TypeCodeStatic <TC_Object, TypeCodeWithId <tk_objref, Object>, TypeCodeOps <Object_var> >
{
public:
	static const char* _name (Bridge <TypeCode>* _b, EnvironmentBridge* _env)
	{
		return "Object";
	}
};

}

#define TC_IMPL_SCALAR(T, t) TC_IMPEX(t, CORBA::Nirvana::TypeCodeScalar <T, CORBA::tk_##t>)

TC_IMPL_SCALAR (void, void)
TC_IMPL_SCALAR (CORBA::Short, short)
TC_IMPL_SCALAR (CORBA::Long, long)
TC_IMPL_SCALAR (CORBA::UShort, ushort)
TC_IMPL_SCALAR (CORBA::ULong, ulong)
TC_IMPL_SCALAR (CORBA::Float, float)
TC_IMPL_SCALAR (CORBA::Double, double)
TC_IMPL_SCALAR (CORBA::Boolean, boolean)
TC_IMPL_SCALAR (CORBA::Char, char)
TC_IMPL_SCALAR (CORBA::Octet, octet)
TC_IMPL_SCALAR (CORBA::LongLong, longlong)
TC_IMPL_SCALAR (CORBA::ULongLong, ulonglong)
TC_IMPL_SCALAR (CORBA::LongDouble, longdouble)
TC_IMPL_SCALAR (CORBA::WChar, wchar)
TC_IMPL_SCALAR (CORBA::Any, any)
TC_IMPEX (TypeCode, CORBA::TC_TypeCode)
TC_IMPEX (string, CORBA::Nirvana::TypeCodeString <CORBA::Nirvana::String, 0>)
TC_IMPEX (wstring, CORBA::Nirvana::TypeCodeString <CORBA::Nirvana::WString, 0>)
TC_IMPEX_BY_ID (Object)

//TC_IMPEX (ValueBase)

