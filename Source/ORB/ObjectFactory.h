#ifndef NIRVANA_ORB_CORE_OBJECTFACTORY_H_
#define NIRVANA_ORB_CORE_OBJECTFACTORY_H_

#include <CORBA/CORBA.h>
#include <CORBA/ObjectFactory_s.h>
#include "ServantBase.h"
#include "LocalObject.h"
#include "ReferenceCounter.h"
#include "SyncDomain.h"

namespace CORBA {
namespace Nirvana {
namespace Core {

class ObjectFactory :
	public ServantStatic <ObjectFactory, CORBA::Nirvana::ObjectFactory>
{
public:
	static void* memory_allocate (size_t size)
	{
		::Nirvana::Core::SyncDomain::enter ();
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
		// TODO: Allocate from read-only heap
		void* p = ::Nirvana::Core::g_core_heap->allocate (0, scs.size, ::Nirvana::Memory::READ_ONLY | ::Nirvana::Memory::RESERVED);
		scs.offset = (uint8_t*)p - (uint8_t*)scs.tmp;
		stateless_creation_frame () = &scs;
	}

	void* stateless_end (bool success)
	{
		StatelessCreationFrame*& scsr = stateless_creation_frame ();
		StatelessCreationFrame* scs = scsr;
		scsr = nullptr;
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
	static StatelessCreationFrame*& stateless_creation_frame ();

	template <class I>
	static I_ptr <I> offset_ptr (I_ptr <I> p)
	{
		return reinterpret_cast <I*> ((Octet*)&p + offset_ptr ());
	}

	static size_t offset_ptr ();
};

}
}
}

#endif
