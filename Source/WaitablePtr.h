#ifndef NIRVANA_CORE_WAITABLEPTR_H_
#define NIRVANA_CORE_WAITABLEPTR_H_

#include <atomic>
#include <CORBA/Exception.h>

namespace Nirvana {
namespace Core {

class WaitablePtr
{
public:
	WaitablePtr () :
		ptr_ (PTR_WAITLIST)
	{}

	void* wait ();
	
	void set_object (void* p)
	{
		set_ptr ((uintptr_t)p);
	}

	void set_exception (const CORBA::Exception& ex);
	void set_unknown_exception ();

	bool is_object () const
	{
		return !(ptr_ & STATE_MASK);
	}

	operator void* () const
	{
		return (void*)(ptr_ & ~STATE_MASK);
	}

private:
	static const uintptr_t STATE_MASK = 3;
	enum
	{
		PTR_OBJECT = 0,
		PTR_WAITLIST = 1,
		PTR_EXCEPTION = 2
	};

	class WaitForCreation;

	void run_in_neutral ();

	void set_ptr (uintptr_t p);

	std::atomic <uintptr_t> ptr_;
};

}
}

#endif
