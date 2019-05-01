#ifndef NIRVANA_CORE_STRINGMANAGER_H_
#define NIRVANA_CORE_STRINGMANAGER_H_

#include "../core.h"
#include "ReferenceCounterImpl.h"
#include <CORBA/Implementation.h>

namespace Nirvana {
namespace Core {

using namespace CORBA;

class StringManagerBase
{
public:
	static void initialize ()
	{
		heap_ = g_heap_factory->create_with_granularity (sizeof (String) * 2);
	}

	static void terminate ()
	{
		CORBA::release (heap_);
	}

protected:
	class String :
		public CORBA::Nirvana::ServantTraits <String>,
		public CORBA::Nirvana::LifeCycleRefCnt <String>,
		public CORBA::Nirvana::InterfaceImpl <String, CORBA::Nirvana::DynamicServant>,
		public ReferenceCounterBase
	{
	public:
		String (ULong size);

		static String* create (ULong size);

		void _delete ()
		{
			UWord cb = sizeof (String) + allocated_size_;
			this->~String ();
			heap ()->release (this, cb);
		}

		static String* get_object (const void* p)
		{
			if (heap ()->is_writable (p, 1))
				return (String*)p - 1;
			else
				return nullptr;
		}

		void* data ()
		{
			return this + 1;
		}

		ULong size () const
		{
			return size_;
		}

		ULong allocated_size () const
		{
			return allocated_size_;
		}

		String* get_writable_copy (ULong size);

	private:
		ULong size_;
		ULong allocated_size_;
	};

protected:
	static Memory_ptr heap ()
	{
		return heap_;
	}

private:
	static CORBA::Nirvana::Bridge <Memory>* heap_;
};

}
}

#endif

