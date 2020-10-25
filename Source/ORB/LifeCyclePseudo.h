#ifndef NIRVANA_ORB_CORE_LIFECYCLEPSEUDO_H_
#define NIRVANA_ORB_CORE_LIFECYCLEPSEUDO_H_

#include "../AtomicCounter.h"

namespace CORBA {
namespace Nirvana {
namespace Core {

/// \brief Dynamic servant life cycle implementation for pseudo objects.
/// \tparam S Servant class.
template <class S>
class LifeCyclePseudo :
	public LifeCycleRefCnt <S>
{
public:
	void _add_ref ()
	{
		ref_cnt_.increment ();
	}

	void _remove_ref ()
	{
		if (!ref_cnt_.decrement ())
			delete static_cast <S*> (this);
	}

private:
	::Nirvana::Core::RefCounter ref_cnt_;
};

}
}
}

#endif
