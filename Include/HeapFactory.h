#ifndef NIRVANA_HEAPFACTORY_H_
#define NIRVANA_HEAPFACTORY_H_

#include "Memory.h"

namespace Nirvana {

class HeapFactory;
typedef ::CORBA::Nirvana::T_ptr <HeapFactory> HeapFactory_ptr;
typedef ::CORBA::Nirvana::T_var <HeapFactory> HeapFactory_var;
typedef ::CORBA::Nirvana::T_out <HeapFactory> HeapFactory_out;

}

namespace CORBA {
namespace Nirvana {

template <>
class Bridge < ::Nirvana::HeapFactory> :
	public Bridge <Interface>
{
public:
	struct EPV
	{
		Bridge <Interface>::EPV interface;

		struct
		{
			Bridge <AbstractBase>* (*CORBA_AbstractBase) (Bridge < ::Nirvana::HeapFactory>*, EnvironmentBridge*);
		}
		base;

		struct
		{
			Bridge < ::Nirvana::Memory>* (*create) (Bridge <::Nirvana::HeapFactory>*, EnvironmentBridge*);
			Bridge < ::Nirvana::Memory>* (*create_with_granularity) (Bridge <::Nirvana::HeapFactory>*, ULong granularity, EnvironmentBridge*);
		}
		epv;
	};

	const EPV& _epv () const
	{
		return (EPV&)Bridge <Interface>::_epv ();
	}

	static const Char* _primary_interface ()
	{
		return "IDL:Nirvana/HeapFactory:1.0";
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
class Client <T, ::Nirvana::HeapFactory> :
	public ClientBase <T, ::Nirvana::HeapFactory>
{
public:
	T_ptr < ::Nirvana::Memory> create ();
	T_ptr < ::Nirvana::Memory> create_with_granularity (ULong granularity);
};

template <class T>
T_ptr < ::Nirvana::Memory> Client <T, ::Nirvana::HeapFactory>::create ()
{
	Environment _env;
	Bridge < ::Nirvana::HeapFactory>& _b = ClientBase <T, ::Nirvana::HeapFactory>::_bridge ();
	T_ptr < ::Nirvana::Memory> _ret = (_b._epv ().epv.create) (&_b, &_env);
	_env.check ();
	return _ret;
}

template <class T>
T_ptr < ::Nirvana::Memory> Client <T, ::Nirvana::HeapFactory>::create_with_granularity (ULong granularity)
{
	Environment _env;
	Bridge < ::Nirvana::HeapFactory>& _b = ClientBase <T, ::Nirvana::HeapFactory>::_bridge ();
	T_ptr < ::Nirvana::Memory> _ret = (_b._epv ().epv.create_with_granularity) (&_b, granularity, &_env);
	_env.check ();
	return _ret;
}

template <class S>
class Skeleton <S, ::Nirvana::HeapFactory>
{
public:
	static const typename Bridge < ::Nirvana::HeapFactory>::EPV epv_;

	template <class Base>
	static Bridge <Interface>* _find_interface (Base& base, const Char* id)
	{
		if (RepositoryId::compatible (Bridge < ::Nirvana::HeapFactory>::_primary_interface (), id))
			return &S::template _narrow < ::Nirvana::HeapFactory> (base);
		else
			return false;
	}

protected:
	static Bridge < ::Nirvana::Memory>* _create (Bridge < ::Nirvana::HeapFactory>* _b, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).create ();
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
		return 0;
	}

	static Bridge < ::Nirvana::Memory>* _create_with_granularity (Bridge < ::Nirvana::HeapFactory>* _b, ULong granularity, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).create_with_granularity (granularity);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
		return 0;
	}
};

template <class S>
const Bridge < ::Nirvana::HeapFactory>::EPV Skeleton <S, ::Nirvana::HeapFactory>::epv_ = {
	{ // interface
		S::template _duplicate < ::Nirvana::HeapFactory>,
		S::template _release < ::Nirvana::HeapFactory>
	},
	{ // base
		S::template _wide <AbstractBase, ::Nirvana::HeapFactory>
	},
	{ // epv
		S::_create,
		S::_create_with_granularity
	}
};

// Standard implementation

template <class S>
class Servant <S, ::Nirvana::HeapFactory> :
	public Implementation <S, ::Nirvana::HeapFactory>
{};

// POA implementation
template <>
class ServantPOA < ::Nirvana::HeapFactory> :
	public ImplementationPOA < ::Nirvana::HeapFactory>
{
public:
	virtual T_ptr < ::Nirvana::Memory> create () = 0;
	virtual T_ptr < ::Nirvana::Memory> create_with_granularity () = 0;
};

// Static implementation

template <class S>
class ServantStatic <S, ::Nirvana::HeapFactory> :
	public ImplementationStatic <S, ::Nirvana::HeapFactory>
{};

// Tied implementation

template <class T>
class ServantTied <T, ::Nirvana::HeapFactory> :
	public ImplementationTied <T, ::Nirvana::HeapFactory>
{
public:
	ServantTied (T* tp, Boolean release) :
		ImplementationTied <T, ::Nirvana::HeapFactory> (tp, release)
	{}
};

}
}

namespace Nirvana {

class HeapFactory :
	public ::CORBA::Nirvana::ClientInterface <HeapFactory>,
	public ::CORBA::Nirvana::Client <HeapFactory, ::CORBA::AbstractBase>
{
public:
	typedef HeapFactory_ptr _ptr_type;

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
