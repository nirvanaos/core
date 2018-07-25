// Nirvana project.
// Windows implementation.
// Execution domain (fiber).

#ifndef NIRVANA_CORE_EXECDOMAIN_H_
#define NIRVANA_CORE_EXECDOMAIN_H_

#include "core.h"
#include "Windows/ExecDomainBase.h"

namespace Nirvana {
namespace Core {

#ifdef _WIN32
typedef Windows::ExecDomainBase ExecDomainBase;
#else
#error Unknown platform.
#endif

class ExecDomain :
	private ExecDomainBase
{
public:
	typedef ::CORBA::ULongLong Deadline;

private:
	Deadline deadline_, effective_deadline_;
};

}
}

#endif
