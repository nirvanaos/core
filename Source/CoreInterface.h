#ifndef NIRVANA_CORE_COREINTERFACE_H_
#define NIRVANA_CORE_COREINTERFACE_H_

#include "core.h"
#include "AtomicCounter.h"

namespace Nirvana {
namespace Core {

/// Core interface.
class CoreInterface
{
public:
	virtual void _add_ref () = 0;
	virtual void _remove_ref () = 0;
};

/// Core smart pointer.
template <class I>
class Core_var
{
public:
	Core_var () :
		p_ (nullptr)
	{}

	Core_var (I* p) :
		p_ (p)
	{}

	Core_var (const Core_var& src) :
		Core_var (src.p_)
	{
		if (p_)
			p_->_add_ref ();
	}

	Core_var (Core_var&& src) :
		p_ (src.p_)
	{
		src.p_ = nullptr;
	}

	~Core_var ()
	{
		if (p_)
			p_->_remove_ref ();
	}

	Core_var& operator = (I* p)
	{
		if (p_ != p)
			reset (p);
		return *this;
	}

	Core_var& operator = (const Core_var& src)
	{
		I* p = src.p_;
		if (p_ != p) {
			reset (p);
			if (p)
				p->_add_ref ();
		}
		return *this;
	}

	Core_var& operator = (Core_var&& src)
	{
		if (this != &src) {
			reset (src.p_);
			src.p_ = nullptr;
		}
		return *this;
	}

	I* operator -> () const
	{
		return p_;
	}

	operator I* () const
	{
		return p_;
	}

	void reset ()
	{
		if (p_) {
			I* tmp = p_;
			p_ = nullptr;
			tmp->_remove_ref ();
		}
	}

private:
	void reset (I* p)
	{
		if (p != p_) {
			I* tmp = p_;
			p_ = p;
			if (tmp)
				tmp->_remove_ref ();
		}
	}

private:
	I* p_;
};

/// Dynamic implementation of a core object.
/// \tparam T object class.
/// \tparam I... interfaces.
template <class T, class ... I>
class ImplDynamic : 
	public CoreObject, // Allocate memory from g_core_heap
	public I...
{
public:
	void _add_ref ()
	{
		ref_cnt_.increment ();
	}

	void _remove_ref ()
	{
		if (!ref_cnt_.decrement ())
			delete static_cast <T*> (this);
	}

private:
	RefCounter ref_cnt_;
};

/// Static or stack implementation of a core object.
/// \tparam I... interfaces.
template <class ... I>
class ImplStatic :
	public I...
{
public:
	void _add_ref ()
	{}

	void _remove_ref ()
	{}
};

}
}

#endif
