#include <CORBA/CORBA.h>
#include <CORBA/Proxy/TypeCodeEnum.h>
#include <CORBA/Proxy/TypeCodeException.h>
#include "tc_impex.h"

namespace CORBA {
namespace Nirvana {

template <>
const Char TypeCodeEnum <TCKind>::name_ [] = "TCKind";

template <>
const Char* const TypeCodeEnum <TCKind>::members_ [] = {
	"tk_null", "tk_void",
	"tk_short", "tk_long", "tk_ushort", "tk_ulong",
	"tk_float", "tk_double", "tk_boolean", "tk_char",
	"tk_octet", "tk_any", "tk_TypeCode", "tk_Principal", "tk_objref",
	"tk_struct", "tk_union", "tk_enum", "tk_string",
	"tk_sequence", "tk_array", "tk_alias", "tk_except",
	"tk_longlong", "tk_ulonglong", "tk_longdouble",
	"tk_wchar", "tk_wstring", "tk_fixed",
	"tk_value", "tk_value_box",
	"tk_native",
	"tk_abstract_interface",
	"tk_local_interface"
};

template <>
class TypeCodeException <TypeCode::BadKind> :
	public TypeCodeExceptionImpl <TypeCode::BadKind>
{};

template <>
class TypeCodeException <TypeCode::Bounds> :
	public TypeCodeExceptionImpl <TypeCode::Bounds>
{};

}

typedef Nirvana::TypeCodeEnum <TCKind> TC_TCKind;
typedef Nirvana::TypeCodeException <TypeCode::BadKind> TC_TypeCode_BadKind;
typedef Nirvana::TypeCodeException <TypeCode::Bounds> TC_TypeCode_Bounds;

}

TC_IMPEX_BY_ID (TCKind)

INTERFACE_EXC_IMPEX (CORBA, TypeCode, BadKind)
INTERFACE_EXC_IMPEX (CORBA, TypeCode, Bounds)

