#ifndef NIRVANA_MEMORY_H_
#define NIRVANA_MEMORY_H_

#include <ORB.h>

namespace Nirvana {

class Memory;
typedef ::CORBA::Nirvana::T_ptr <Memory> Memory_ptr;

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
			Pointer (*allocate) (Pointer dst, UWord size, Flags flags, EnvironmentBridge*);
			void (*commit) (Pointer dst, UWord size, EnvironmentBridge*);
			void (*decommit) (Pointer dst, UWord size, EnvironmentBridge*);
			void (*release) (Pointer dst, UWord size, EnvironmentBridge*);
			Pointer (*copy) (Pointer dst, Pointer src, UWord size, Flags flags, EnvironmentBridge*);
			Boolean (*is_private) (Pointer p, EnvironmentBridge*);
			Boolean (*is_copy) (Pointer p1, Pointer p2, UWord size, EnvironmentBridge*);
			Word (*query) (Pointer p, QueryParam param, EnvironmentBridge*);
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
	Pointer allocate (Pointer dst, UWord size, Flags flags);
	void commit (Pointer dst, UWord size);
	void decommit (Pointer dst, UWord size);
	void release (Pointer dst, UWord size);
	Pointer copy (Pointer dst, Pointer src, UWord size, Flags flags);
	Boolean is_private (Pointer p);
	Boolean is_copy (Pointer p1, Pointer p2, UWord size);
	Word query (Pointer p, Bridge < ::Nirvana::Memory>::QueryParam param);
};

template <class T>
Pointer Client <T, ::Nirvana::Memory>::allocate (Pointer dst, UWord size, Flags flags)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	Pointer _ret = (_b._epv ().epv.allocate) (dst, size, flags, &_env);
	_env.check ();
	return _ret;
}

template <class T>
void Client <T, ::Nirvana::Memory>::commit (Pointer dst, UWord size)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	(_b._epv ().epv.commit) (dst, size, &_env);
	_env.check ();
}

template <class T>
void Client <T, ::Nirvana::Memory>::decommit (Pointer dst, UWord size)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	(_b._epv ().epv.decommit) (dst, size, &_env);
	_env.check ();
}

template <class T>
void Client <T, ::Nirvana::Memory>::release (Pointer dst, UWord size)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	(_b._epv ().epv.release) (dst, size, &_env);
	_env.check ();
}

template <class T>
Pointer Client <T, ::Nirvana::Memory>::copy (Pointer dst, Pointer src, UWord size, Flags flags)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	Pointer _ret = (_b._epv ().epv.copy) (dst, src, size, flags, &_env);
	_env.check ();
	return _ret;
}

template <class T>
Boolean Client <T, ::Nirvana::Memory>::is_private (Pointer p)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	Boolean _ret = (_b._epv ().epv.is_private) (p, &_env);
	_env.check ();
	return _ret;
}

template <class T>
Boolean Client <T, ::Nirvana::Memory>::is_copy (Pointer p1, Pointer p2, UWord size)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	Boolean _ret = (_b._epv ().epv.is_copy) (p1, p2, size, &_env);
	_env.check ();
	return _ret;
}

template <class T>
Word Client <T, ::Nirvana::Memory>::query (Pointer p, Bridge < ::Nirvana::Memory>::QueryParam param)
{
	Environment _env;
	Bridge < ::Nirvana::Memory>& _b = ClientBase <T, ::Nirvana::Memory>::_bridge ();
	Word _ret = (_b._epv ().epv.query) (p, param, &_env);
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
			return &S::_narrow < ::Nirvana::Memory> (base);
		else
			return Skeleton <S, Object>::_find_interface (base, id);
	}

protected:
	static Pointer _allocate (Bridge < ::Nirvana::Memory>* _b, Pointer dst, UWord size, Flags flags, EnvironmentBridge* _env)
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

	static void _commit (Bridge < ::Nirvana::Memory>* _b, Pointer dst, UWord size, EnvironmentBridge* _env)
	{
		try {
			S::_implementation (_b).commit (dst, size);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
	}

	static void _decommit (Pointer dst, UWord size, EnvironmentBridge* _env)
	{
		try {
			S::_implementation (_b).decommit (dst, size);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
	}

	static void _release (Pointer dst, UWord size, EnvironmentBridge* _env)
	{
		try {
			S::_implementation (_b).release (dst, size);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
	}

	static Pointer _copy (Pointer dst, Pointer src, UWord size, Flags flags, EnvironmentBridge* _env)
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

	static Boolean _is_private (Pointer p, EnvironmentBridge* _env)
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

	static Boolean _is_copy (Pointer p1, Pointer p2, UWord size, EnvironmentBridge* _env)
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
	
	static Word _query (Pointer p, Bridge < ::Nirvana::Memory>::QueryParam param, EnvironmentBridge* _env)
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
		S::_duplicate < ::Nirvana::Memory>,
		S::_release < ::Nirvana::Memory>
	},
	{ // base
		S::_wide <Object, ::Nirvana::Memory>
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

