#ifndef NIRVANA_TESTORB_REFERENCECOUNTERIMPL_H_
#define NIRVANA_TESTORB_REFERENCECOUNTERIMPL_H_

#include <CORBA/ReferenceCounter_s.h>
#include <CORBA/DynamicServant_s.h>
#include <CORBA/Implementation.h>
#include "../AtomicCounter.h"

namespace Nirvana {
namespace Core {

using namespace ::CORBA;
using namespace ::CORBA::Nirvana;

class ReferenceCounterBase
{
public:
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
				assert (false); // Swallow exception or log
			}
		}
	}

	ULong _refcount_value () const
	{
		return ref_cnt_;
	}

protected:
	ReferenceCounterBase (DynamicServant_ptr dynamic) :
		ref_cnt_ (1),
		dynamic_ (dynamic)
	{}

private:
	AtomicCounter ref_cnt_;
	DynamicServant_ptr dynamic_;
};

template <class S>
class ReferenceCounterImpl :
	public ReferenceCounterBase,
	public InterfaceImpl <S, ReferenceCounter>
{
protected:
	ReferenceCounterImpl (DynamicServant_ptr dynamic) :
		ReferenceCounterBase (dynamic)
	{}
};

class ReferenceCounterCore :
	public ReferenceCounterImpl <ReferenceCounterCore>,
	public ServantTraits <ReferenceCounterCore>,
	public LifeCycleNoCopy <ReferenceCounterCore>
{
public:
	ReferenceCounterCore (DynamicServant_ptr dynamic) :
		ReferenceCounterImpl (dynamic)
	{}
};

}
}

#endif
