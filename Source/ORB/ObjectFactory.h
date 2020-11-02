#ifndef NIRVANA_ORB_CORE_OBJECTFACTORY_H_
#define NIRVANA_ORB_CORE_OBJECTFACTORY_H_

#include <CORBA/CORBA.h>
#include <CORBA/ObjectFactory_s.h>
#include "ServantBase.h"
#include "LocalObject.h"
#include "ReferenceCounter.h"

namespace CORBA {
namespace Nirvana {
namespace Core {

class ObjectFactory :
	public ServantStatic <ObjectFactory, CORBA::Nirvana::ObjectFactory>
{
public:
	static void* memory_allocate (size_t size)
	{
		enter_sync_domain ();
		return ::Nirvana::g_memory->allocate (0, size, 0);
	}

	static void memory_release (void* p, size_t size)
	{
		::Nirvana::g_memory->release (p, size);
	}

	void stateless_begin (StatelessCreationFrame& scs)
	{
		if (!(scs.tmp && scs.size))
			throw BAD_PARAM ();
		void* p = ::Nirvana::g_memory->allocate (0, scs.size, ::Nirvana::Memory::READ_ONLY | ::Nirvana::Memory::RESERVED);
		scs.offset = (Octet*)p - (Octet*)scs.tmp;
		stateless_ = &scs;
	}

	void* stateless_end (bool success)
	{
		StatelessCreationFrame* scs = stateless_;
		stateless_ = 0;
		if (!scs)
			throw BAD_INV_ORDER ();
		void* p = (Octet*)scs->tmp + scs->offset;
		if (success) {
			::Nirvana::g_memory->copy (p, const_cast <void*> (scs->tmp), scs->size, ::Nirvana::Memory::READ_ONLY);
			return p;
		} else {
			::Nirvana::g_memory->release (p, scs->size);
			return 0;
		}
	}

	const void* stateless_copy (const void* src, size_t size)
	{
		return ::Nirvana::g_memory->copy (0, const_cast <void*> (src), size, ::Nirvana::Memory::READ_ONLY);
	}

	static CORBA::Nirvana::ReferenceCounter_ptr create_reference_counter (CORBA::Nirvana::DynamicServant_ptr dynamic)
	{
		return (new ReferenceCounter (offset_ptr (dynamic)))->_get_ptr ();
	}

	static PortableServer::ServantBase_var create_servant (PortableServer::Servant servant)
	{
		return (new ServantBase (offset_ptr (servant)))->_get_ptr ();
	}

	static LocalObject_var create_local_object (LocalObject_ptr servant, AbstractBase_ptr abstract_base)
	{
		return (new LocalObject (offset_ptr (servant), offset_ptr (abstract_base)))->_get_ptr ();
	}

private:
	template <class I>
	static I_ptr <I> offset_ptr (I_ptr <I> p)
	{
		if (stateless_) {
			void* vp = &p;
			p = reinterpret_cast <I*> ((Octet*)vp + stateless_->offset);
		} else
			enter_sync_domain ();
		return p;
	}

	static void enter_sync_domain ()
	{
		// If we currently run out of SD, create new SD and enter into it.
	}

private:
	static StatelessCreationFrame* stateless_;
};

}
}
}

#endif
