#ifndef NIRVANA_ORB_CORE_REFERENCECOUNTER_H_
#define NIRVANA_ORB_CORE_REFERENCECOUNTER_H_

#include <CORBA/ReferenceCounter_s.h>
#include <CORBA/DynamicServant_s.h>
#include <CORBA/ImplementationPseudo.h>
#include "../AtomicCounter.h"
#include <limits>

namespace CORBA {
namespace Nirvana {
namespace Core {

class ReferenceCounter :
	public LifeCycleNoCopy <ReferenceCounter>,
	public ImplementationPseudo <ReferenceCounter, CORBA::Nirvana::ReferenceCounter>
{
public:
	ReferenceCounter (DynamicServant_ptr dynamic) :
		dynamic_ (dynamic)
	{}

	void _add_ref ()
	{
		ref_cnt_.increment ();
	}

	void _remove_ref ()
	{
		if (!ref_cnt_.decrement () && dynamic_) {
			try {
				dynamic_->_delete ();
			} catch (...) {
				assert (false); // TODO: Swallow exception or log
			}
		}
	}

	ULong _refcount_value () const
	{
		::Nirvana::Core::RefCounter::UIntType ucnt = ref_cnt_;
		return ucnt > std::numeric_limits <ULong>::max () ? std::numeric_limits <ULong>::max () : (ULong)ucnt;
	}

private:
	::Nirvana::Core::RefCounter ref_cnt_;
	DynamicServant_ptr dynamic_;
};

}
}
}

#endif
