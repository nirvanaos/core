#ifndef NIRVANA_CORE_MODULE_H_
#define NIRVANA_CORE_MODULE_H_

#include <Nirvana/Module_s.h>
#include "AtomicCounter.h"
#include <CORBA/LifeCycleRefCnt.h>
#include "Binder.h"
#include <vector>

namespace Nirvana {
namespace Core {

class Module :
	public CORBA::Nirvana::Servant <Module, ::Nirvana::Module>,
	public CORBA::Nirvana::LifeCycleRefCnt <Module>
{
public:
	void _add_ref ()
	{
		ref_cnt_.increment ();
	}

	void _remove_ref ()
	{
		if (!ref_cnt_.decrement ())
			on_release ();
	}

	std::vector <Binder::Iterator> exported_interfaces_;

private:
	void on_release ()
	{}

private:
	RefCounter ref_cnt_;
};

}
}

#endif
