#ifndef NIRVANA_MEMORY_H_
#define NIRVANA_MEMORY_H_

#include <ORB.h>
#include <Nirvana.h>

namespace Nirvana {

class Memory;
typedef ::CORBA::Nirvana::T_ptr <Memory> Memory_ptr;
typedef ::CORBA::Nirvana::T_var <Memory> Memory_var;
typedef ::CORBA::Nirvana::T_out <Memory> Memory_out;

}

namespace CORBA {
namespace Nirvana {

template <>
class Bridge < ::Nirvana::Memory> :
	public Bridge <Interface>
{
public:
	enum
	{
		READ_WRITE = 0x00,
		READ_ONLY = 0x01,
		RESERVED = 0x02,

		ALLOCATE = 0x08,
		DECOMMIT = 0x10,
		RELEASE = 0x30,

		ZERO_INIT = 0x40,
		EXACTLY = 0x80
	};

	enum QueryParam
	{
		ALLOCATION_UNIT,
		PROTECTION_UNIT,
		COMMIT_UNIT,
		OPTIMAL_COMMIT_UNIT,
		SHARING_UNIT,
		SHARING_ASSOCIATIVITY,
		GRANULARITY,
		ALLOCATION_SPACE_BEGIN,
		ALLOCATION_SPACE_END,
		FLAGS
	};

	// Flags
	enum
	{
		HARDWARE_PROTECTION = 0x0001,
		COPY_ON_WRITE = 0x0002,
		SPACE_RESERVATION = 0x0004,
		ACCESS_CHECK = 0x0008
	};

	struct EPV
	{
		Bridge <Interface>::EPV interface;

		struct
		{
			Bridge <AbstractBase>* (*CORBA_AbstractBase) (Bridge < ::Nirvana::Memory>*, EnvironmentBridge*);
		}
		base;

		struct
		{
			::Nirvana::Pointer (*allocate) (Bridge <::Nirvana::Memory>*, ::Nirvana::Pointer dst, ::Nirvana::UWord size, Flags flags, EnvironmentBridge*);
			void (*commit) (Bridge <::Nirvana::Memory>*, ::Nirvana::Pointer dst, ::Nirvana::UWord size, EnvironmentBridge*);
			void (*decommit) (Bridge <::Nirvana::Memory>*, ::Nirvana::Pointer dst, ::Nirvana::UWord size, EnvironmentBridge*);
			void (*release) (Bridge <::Nirvana::Memory>*, ::Nirvana::Pointer dst, ::Nirvana::UWord size, EnvironmentBridge*);
			::Nirvana::Pointer (*copy) (Bridge <::Nirvana::Memory>*, ::Nirvana::Pointer dst, ::Nirvana::Pointer src, ::Nirvana::UWord size, Flags flags, EnvironmentBridge*);
			Boolean (*is_readable) (Bridge <::Nirvana::Memory>*, ::Nirvana::ConstPointer p, ::Nirvana::UWord size, EnvironmentBridge*);
			Boolean (*is_writable) (Bridge <::Nirvana::Memory>*, ::Nirvana::ConstPointer p, ::Nirvana::UWord size, EnvironmentBridge*);
			Boolean (*is_private) (Bridge <::Nirvana::Memory>*, ::Nirvana::ConstPointer p, ::Nirvana::UWord size, EnvironmentBridge*);
			Boolean (*is_copy) (Bridge <::Nirvana::Memory>*, ::Nirvana::ConstPointer p1, ::Nirvana::ConstPointer p2, ::Nirvana::UWord size, EnvironmentBridge*);
			::Nirvana::Word (*query) (Bridge <::Nirvana::Memory>*, ::Nirvana::ConstPointer p, QueryParam param, EnvironmentBridge*);
		}
		epv;
	};

	const EPV& _epv () const
	{
		return (EPV&)Bridge <Interface>::_epv ();
	}

	static const Char* _primary_interface ()
	{
		return "IDL:Nirvana/Memory:1.0";
	}

	static Boolean ___is_a (const Char* id)
	{
		if (RepositoryId::compatible (_primary_interface (), id))
			return TRUE;
		// Here we must call all base classes
		return FALSE;
	}

protected:
	Bridge (const EPV& epv) :
		Bridge <Interface> (epv.interface)
	{}

	Bridge ()
	{}
};

template <class T>
class Client <T, ::Nirvana::Memory> :
	public ClientBase <T, ::Nirvana::Memory>
{
public:
	::Nirvana::Pointer allocate (::Nirvana::Pointer dst, ::Nirvana::UWord size, Flags flags);
	void commit (::Nirvana::Pointer dst, ::Nirvana::UWord size);
	void decommit (::Nirvana::Pointer dst, ::Nirvana::UWord size);
	void release (::Nirvana::Pointer dst, ::Nirvana::UWord size);
	::Nirvana::Pointer copy (::Nirvana::Pointer dst, ::Nirvana::Pointer src, ::Nirvana::UWord size, Flags flags);
	Boolean is_readable (::Nirvana::ConstPointer p, ::Nirvana::UWord size);
	Boolean is_writable (::Nirvana::ConstPointer p, ::Nirvana::UWord size);
	Boolean is_private (::Nirvana::ConstPointer p, ::Nirvana::UWord size);
	Boolean is_copy (::Nirvana::ConstPointer p1, ::Nirvana::ConstPointer p2, ::Nirvana::UWord size);
	::Nirvana::Word query (::Nirvana::ConstPointer p, Bridge < ::Nirvana::Memory>::QueryParam param);
};

template <class T>
::Nirvana::Pointer Client <T, ::Nirvana::Memory>::allocate (::Nirvana::Pointer dst, ::Nirvana::UWord size, Flags flags)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	::Nirvana::Pointer _ret = (_b._epv ().epv.allocate) (&_b, dst, size, flags, &_env);
	_env.check ();
	return _ret;
}

template <class T>
void Client <T, ::Nirvana::Memory>::commit (::Nirvana::Pointer dst, ::Nirvana::UWord size)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	(_b._epv ().epv.commit) (&_b, dst, size, &_env);
	_env.check ();
}

template <class T>
void Client <T, ::Nirvana::Memory>::decommit (::Nirvana::Pointer dst, ::Nirvana::UWord size)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	(_b._epv ().epv.decommit) (&_b, dst, size, &_env);
	_env.check ();
}

template <class T>
void Client <T, ::Nirvana::Memory>::release (::Nirvana::Pointer dst, ::Nirvana::UWord size)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	(_b._epv ().epv.release) (&_b, dst, size, &_env);
	_env.check ();
}

template <class T>
::Nirvana::Pointer Client <T, ::Nirvana::Memory>::copy (::Nirvana::Pointer dst, ::Nirvana::Pointer src, ::Nirvana::UWord size, Flags flags)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	::Nirvana::Pointer _ret = (_b._epv ().epv.copy) (&_b, dst, src, size, flags, &_env);
	_env.check ();
	return _ret;
}

template <class T>
Boolean Client <T, ::Nirvana::Memory>::is_readable (::Nirvana::ConstPointer p, ::Nirvana::UWord size)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	Boolean _ret = (_b._epv ().epv.is_readable) (&_b, p, size, &_env);
	_env.check ();
	return _ret;
}

template <class T>
Boolean Client <T, ::Nirvana::Memory>::is_writable (::Nirvana::ConstPointer p, ::Nirvana::UWord size)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	Boolean _ret = (_b._epv ().epv.is_writable) (&_b, p, size, &_env);
	_env.check ();
	return _ret;
}

template <class T>
Boolean Client <T, ::Nirvana::Memory>::is_private (::Nirvana::ConstPointer p, ::Nirvana::UWord size)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	Boolean _ret = (_b._epv ().epv.is_private) (&_b, p, size, &_env);
	_env.check ();
	return _ret;
}

template <class T>
Boolean Client <T, ::Nirvana::Memory>::is_copy (::Nirvana::ConstPointer p1, ::Nirvana::ConstPointer p2, ::Nirvana::UWord size)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	Boolean _ret = (_b._epv ().epv.is_copy) (&_b, p1, p2, size, &_env);
	_env.check ();
	return _ret;
}

template <class T>
::Nirvana::Word Client <T, ::Nirvana::Memory>::query (::Nirvana::ConstPointer p, Bridge < ::Nirvana::Memory>::QueryParam param)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	::Nirvana::Word _ret = (_b._epv ().epv.query) (&_b, p, param, &_env);
	_env.check ();
	return _ret;
}

template <class S>
class Skeleton <S, ::Nirvana::Memory>
{
public:
	static const typename Bridge < ::Nirvana::Memory>::EPV sm_epv;

	template <class Base>
	static Bridge <Interface>* _find_interface (Base& base, const Char* id)
	{
		if (RepositoryId::compatible (Bridge < ::Nirvana::Memory>::_primary_interface (), id))
			return &S::template _narrow < ::Nirvana::Memory> (base);
		else
			return false;
	}

protected:
	static ::Nirvana::Pointer _allocate (Bridge < ::Nirvana::Memory>* _b, ::Nirvana::Pointer dst, ::Nirvana::UWord size, Flags flags, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).allocate (dst, size, flags);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
		return 0;
	}

	static void _commit (Bridge < ::Nirvana::Memory>* _b, ::Nirvana::Pointer dst, ::Nirvana::UWord size, EnvironmentBridge* _env)
	{
		try {
			S::_implementation (_b).commit (dst, size);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
	}

	static void _decommit (Bridge < ::Nirvana::Memory>* _b, ::Nirvana::Pointer dst, ::Nirvana::UWord size, EnvironmentBridge* _env)
	{
		try {
			S::_implementation (_b).decommit (dst, size);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
	}

	static void _Memory_release (Bridge < ::Nirvana::Memory>* _b, ::Nirvana::Pointer dst, ::Nirvana::UWord size, EnvironmentBridge* _env)
	{
		try {
			S::_implementation (_b).release (dst, size);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
	}

	static ::Nirvana::Pointer _copy (Bridge < ::Nirvana::Memory>* _b, ::Nirvana::Pointer dst, ::Nirvana::Pointer src, ::Nirvana::UWord size, Flags flags, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).copy (dst, src, size, flags);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
		return 0;
	}

	static Boolean _is_readable (Bridge < ::Nirvana::Memory>* _b, ::Nirvana::ConstPointer p, ::Nirvana::UWord size, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).is_readable (p, size);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
		return 0;
	}

	static Boolean _is_writable (Bridge < ::Nirvana::Memory>* _b, ::Nirvana::ConstPointer p, ::Nirvana::UWord size, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).is_writable (p, size);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
		return 0;
	}

	static Boolean _is_private (Bridge < ::Nirvana::Memory>* _b, ::Nirvana::ConstPointer p, ::Nirvana::UWord size, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).is_private (p, size);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
		return 0;
	}

	static Boolean _is_copy (Bridge < ::Nirvana::Memory>* _b, ::Nirvana::ConstPointer p1, ::Nirvana::ConstPointer p2, ::Nirvana::UWord size, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).is_copy (p1, p2, size);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
		return 0;
	}
	
	static ::Nirvana::Word _query (Bridge < ::Nirvana::Memory>* _b, ::Nirvana::ConstPointer p, Bridge < ::Nirvana::Memory>::QueryParam param, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).query (p, param);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
		return 0;
	}
};

template <class S>
const Bridge < ::Nirvana::Memory>::EPV Skeleton <S, ::Nirvana::Memory>::sm_epv = {
	{ // interface
		S::template _duplicate < ::Nirvana::Memory>,
		S::template _release < ::Nirvana::Memory>
	},
	{ // base
		S::template _wide <AbstractBase, ::Nirvana::Memory>
	},
	{ // epv
		S::_allocate,
		S::_commit,
		S::_decommit,
		S::_Memory_release,
		S::_copy,
		S::_is_readable,
		S::_is_writable,
		S::_is_private,
		S::_is_copy,
		S::_query
	}
};

// Standard implementation

template <class S>
class Servant <S, ::Nirvana::Memory> :
	public Implementation <S, ::Nirvana::Memory>
{};

// POA implementation
/*
template <>
class ServantPOA < ::Nirvana::Memory> :
	public ImplementationPOA < ::Nirvana::Memory>
{
public:
	virtual ::Nirvana::Pointer allocate (::Nirvana::Pointer dst, ::Nirvana::UWord size, Flags flags) = 0;
	virtual void commit (::Nirvana::Pointer dst, ::Nirvana::UWord size) = 0;
	virtual void decommit (::Nirvana::Pointer dst, ::Nirvana::UWord size) = 0;
	virtual void release (::Nirvana::Pointer dst, ::Nirvana::UWord size) = 0;
	virtual ::Nirvana::Pointer copy (::Nirvana::Pointer dst, ::Nirvana::Pointer src, ::Nirvana::UWord size, Flags flags) = 0;
	virtual Boolean is_readable (::Nirvana::ConstPointer p, ::Nirvana::UWord size) = 0;
	virtual Boolean is_writable (::Nirvana::ConstPointer p, ::Nirvana::UWord size) = 0;
	virtual Boolean is_private (::Nirvana::ConstPointer p, ::Nirvana::UWord size) = 0;
	virtual Boolean is_copy (::Nirvana::ConstPointer p1, ::Nirvana::ConstPointer p2, ::Nirvana::UWord size) = 0;
	virtual ::Nirvana::Word query (::Nirvana::ConstPointer p, Bridge < ::Nirvana::Memory>::QueryParam param) = 0;
};
*/
// Static implementation

template <class S>
class ServantStatic <S, ::Nirvana::Memory> :
	public ImplementationStatic <S, ::Nirvana::Memory>
{};

// Tied implementation

template <class T>
class ServantTied <T, ::Nirvana::Memory> :
	public ImplementationTied <T, ::Nirvana::Memory>
{
public:
	ServantTied (T* tp, Boolean release) :
		ImplementationTied <T, ::Nirvana::Memory> (tp, release)
	{}
};

}
}

namespace Nirvana {

class Memory :
	public ::CORBA::Nirvana::ClientInterface <Memory>,
	public ::CORBA::Nirvana::Client <Memory, ::CORBA::AbstractBase>
{
public:
	typedef Memory_ptr _ptr_type;

	operator ::CORBA::AbstractBase& ()
	{
		::CORBA::Environment _env;
		::CORBA::AbstractBase* _ret = static_cast < ::CORBA::AbstractBase*> ((_epv ().base.CORBA_AbstractBase) (this, &_env));
		_env.check ();
		assert (_ret);
		return *_ret;
	}
};

}

#endif

