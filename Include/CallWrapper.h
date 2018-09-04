#ifndef NIRVANA_CALLWRAPPER_H_
#define NIRVANA_CALLWRAPPER_H_

#include <ORB.h>
#include <Nirvana.h>

namespace Nirvana {

class CallWrapper;
typedef ::CORBA::Nirvana::T_ptr <CallWrapper> CallWrapper_ptr;
typedef ::CORBA::Nirvana::T_var <CallWrapper> CallWrapper_var;
typedef ::CORBA::Nirvana::T_out <CallWrapper> CallWrapper_out;

}

namespace CORBA {
namespace Nirvana {

template <>
class Bridge < ::Nirvana::CallWrapper> :
	public Bridge <Interface>
{
public:
	struct EPV
	{
		Bridge <Interface>::EPV interface;

		struct
		{
			Bridge <AbstractBase>* (*CORBA_AbstractBase) (Bridge < ::Nirvana::CallWrapper>*, EnvironmentBridge*);
		}
		base;

		struct
		{
			void (*call) (Bridge <::Nirvana::CallWrapper>*, EnvironmentBridge*);
		}
		epv;
	};

	const EPV& _epv () const
	{
		return (EPV&)Bridge <Interface>::_epv ();
	}

	static const Char* _primary_interface ()
	{
		return "IDL:Nirvana/CallWrapper:1.0";
	}

protected:
	Bridge (const EPV& epv) :
		Bridge <Interface> (epv.interface)
	{}

	Bridge ()
	{}
};

template <class T>
class Client <T, ::Nirvana::CallWrapper> :
	public ClientBase <T, ::Nirvana::CallWrapper>
{
public:
	void call ();
};


template <class T>
void Client <T, ::Nirvana::CallWrapper>::call ()
{
	Environment _env;
	Bridge < ::Nirvana::CallWrapper>& _b = ClientBase <T, ::Nirvana::CallWrapper>::_bridge ();
	(_b._epv ().epv.call) (&_b, &_env);
	_env.check ();
}

template <class S>
class Skeleton <S, ::Nirvana::CallWrapper>
{
public:
	static const typename Bridge < ::Nirvana::CallWrapper>::EPV epv_;

	template <class Base>
	static Bridge <Interface>* _find_interface (Base& base, const Char* id)
	{
		if (RepositoryId::compatible (Bridge < ::Nirvana::CallWrapper>::_primary_interface (), id))
			return &S::template _narrow < ::Nirvana::CallWrapper> (base);
		else
			return false;
	}

protected:
	static void _call (Bridge < ::Nirvana::CallWrapper>* _b, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).call ();
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
		return 0;
	}
};

template <class S>
const Bridge < ::Nirvana::CallWrapper>::EPV Skeleton <S, ::Nirvana::CallWrapper>::epv_ = {
	{ // interface
		S::template _duplicate < ::Nirvana::CallWrapper>,
		S::template _release < ::Nirvana::CallWrapper>
	},
	{ // base
		S::template _wide <AbstractBase, ::Nirvana::CallWrapper>
	},
	{ // epv
		S::_call
	}
};

// Standard implementation

template <class S>
class Servant <S, ::Nirvana::CallWrapper> :
	public Implementation <S, ::Nirvana::CallWrapper>
{};

}
}

namespace Nirvana {

class CallWrapper :
	public ::CORBA::Nirvana::ClientInterface <CallWrapper>,
	public ::CORBA::Nirvana::Client <CallWrapper, ::CORBA::AbstractBase>
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
