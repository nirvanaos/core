#ifndef NIRVANA_CORE_TAGGEDPTR_H_
#define NIRVANA_CORE_TAGGEDPTR_H_

#include "core.h"
#include "BackOff.h"
#include <atomic>
#include <algorithm>
#include <Nirvana/bitutils.h>

#define CORE_OBJECT_ALIGN(T) (std::max ((unsigned)::Nirvana::Core::HEAP_UNIT_CORE, (unsigned)(1 << ::Nirvana::log2_ceil (sizeof (T)))))

namespace Nirvana {
namespace Core {

template <unsigned TAG_BITS, unsigned ALIGN> class AtomicPtr;
template <unsigned TAG_BITS, unsigned ALIGN> class LockablePtr;

template <unsigned TAG_BITS, unsigned ALIGN = HEAP_UNIT_CORE>
class TaggedPtr
{
	static_assert (ALIGN == 1 << log2_ceil (ALIGN), "Alignment must be power of 2.");
public:
	static const uintptr_t ALIGN_MASK = ALIGN - 1;
	static const uintptr_t TAG_MASK = ~(~0u << TAG_BITS);
	static_assert (ALIGN_MASK >= TAG_MASK, "ALIGN_MASK >= TAG_MASK");
	typedef AtomicPtr <TAG_BITS, ALIGN> Atomic;
	typedef LockablePtr <TAG_BITS, ALIGN> Lockable;

	TaggedPtr ()
	{}

	TaggedPtr (void* p)
	{
		assert (!((uintptr_t)p & ALIGN_MASK));
		ptr_ = (uintptr_t)p;
	}

	TaggedPtr (void* p, uintptr_t tag_bits)
	{
		assert (!((uintptr_t)p & ALIGN_MASK));
		assert (!(tag_bits & ~TAG_MASK));
		ptr_ = (uintptr_t)p | tag_bits;
	}

	template <unsigned A>
	TaggedPtr (const TaggedPtr <TAG_BITS, A>& src) :
		ptr_ (src.ptr_)
	{
		static_assert (A > ALIGN_MASK, "Alignment decreasing.");
	}

	template <unsigned A>
	TaggedPtr <TAG_BITS, ALIGN>& operator = (const TaggedPtr <TAG_BITS, A>& src)
	{
		static_assert (A > ALIGN_MASK, "Alignment decreasing.");
		ptr_ = src.ptr_;
		return *this;
	}

	void* operator = (void* p)
	{
		assert (!((uintptr_t)p & ALIGN_MASK));
		ptr_ = (uintptr_t)p;
		return p;
	}

	operator void* () const
	{
		return (void*)(ptr_ & ~TAG_MASK);
	}

	unsigned tag_bits () const
	{
		return (unsigned)ptr_ & TAG_MASK;
	}

	TaggedPtr <TAG_BITS, ALIGN> untagged () const
	{
		return TaggedPtr <TAG_BITS, ALIGN> (ptr_ & ~TAG_MASK);
	}

	template <unsigned A>
	bool operator == (const TaggedPtr <TAG_BITS, A>& rhs) const
	{
		return ptr_ == rhs.ptr_;
	}

	operator bool () const
	{
		return ptr_ != 0;
	}

private:
	explicit TaggedPtr (uintptr_t val) :
		ptr_ (val)
	{}

private:
	friend class AtomicPtr <TAG_BITS, ALIGN>;
	friend class LockablePtr <TAG_BITS, ALIGN>;

	uintptr_t ptr_;
};

template <unsigned TAG_BITS, unsigned ALIGN = HEAP_UNIT_CORE>
class AtomicPtr
{
public:
	typedef TaggedPtr <TAG_BITS, ALIGN> Ptr;

	AtomicPtr ()
	{}

	AtomicPtr (Ptr src) :
		ptr_ (src.ptr_)
	{}

	Ptr load () const
	{
		return Ptr (ptr_.load ());
	}

	Ptr operator = (Ptr src)
	{
		ptr_.store (src.ptr_);
		return src;
	}

	bool cas (Ptr from, const Ptr& to)
	{
		return compare_exchange (from, to);
	}

	bool compare_exchange (Ptr& cur, const Ptr& to)
	{
		return ptr_.compare_exchange_weak (cur.ptr_, to.ptr_);
	}

	Ptr exchange (Ptr& to)
	{
		return Ptr (ptr_.exchange (to.ptr_));
	}

private:
	volatile std::atomic <uintptr_t> ptr_;
};

template <unsigned TAG_BITS, unsigned ALIGN = HEAP_UNIT_CORE>
class LockablePtr
{
public:
	typedef TaggedPtr <TAG_BITS, ALIGN> Ptr;

	LockablePtr ()
	{}

	LockablePtr (Ptr src) :
		ptr_ (src.ptr_)
	{}

	LockablePtr (const LockablePtr&) = delete;

	Ptr load () const
	{
		return Ptr (ptr_.load () & ~SPIN_MASK);
	}

	LockablePtr& operator = (const LockablePtr&) = delete;

	Ptr operator = (Ptr src)
	{
		assert ((src.ptr_ & SPIN_MASK) == 0);
		assert ((ptr_.load () & SPIN_MASK) == 0);
		ptr_.store (src.ptr_);
		return src;
	}

	bool cas (Ptr from, const Ptr& to)
	{
		return compare_exchange (from, to);
	}

	bool compare_exchange (Ptr& cur, const Ptr& to);

	Ptr lock ();

	void unlock ()
	{
		ptr_.fetch_sub (Ptr::TAG_MASK + 1, std::memory_order_release);
	}

private:
	static_assert (Ptr::ALIGN_MASK > Ptr::TAG_MASK, "Ptr::ALIGN_MASK > Ptr::TAG_MASK");
	static const uintptr_t SPIN_MASK = Ptr::ALIGN_MASK & ~Ptr::TAG_MASK;

	volatile std::atomic <uintptr_t> ptr_;
};

template <unsigned TAG_BITS, unsigned ALIGN>
bool LockablePtr <TAG_BITS, ALIGN>::compare_exchange (Ptr& cur, const Ptr& to)
{
	uintptr_t tcur = cur.ptr_;
	assert ((tcur & SPIN_MASK) == 0);
	for (BackOff bo; !ptr_.compare_exchange_weak (tcur, to.ptr_); bo.sleep ()) {
		tcur &= ~SPIN_MASK;
		if (tcur != cur.ptr_) {
			cur.ptr_ = tcur;
			return false;
		}
		tcur = cur.ptr_;
	}
	return true;
}

template <unsigned TAG_BITS, unsigned ALIGN>
typename LockablePtr <TAG_BITS, ALIGN>::Ptr LockablePtr <TAG_BITS, ALIGN>::lock ()
{
	for (BackOff bo; true; bo.sleep ()) {
		uintptr_t cur = ptr_.load ();
		while ((cur & SPIN_MASK) != SPIN_MASK) {
			if (ptr_.compare_exchange_weak (cur, cur + Ptr::TAG_MASK + 1, std::memory_order_acquire))
				return Ptr (cur & ~SPIN_MASK);
		}
		;
	}
}

template <class T, unsigned TAG_BITS, unsigned ALIGN> class AtomicPtrT;
template <class T, unsigned TAG_BITS, unsigned ALIGN> class LockablePtrT;

template <class T, unsigned TAG_BITS, unsigned ALIGN = CORE_OBJECT_ALIGN (T)>
class TaggedPtrT : public TaggedPtr <TAG_BITS, ALIGN>
{
	typedef TaggedPtr <TAG_BITS, ALIGN> Base;
public:
	typedef AtomicPtrT <T, TAG_BITS, ALIGN> Atomic;
	typedef LockablePtrT <T, TAG_BITS, ALIGN> Lockable;

	TaggedPtrT ()
	{}

	TaggedPtrT (T* p) :
		Base (p)
	{}

	TaggedPtrT (T* p, unsigned tag_bits) :
		Base (p, tag_bits)
	{}

	template <unsigned A>
	TaggedPtrT (const TaggedPtrT <T, TAG_BITS, A>& src) :
		Base (src)
	{}

	T* operator = (T* p)
	{
		return (T*)Base::operator = (p);
	}

	template <unsigned A>
	TaggedPtrT <T, TAG_BITS, ALIGN>& operator = (const TaggedPtrT <T, TAG_BITS, A>& src)
	{
		Base::operator = (src);
		return *this;
	}

	operator T* () const
	{
		return (T*)Base::operator void* ();
	}

	T* operator -> () const
	{
		return operator T* ();
	}

	TaggedPtrT <T, TAG_BITS, ALIGN> untagged () const
	{
		return TaggedPtrT <T, TAG_BITS, ALIGN> (Base::untagged ());
	}

private:
	friend class AtomicPtrT <T, TAG_BITS, ALIGN>;
	friend class LockablePtrT <T, TAG_BITS, ALIGN>;

	explicit TaggedPtrT (const Base& src) :
		Base (src)
	{}
};

template <class T, unsigned TAG_BITS, unsigned ALIGN = CORE_OBJECT_ALIGN (T)>
class AtomicPtrT : public AtomicPtr <TAG_BITS, ALIGN>
{
	typedef AtomicPtr <TAG_BITS, ALIGN> Base;
public:
	typedef TaggedPtrT <T, TAG_BITS, ALIGN> Ptr;

	AtomicPtrT ()
	{}

	AtomicPtrT (Ptr src) :
		Base (src)
	{}

	Ptr load () const
	{
		return Ptr (Base::load ());
	}

	Ptr operator = (Ptr src)
	{
		return Ptr (Base::operator = (src));
	}

	bool cas (const Ptr& from, const Ptr& to)
	{
		return Base::cas (from, to);
	}

	bool compare_exchange (Ptr& cur, const Ptr& to)
	{
		return Base::compare_exchange (cur, to);
	}
};

template <class T, unsigned TAG_BITS, unsigned ALIGN = CORE_OBJECT_ALIGN (T)>
class LockablePtrT : public LockablePtr <TAG_BITS, ALIGN>
{
	typedef LockablePtr <TAG_BITS, ALIGN> Base;
public:
	typedef TaggedPtrT <T, TAG_BITS, ALIGN> Ptr;

	LockablePtrT ()
	{}

	LockablePtrT (Ptr src) :
		Base (src)
	{}

	Ptr load () const
	{
		return Ptr (Base::load ());
	}

	Ptr operator = (Ptr src)
	{
		return Ptr (Base::operator = (src));
	}

	bool cas (const Ptr& from, const Ptr& to)
	{
		return Base::cas (from, to);
	}

	bool compare_exchange (Ptr& cur, const Ptr& to)
	{
		return Base::compare_exchange (cur, to);
	}

	Ptr lock ()
	{
		return Ptr (Base::lock ());
	}
};

}
}

#endif
