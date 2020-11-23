#include "Startup.h"
#include "Scheduler.h"

namespace Nirvana {
namespace Core {

void Startup::on_exception () NIRVANA_NOEXCEPT
{
	exception_ = std::current_exception ();
	Scheduler::shutdown ();
}

void Startup::on_crash (Word error_code) NIRVANA_NOEXCEPT
{
	exception_code_ = (CORBA::SystemException::Code)error_code;
	Scheduler::shutdown ();
}

void Startup::check () const
{
	if (CORBA::Exception::EC_NO_EXCEPTION != exception_code_)
		CORBA::SystemException::_raise_by_code (exception_code_);
	else if (exception_)
		std::rethrow_exception (exception_);
}

}
}
