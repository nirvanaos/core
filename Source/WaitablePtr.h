#ifndef NIRVANA_CORE_WAITABLEPTR_H_
#define NIRVANA_CORE_WAITABLEPTR_H_

#include "TaggedPtr.h"
#include <CORBA/Exception.h>

namespace Nirvana {
namespace Core {

class WaitablePtr :
	public AtomicPtr <2, 4>
{
public:
	enum
	{
		TAG_OBJECT = 0,
		TAG_WAITLIST = 1,
		TAG_EXCEPTION = 2
	};

	WaitablePtr () :
		AtomicPtr <2, 4> (Ptr (nullptr, TAG_WAITLIST))
	{}

	~WaitablePtr ()
	{
		Ptr p = load ();
		if (p.tag_bits () == TAG_EXCEPTION)
			delete_exception ((CORBA::Exception*)(void*)p);
	}

	void* wait ();
	
	void set_object (void* p)
	{
		set_ptr (p);
	}

	void set_exception (const CORBA::Exception& ex);
	void set_unknown_exception ();

private:

	class WaitForCreation;

	void run_in_neutral ();

	void set_ptr (Ptr p);

	void delete_exception (CORBA::Exception* pex);
};

}
}

#endif
