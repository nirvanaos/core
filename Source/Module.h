#ifndef NIRVANA_CORE_MODULE_H_
#define NIRVANA_CORE_MODULE_H_

#include <Nirvana/Module_s.h>
#include "AtomicCounter.h"
#include <CORBA/LifeCycleRefCnt.h>

namespace Nirvana {
namespace Core {

class Module :
	public CORBA::Nirvana::Servant <Module, ::Nirvana::Module>,
	public CORBA::Nirvana::LifeCycleRefCnt <Module>
{
public:
	Module (const void* base_address) :
		base_address_ (base_address),
		module_entry_ (nullptr)
	{}

	const void* base_address () const
	{
		return base_address_;
	}

	void _add_ref ()
	{
		ref_cnt_.increment ();
	}

	void _remove_ref ()
	{
		if (!ref_cnt_.decrement ())
			on_release ();
	}

private:
	void on_release ()
	{}

public:
	const ImportInterface* module_entry_;
	const void* base_address_;

private:
	RefCounter ref_cnt_;
};

}
}

#endif
