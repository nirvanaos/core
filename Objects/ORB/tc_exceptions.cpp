#include <CORBA/CORBA.h>
#include <CORBA/Proxy/TypeCodeEnum.h>
#include <CORBA/Proxy/TypeCodeException.h>
#include "tc_impex.h"

namespace CORBA {
namespace Nirvana {

template <>
const Char TypeCodeEnum <CompletionStatus>::name_ [] = "CompletionStatus";

template <>
const Char* const TypeCodeEnum <CompletionStatus>::members_ [] = {
	"COMPLETED_YES",
	"COMPLETED_NO",
	"COMPLETED_MAYBE"
};

class SystemExceptionMembers
{
protected:
	static const Parameter members_ [];
};

const Parameter SystemExceptionMembers::members_ [] = {
{ "minor", _tc_ulong },
{ "completed", _tc_CompletionStatus }
};

template <class E>
class TypeCodeException :
	public TypeCodeWithMembersImpl <2, TypeCodeExceptionRoot <E>, SystemExceptionMembers>
{};

}

typedef Nirvana::TypeCodeEnum <CompletionStatus> TC_CompletionStatus;

#define TC_EXCEPTION(E) typedef Nirvana::TypeCodeException <E> TC_##E;

SYSTEM_EXCEPTIONS(TC_EXCEPTION)
TC_EXCEPTION(UnknownUserException)

}

TC_IMPEX_BY_ID (CompletionStatus)

SYSTEM_EXCEPTIONS(TC_IMPEX_BY_ID)
TC_IMPEX_BY_ID(UnknownUserException)
