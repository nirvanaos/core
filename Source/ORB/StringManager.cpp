#include "../core.h"
#include <CORBA/StringManager_s.h>
#include <CORBA/Implementation.h>
#include "ReferenceCounterImpl.h"
#include <limits>

namespace Nirvana {
namespace Core {

using namespace CORBA;

class StringManagerBase
{
public:
	static void initialize ()
	{
		heap_ = g_heap_factory->create_with_granularity (sizeof (String) * 2);
		allocation_unit_ = heap ()->query (nullptr, Memory::ALLOCATION_UNIT);
	}

	static void terminate ()
	{
		release (heap_);
	}

protected:
	class String :
		public ReferenceCounterBase,
		public CORBA::Nirvana::ServantTraits <String>,
		public CORBA::Nirvana::LifeCycleRefCnt <String>,
		public CORBA::Nirvana::InterfaceImpl <String, CORBA::Nirvana::DynamicServant>
	{
	public:
		String (ULong size) :
			ReferenceCounterBase (*this),
			size_ (size),
			allocated_size_ ((ULong)(round_up (sizeof (String) + size_, allocation_unit_) - sizeof (String)))
		{}

		static String* create (ULong size)
		{
			String* p = (String*)heap ()->allocate (0, sizeof (String) + size, 0);
			new (p) String (size);
			return p;
		}

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

		String* get_writable_copy (ULong size)
		{
			if ((_refcount_value () == 1) && (allocated_size_ >= size)) {
				size_ = size;
				return this;
			}
			String* ns = (String*)heap ()->copy (nullptr, this, sizeof (String) + size, Memory::ALLOCATE | Memory::READ_WRITE);
			new (ns) String (size);
			release (this);
			return ns + 1;
		}

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
	static size_t allocation_unit_;
};

template <class C>
class StringManager :
	public CORBA::Nirvana::ServantStatic <StringManager <C>, CORBA::Nirvana::StringManager <C> >,
	public StringManagerBase
{
public:
	static const ULong MAX_LENGTH = std::numeric_limits <ULong>::max () / sizeof (C);

	C* string_alloc (ULong length)
	{
		if (length > MAX_LENGTH)
			throw IMP_LIMIT ();
		ULong size = (length + 1) * sizeof (C);
		String* s = String::create (size);
		C* ret = (C*)(s + 1);
		ret [length] = 0;
		return ret;
	}

	C* string_dup (const C* p)
	{
		if (p) {
			String* s = String::get_object (p);
			if (s)
				s->_add_ref ();
		}
		return const_cast <C*> (p);
	}

	void string_free (C* p)
	{
		if (p) {
			String* s = String::get_object (p);
			if (s)
				s->_remove_ref ();
		}
	}

	C& at (C*& p, ULong index)
	{
		C* pc = p;
		String* s = String::get_object (pc);
		if (s) {
			ULong length = s->size () / sizeof (C) - 1;
			if (index >= length)
				throw BAD_PARAM ();
			s = s->get_writable_copy (s->size ());
		} else {
			// Read-only string
			ULong length = 0;
			for (const C* end = pc; *end; ++end) {
				if (length == MAX_LENGTH)
					throw BAD_PARAM ();
				++length;
			}
			ULong size = (length + 1) * sizeof (C);
			String* s = String::create (size);
			heap ()->copy (s->data (), pc, size, 0);
		}
		pc = (C*)s->data ();
		p = pc;
		return pc [index];
	}
};

}
}

namespace CORBA {
namespace Nirvana {

Bridge <StringManager <Char> >* const StringManager <Char>::singleton_ = STATIC_BRIDGE (::Nirvana::Core::StringManager <Char>, StringManager <Char>);
Bridge <StringManager <WChar> >* const StringManager <WChar>::singleton_ = STATIC_BRIDGE (::Nirvana::Core::StringManager <WChar>, StringManager <WChar>);

}
}