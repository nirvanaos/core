#ifndef NIRVANA_CORE_STARTUP_H_
#define NIRVANA_CORE_STARTUP_H_

#include "Runnable.h"
#include "initterm.h"
#include <exception>

namespace Nirvana {
namespace Core {

class NIRVANA_NOVTABLE Startup : public Runnable
{
public:
	Startup (int argc, char* argv []) :
		argc_ (argc),
		argv_ (argv),
		exception_code_ (CORBA::Exception::EC_NO_EXCEPTION)
	{}

	~Startup ()
	{}

	virtual void run ()
	{
		initialize ();
	}

	virtual void on_exception () NIRVANA_NOEXCEPT;
	virtual void on_crash (Word error_code) NIRVANA_NOEXCEPT;

	void check () const;

	int ret () const NIRVANA_NOEXCEPT
	{
		return ret_;
	}

protected:
	int argc_;
	char** argv_;
	int ret_;

private:
	std::exception_ptr exception_;
	CORBA::SystemException::Code exception_code_;
};


}
}

#endif
