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
			Bridge <Object>* (*CORBA_Object) (Bridge < ::Nirvana::Memory>*, EnvironmentBridge*);
		}
		base;

		struct
		{
			::Nirvana::Pointer (*allocate) (::Nirvana::Pointer dst, ::Nirvana::UWord size, Flags flags, EnvironmentBridge*);
			void (*commit) (::Nirvana::Pointer dst, ::Nirvana::UWord size, EnvironmentBridge*);
			void (*decommit) (::Nirvana::Pointer dst, ::Nirvana::UWord size, EnvironmentBridge*);
			void (*release) (::Nirvana::Pointer dst, ::Nirvana::UWord size, EnvironmentBridge*);
			::Nirvana::Pointer (*copy) (::Nirvana::Pointer dst, ::Nirvana::Pointer src, ::Nirvana::UWord size, Flags flags, EnvironmentBridge*);
			Boolean (*is_private) (::Nirvana::Pointer p, EnvironmentBridge*);
			Boolean (*is_copy) (::Nirvana::Pointer p1, ::Nirvana::Pointer p2, ::Nirvana::UWord size, EnvironmentBridge*);
			::Nirvana::Word (*query) (::Nirvana::Pointer p, QueryParam param, EnvironmentBridge*);
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
	Boolean is_private (::Nirvana::Pointer p);
	Boolean is_copy (::Nirvana::Pointer p1, ::Nirvana::Pointer p2, ::Nirvana::UWord size);
	::Nirvana::Word query (::Nirvana::Pointer p, Bridge < ::Nirvana::Memory>::QueryParam param);
};

template <class T>
::Nirvana::Pointer Client <T, ::Nirvana::Memory>::allocate (::Nirvana::Pointer dst, ::Nirvana::UWord size, Flags flags)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	::Nirvana::Pointer _ret = (_b._epv ().epv.allocate) (dst, size, flags, &_env);
	_env.check ();
	return _ret;
}

template <class T>
void Client <T, ::Nirvana::Memory>::commit (::Nirvana::Pointer dst, ::Nirvana::UWord size)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	(_b._epv ().epv.commit) (dst, size, &_env);
	_env.check ();
}

template <class T>
void Client <T, ::Nirvana::Memory>::decommit (::Nirvana::Pointer dst, ::Nirvana::UWord size)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	(_b._epv ().epv.decommit) (dst, size, &_env);
	_env.check ();
}

template <class T>
void Client <T, ::Nirvana::Memory>::release (::Nirvana::Pointer dst, ::Nirvana::UWord size)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	(_b._epv ().epv.release) (dst, size, &_env);
	_env.check ();
}

template <class T>
::Nirvana::Pointer Client <T, ::Nirvana::Memory>::copy (::Nirvana::Pointer dst, ::Nirvana::Pointer src, ::Nirvana::UWord size, Flags flags)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	::Nirvana::Pointer _ret = (_b._epv ().epv.copy) (dst, src, size, flags, &_env);
	_env.check ();
	return _ret;
}

template <class T>
Boolean Client <T, ::Nirvana::Memory>::is_private (::Nirvana::Pointer p)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	Boolean _ret = (_b._epv ().epv.is_private) (p, &_env);
	_env.check ();
	return _ret;
}

template <class T>
Boolean Client <T, ::Nirvana::Memory>::is_copy (::Nirvana::Pointer p1, ::Nirvana::Pointer p2, ::Nirvana::UWord size)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	Boolean _ret = (_b._epv ().epv.is_copy) (p1, p2, size, &_env);
	_env.check ();
	return _ret;
}

template <class T>
::Nirvana::Word Client <T, ::Nirvana::Memory>::query (::Nirvana::Pointer p, Bridge < ::Nirvana::Memory>::QueryParam param)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	::Nirvana::Word _ret = (_b._epv ().epv.query) (p, param, &_env);
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
			return Skeleton <S, Object>::_find_interface (base, id);
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

	static void _release (Bridge < ::Nirvana::Memory>* _b, ::Nirvana::Pointer dst, ::Nirvana::UWord size, EnvironmentBridge* _env)
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

	static Boolean _is_private (Bridge < ::Nirvana::Memory>* _b, ::Nirvana::Pointer p, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).is_private (p);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
		return 0;
	}

	static Boolean _is_copy (Bridge < ::Nirvana::Memory>* _b, ::Nirvana::Pointer p1, ::Nirvana::Pointer p2, ::Nirvana::UWord size, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).is_copy (p1, p2);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
		return 0;
	}
	
	static ::Nirvana::Word _query (Bridge < ::Nirvana::Memory>* _b, ::Nirvana::Pointer p, Bridge < ::Nirvana::Memory>::QueryParam param, EnvironmentBridge* _env)
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
		S::template _wide <Object, ::Nirvana::Memory>
	},
	{ // epv
		S::_allocate,
		S::_commit,
		S::_decommit,
		S::_release,
		S::_copy,
		S::_is_private,
		S::_is_copy,
		S::_query
	}
};

}
}

namespace Nirvana {

class Memory :
	public ::CORBA::Nirvana::ClientInterface <Memory>,
	public ::CORBA::Nirvana::Client <Memory, ::CORBA::Object>
{
public:
	typedef Memory_ptr _ptr_type;

	operator ::CORBA::Object& ()
	{
		::CORBA::Environment _env;
		::CORBA::Object* _ret = static_cast < ::CORBA::Object*> ((_epv ().base.CORBA_Object) (this, &_env));
		_env.check ();
		assert (_ret);
		return *_ret;
	}
};

}

#endif

