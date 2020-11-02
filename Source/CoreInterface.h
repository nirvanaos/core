#ifndef NIRVANA_CORE_COREINTERFACE_H_
#define NIRVANA_CORE_COREINTERFACE_H_

#include "AtomicCounter.h"

namespace Nirvana {
namespace Core {

template <class> class Core_var;

/// Core interface.
class NIRVANA_NOVTABLE CoreInterface
{
protected:
	template <class> friend class Core_var;
	friend class ExecDomain;

	virtual void _add_ref () = 0;
	virtual void _remove_ref () = 0;
};

/// Core smart pointer.
/// \tparam T object or core interface class.
template <class T>
class Core_var
{
	template <class T1> friend class Core_var;
public:
	Core_var () NIRVANA_NOEXCEPT :
		p_ (nullptr)
	{}

	/// Increments reference counter unlike I_var.
	Core_var (T* p) NIRVANA_NOEXCEPT :
		p_ (p)
	{
		if (p_)
			p_->_add_ref ();
	}

	template <class T1>
	Core_var (const Core_var <T1>& src) NIRVANA_NOEXCEPT :
		Core_var (src.p_)
	{
		if (p_)
			p_->_add_ref ();
	}

	template <class T1>
	Core_var (Core_var <T1>&& src) NIRVANA_NOEXCEPT :
		p_ (src.p_)
	{
		src.p_ = nullptr;
	}

	/// Creates the object.
	/// \tparam Impl Object implementation class.
	template <class Impl, class ... Args>
	static Core_var create (Args ... args)
	{
		Core_var v;
		v.p_ = new Impl (std::forward <Args> (args)...);
		return v;
	}

	~Core_var () NIRVANA_NOEXCEPT
	{
		if (p_)
			p_->_remove_ref ();
	}

	/// Increments reference counter unlike I_var.
	Core_var& operator = (T* p) NIRVANA_NOEXCEPT
	{
		if (p_ != p) {
			reset (p);
			if (p)
				p->_add_ref ();
		}
		return *this;
	}

	template <class T1>
	Core_var& operator = (const Core_var <T1>& src) NIRVANA_NOEXCEPT
	{
		T* p = src.p_;
		if (p_ != p) {
			reset (p);
			if (p)
				p->_add_ref ();
		}
		return *this;
	}

	template <class T1>
	Core_var& operator = (Core_var <T1>&& src) NIRVANA_NOEXCEPT
	{
		if (this != &src) {
			reset (src.p_);
			src.p_ = nullptr;
		}
		return *this;
	}

	T* operator -> () const NIRVANA_NOEXCEPT
	{
		return p_;
	}

	operator T* () const NIRVANA_NOEXCEPT
	{
		return p_;
	}

	T& operator * () const NIRVANA_NOEXCEPT
	{
		assert (p_);
		return *p_;
	}

	void reset () NIRVANA_NOEXCEPT
	{
		if (p_) {
			T* tmp = p_;
			p_ = nullptr;
			tmp->_remove_ref ();
		}
	}

private:
	void reset (T* p) NIRVANA_NOEXCEPT
	{
		if (p != p_) {
			T* tmp = p_;
			p_ = p;
			if (tmp)
				tmp->_remove_ref ();
		}
	}

private:
	T* p_;
};

/// Dynamic implementation of a core object.
/// \tparam T object class.
template <class T>
class ImplDynamic : 
	public T
{
private:
	template <class> friend class Core_var;

	template <class ... Args>
	ImplDynamic (Args ... args) :
		T (std::forward <Args> (args)...)
	{}

	void _add_ref ()
	{
		ref_cnt_.increment ();
	}

	void _remove_ref ()
	{
		if (!ref_cnt_.decrement ())
			delete this;
	}

private:
	RefCounter ref_cnt_;
};

/// Static or stack implementation of a core object.
/// \tparam T object class.
template <class T>
class ImplStatic :
	public T
{
public:
	template <class ... Args>
	ImplStatic (Args ... args) :
		T (std::forward <Args> (args)...)
	{}

private:
	void _add_ref ()
	{}

	void _remove_ref ()
	{}
};

}
}

#endif
