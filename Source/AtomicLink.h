#ifndef NIRVANA_CORE_ATOMICLINK_H_
#define NIRVANA_CORE_ATOMICLINK_H_

#include "BackOff.h"
#include "config.h"
#include <atomic>
#include <type_traits>

namespace Nirvana {
namespace Core {

template <class T> class AtomicLink;

namespace {

template <typename T>
struct SizeOf
{
	static const size_t value = sizeof (T);
};

template <>
struct SizeOf <void>
{
	static const size_t value = HEAP_UNIT_MIN;
};

/// In Nirvana all memory blocks aligned to the nearest power of 2 accordingly to size.
/// So, lowest log2_ceil (sizeof) bits of pointer are always 0. We use them for flags and spin.
template <typename T>
struct FlagBits
{
	static const uintptr_t mask = ~((~0) << log2_ceil (SizeOf <T>::value));
	static const uintptr_t spin = mask & ~(uintptr_t)1;
};

}

template <typename T>
class Link
{
public:
	Link ()
	{}

	Link (T* p) :
		ptr_ ((uintptr_t)p)
	{
		assert (!((uintptr_t)p & FlagBits <T>::mask));
	}

	Link (const Link <T>& src) :
		ptr_ (src.ptr_)
	{}

	T* operator = (T* p)
	{
		assert (!((uintptr_t)p & FlagBits <T>::mask));
		ptr_ = (uintptr_t)p;
		return p;
	}

	Link& operator = (const Link <T>& src)
	{
		ptr_ = src.ptr_;
		return *this;
	}

	uintptr_t is_marked () const
	{
		return ptr_ & 1;
	}

	Link <T> marked () const
	{
		Link <T> link;
		link.ptr_ = ptr_ | 1;
		return link;
	}

	Link <T> unmarked () const
	{
		Link <T> link;
		link.ptr_ = ptr_ & ~(uintptr_t)1;
		return link;
	}

	operator T* () const
	{
		return (T*)(ptr_ & ~(uintptr_t)1);
	}

	bool operator == (const Link <T>& rhs) const
	{
		return ptr_ == rhs.ptr_;
	}

private:
	friend class AtomicLink <T>;
	uintptr_t ptr_;
};

template <class T>
class AtomicLink
{
public:
	AtomicLink ()
	{}

	AtomicLink (Link <T> link) :
		ptr_ (link.ptr_)
	{}

	Link <T> load () const
	{
		Link <T> link;
		link.ptr_ = ptr_.load () & ~FlagBits <T>::spin;
		return link;
	}

	Link <T> operator = (Link <T> link)
	{
		assert ((ptr_.load () & FlagBits <T>::spin) == 0);
		ptr_ = link.ptr_;
		return link;
	}

	bool cas (const Link <T>& _cur, const Link <T>& _new)
	{
		uintptr_t cur = _cur.ptr_;
		assert ((cur & FlagBits <T>::spin) == 0);
		for (BackOff bo; !ptr_.compare_exchange_strong (cur, _new.ptr_); bo.sleep ()) {
			if ((cur & ~FlagBits <T>::spin) != _cur.ptr_)
				return false;
			cur = _cur.ptr_;
		}
		return true;
	}

	void lock ()
	{
		const uintptr_t MAX_SPIN = FlagBits <T>::spin;
		BackOff bo;
		uintptr_t cur = ptr_.load ();
		for (;;) {
			if ((cur & MAX_SPIN) < MAX_SPIN) {
				if (ptr_.compare_exchange_weak (cur, cur + 2, std::memory_order_acquire))
					break;
			} else
				bo.sleep ();
		}
	}

	void unlock ()
	{
		ptr_.fetch_sub (2, std::memory_order_release);
	}

private:
	volatile std::atomic <uintptr_t> ptr_;
};

}
}

#endif
