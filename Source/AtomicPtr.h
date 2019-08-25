#ifndef NIRVANA_CORE_ATOMICPTR_H_
#define NIRVANA_CORE_ATOMICPTR_H_

#include "core.h"
#include "BackOff.h"
#include <atomic>
#include <algorithm>
#include <type_traits>

namespace Nirvana {
namespace Core {

template <unsigned ALIGN> class AtomicPtr;

template <unsigned ALIGN = HEAP_UNIT_CORE>
class TaggedPtr
{
	static const uintptr_t TAG_BITS = ALIGN - 1;
public:
	static const unsigned alignment = ALIGN;
	typedef AtomicPtr <ALIGN> Atomic;

	TaggedPtr ()
	{}

	TaggedPtr (void* p) :
		ptr_ ((uintptr_t)p)
	{
		assert (!((uintptr_t)p & TAG_BITS));
	}

	TaggedPtr (const TaggedPtr <ALIGN>& src) :
		ptr_ (src.ptr_)
	{}

	void* operator = (void* p)
	{
		assert (!((uintptr_t)p & TAG_BITS));
		ptr_ = (uintptr_t)p;
		return p;
	}

	TaggedPtr <ALIGN>& operator = (const TaggedPtr <ALIGN>& src)
	{
		ptr_ = src.ptr_;
		return *this;
	}

	uintptr_t is_marked () const
	{
		return ptr_ & 1;
	}

	TaggedPtr <ALIGN> marked () const
	{
		return TaggedPtr <ALIGN> (ptr_ | 1);
	}

	TaggedPtr <ALIGN> unmarked () const
	{
		return TaggedPtr <ALIGN> (ptr_ & ~(uintptr_t)1);
	}

	operator void* () const
	{
		return (void*)(ptr_ & ~(uintptr_t)1);
	}

	bool operator == (const TaggedPtr <ALIGN>& rhs) const
	{
		return ptr_ == rhs.ptr_;
	}

private:
	TaggedPtr (uintptr_t val) :
		ptr_ (val)
	{}

private:
	friend class MarkablePtr;
	friend class AtomicPtr <ALIGN>;
	friend class TaggedNil;

	uintptr_t ptr_;
};

class MarkablePtr
{
public:
	typedef TaggedPtr <2> Tagged;

	MarkablePtr ()
	{}

	MarkablePtr (Tagged src) :
		ptr_ (src.ptr_)
	{}

	Tagged load () const
	{
		return Tagged (ptr_.load ());
	}

	Tagged operator = (Tagged src)
	{
		ptr_ = src.ptr_;
	}

	bool compare_exchange (Tagged& from, const Tagged& to)
	{
		return ptr_.compare_exchange_weak (from.ptr_, to.ptr_);
	}

private:
	volatile std::atomic <uintptr_t> ptr_;
};

template <unsigned ALIGN = HEAP_UNIT_CORE>
class AtomicPtr
{
	static const uintptr_t SPIN_BITS = TaggedPtr <ALIGN>::TAG_BITS & ~(uintptr_t)1;
public:
	static const unsigned alignment = ALIGN;
	typedef TaggedPtr <ALIGN> Tagged;

	AtomicPtr ()
	{}

	AtomicPtr (Tagged src) :
		ptr_ (src.ptr_)
	{}

	Tagged load () const
	{
		return Tagged (ptr_.load () & ~SPIN_BITS);
	}

	Tagged operator = (Tagged src)
	{
		assert ((src.ptr_ & SPIN_BITS) == 0);
		for (BackOff bo; true; bo.sleep ()) {
			uintptr_t cur = ptr_.load ();
			while (!(cur & SPIN_BITS)) {
				if (ptr_.compare_exchange_weak (cur, src.ptr_))
					return src;
			}
		}
	}

	bool cas (const Tagged& from, const Tagged& to)
	{
		Tagged tmp = from;
		return compare_exchange (tmp, to);
	}

	bool compare_exchange (Tagged& cur, const Tagged& to)
	{
		uintptr_t tcur = cur.ptr_;
		assert ((tcur & SPIN_BITS) == 0);
		for (BackOff bo; !ptr_.compare_exchange_weak (tcur, to.ptr_); bo.sleep ()) {
			tcur &= ~SPIN_BITS;
			if (tcur != cur.ptr_) {
				cur.ptr_ = tcur;
				return false;
			}
			tcur = cur.ptr_;
		}
		return true;
	}

	Tagged lock ()
	{
		for (BackOff bo; true; bo.sleep ()) {
			uintptr_t cur = ptr_.load ();
			while ((cur & SPIN_BITS) != SPIN_BITS) {
				if (ptr_.compare_exchange_weak (cur, cur + 2, std::memory_order_acquire))
					return Tagged (cur & ~SPIN_BITS);
			}
		}
	}

	void unlock ()
	{
		ptr_.fetch_sub (2, std::memory_order_release);
	}

private:
	volatile std::atomic <uintptr_t> ptr_;
};

template <class T, unsigned ALIGN> class AtomicPtrT;

template <class T, unsigned ALIGN = std::max ((unsigned)HEAP_UNIT_CORE, (unsigned)(1 << log2_ceil (sizeof (T))))>
class TaggedPtrT : public TaggedPtr <ALIGN>
{
public:
	typedef AtomicPtrT <T, ALIGN> Atomic;

	TaggedPtrT ()
	{}

	TaggedPtrT (T* p) :
		TaggedPtr <ALIGN> (p)
	{}

	TaggedPtrT (const TaggedPtrT <T, ALIGN>& src) :
		TaggedPtr <ALIGN> (src)
	{}

	explicit
		TaggedPtrT (const TaggedPtr <ALIGN>& src) :
		TaggedPtr <ALIGN> (src)
	{}

	T* operator = (T* p)
	{
		return (T*)TaggedPtr <ALIGN>::operator = (p);
	}

	TaggedPtrT <T, ALIGN>& operator = (const TaggedPtrT <T, ALIGN>& src)
	{
		TaggedPtr <ALIGN>::operator = (src);
		return *this;
	}

	TaggedPtrT <T, ALIGN> marked () const
	{
		return TaggedPtrT <T, ALIGN> (TaggedPtr <ALIGN>::marked ());
	}

	TaggedPtrT <T, ALIGN> unmarked () const
	{
		return TaggedPtrT <T, ALIGN> (TaggedPtr <ALIGN>::unmarked ());
	}

	operator T* () const
	{
		return (T*)TaggedPtr <ALIGN>::operator void* ();
	}

	T* operator -> () const
	{
		return operator T* ();
	}
};

template <class T>
class MarkablePtrT : public MarkablePtr
{
public:
	typedef TaggedPtrT <T, 2> Tagged;

	MarkablePtrT ()
	{}

	MarkablePtrT (Tagged src) :
		MarkablePtr (src)
	{}

	Tagged load () const
	{
		return Tagged (MarkablePtr::load ());
	}

	Tagged operator = (Tagged src)
	{
		return Tagged (MarkablePtr::operator = (src));
	}

	bool compare_exchange (Tagged& from, const Tagged& to)
	{
		return MarkablePtr::compare_exchange (from, to);
	}
};

template <class T, unsigned ALIGN = std::max ((unsigned)HEAP_UNIT_CORE, (unsigned)(1 << log2_ceil (sizeof (T))))>
class AtomicPtrT : public AtomicPtr <ALIGN>
{
public:
	typedef TaggedPtrT <T, ALIGN> Tagged;

	AtomicPtrT ()
	{}

	AtomicPtrT (Tagged src) :
		AtomicPtr <ALIGN> (src)
	{}

	Tagged load () const
	{
		return Tagged (AtomicPtr <ALIGN>::load ());
	}

	Tagged operator = (Tagged src)
	{
		return Tagged (AtomicPtr <ALIGN>::operator = (src));
	}

	bool cas (const Tagged& from, const Tagged& to)
	{
		return AtomicPtr <ALIGN>::cas (from, to);
	}

	Tagged lock ()
	{
		return Tagged (AtomicPtr <ALIGN>::lock ());
	}
};

class TaggedNil
{
public:
	template <unsigned ALIGN>
	operator TaggedPtr <ALIGN> () const
	{
		return TaggedPtr <ALIGN> (ptr_);
	}
	
	template <class T, unsigned ALIGN>
	operator TaggedPtrT <T, ALIGN> () const
	{
		return TaggedPtrT <T, ALIGN> (TaggedPtr <ALIGN> (ptr_));
	}

	static TaggedNil marked ()
	{
		return TaggedNil (1);
	}

	static TaggedNil unmarked ()
	{
		return TaggedNil (0);
	}

private:
	TaggedNil (uintptr_t marked) :
		ptr_ ((uintptr_t)nullptr | marked)
	{}

private:
	uintptr_t ptr_;
};

}
}

#endif
