#ifndef NIRVANA_CORE_ATOMICCOUNTER_H_
#define NIRVANA_CORE_ATOMICCOUNTER_H_

#include "core.h"
#include <atomic>

namespace Nirvana {
namespace Core {

class AtomicCounter
{
public:
	typedef UWord UIntType;
	typedef Word IntType;

	AtomicCounter (IntType init) :
		cnt_ (init)
	{
		assert (cnt_.is_lock_free ());
	}

	explicit operator IntType () const
	{
		return cnt_;
	}

	explicit operator UIntType () const
	{
		return cnt_;
	}

	operator bool () const
	{
		return cnt_ != 0;
	}

	IntType increment ()
	{
		return ++cnt_;
	}

	IntType decrement ()
	{
		return --cnt_;
	}

private:
	std::atomic <Word> cnt_;
};

//! Reference counter
//! Initialized with 1.
class RefCounter : public AtomicCounter
{
public:
	RefCounter () :
		AtomicCounter (1)
	{}

	RefCounter (const RefCounter&) :
		AtomicCounter (1)
	{}

	RefCounter& operator = (const RefCounter&)
	{
		return *this;
	}

	UIntType decrement ()
	{
		assert ((UIntType)*this > 0);
		return AtomicCounter::decrement ();
	}

	UIntType increment ()
	{
		return AtomicCounter::increment ();
	}
};

}
}

#endif
