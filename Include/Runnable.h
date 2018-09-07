#ifndef NIRVANA_CALLWRAPPER_H_
#define NIRVANA_CALLWRAPPER_H_

#include <ORB.h>
#include <Nirvana.h>

namespace Nirvana {

class Runnable;
typedef ::CORBA::Nirvana::T_ptr <Runnable> CallWrapper_ptr;
typedef ::CORBA::Nirvana::T_var <Runnable> CallWrapper_var;
typedef ::CORBA::Nirvana::T_out <Runnable> CallWrapper_out;

}

namespace CORBA {
namespace Nirvana {

template <>
class Bridge < ::Nirvana::Runnable> :
	public Bridge <Interface>
{
public:
	struct EPV
	{
		Bridge <Interface>::EPV interface;

		struct
		{
			Bridge <AbstractBase>* (*CORBA_AbstractBase) (Bridge < ::Nirvana::Runnable>*, EnvironmentBridge*);
		}
		base;

		struct
		{
			void (*run) (Bridge <::Nirvana::Runnable>*, EnvironmentBridge*);
		}
		epv;
	};

	const EPV& _epv () const
	{
		return (EPV&)Bridge <Interface>::_epv ();
	}

	static const Char* _primary_interface ()
	{
		return "IDL:Nirvana/Runnable:1.0";
	}

protected:
	Bridge (const EPV& epv) :
		Bridge <Interface> (epv.interface)
	{}

	Bridge ()
	{}
};

template <class T>
class Client <T, ::Nirvana::Runnable> :
	public ClientBase <T, ::Nirvana::Runnable>
{
public:
	void run ();
};


template <class T>
void Client <T, ::Nirvana::Runnable>::run ()
{
	Environment _env;
	Bridge < ::Nirvana::Runnable>& _b = ClientBase <T, ::Nirvana::Runnable>::_bridge ();
	(_b._epv ().epv.run) (&_b, &_env);
	_env.check ();
}

template <class S>
class Skeleton <S, ::Nirvana::Runnable>
{
public:
	static const typename Bridge < ::Nirvana::Runnable>::EPV epv_;

	template <class Base>
	static Bridge <Interface>* _find_interface (Base& base, const Char* id)
	{
		if (RepositoryId::compatible (Bridge < ::Nirvana::Runnable>::_primary_interface (), id))
			return &S::template _narrow < ::Nirvana::Runnable> (base);
		else
			return false;
	}

protected:
	static void _call (Bridge < ::Nirvana::Runnable>* _b, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).run ();
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
		return 0;
	}
};

template <class S>
const Bridge < ::Nirvana::Runnable>::EPV Skeleton <S, ::Nirvana::Runnable>::epv_ = {
	{ // interface
		S::template _duplicate < ::Nirvana::Runnable>,
		S::template _release < ::Nirvana::Runnable>
	},
	{ // base
		S::template _wide <AbstractBase, ::Nirvana::Runnable>
	},
	{ // epv
		S::_call
	}
};

// Standard implementation

template <class S>
class Servant <S, ::Nirvana::Runnable> :
	public Implementation <S, ::Nirvana::Runnable>
{};

}
}

namespace Nirvana {

class Runnable :
	public ::CORBA::Nirvana::ClientInterface <Runnable>,
	public ::CORBA::Nirvana::Client <Runnable, ::CORBA::AbstractBase>
{
public:
	typedef CallWrapper_ptr _ptr_type;

	operator ::CORBA::AbstractBase& ()
	{
		::CORBA::Environment _env;
		::CORBA::AbstractBase* _ret = static_cast <::CORBA::AbstractBase*> ((_epv ().base.CORBA_AbstractBase) (this, &_env));
		_env.check ();
		assert (_ret);
		return *_ret;
	}
};

}

#endif
