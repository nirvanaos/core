#ifndef NIRVANA_CORE_SCHEDULER_H_
#define NIRVANA_SCHEDULER_H_

#include "Runnable.h"
#include <Nirvana.h>

namespace Nirvana {
namespace Core {

class Scheduler;
typedef ::CORBA::Nirvana::T_ptr <Scheduler> Scheduler_ptr;
typedef ::CORBA::Nirvana::T_var <Scheduler> Scheduler_var;
typedef ::CORBA::Nirvana::T_out <Scheduler> Scheduler_out;

}
}

namespace CORBA {
namespace Nirvana {

template <>
class Bridge < ::Nirvana::Core::Scheduler> :
	public Bridge <Interface>
{
public:
	struct EPV
	{
		Bridge <Interface>::EPV interface;

		struct
		{
			Bridge <AbstractBase>* (*CORBA_AbstractBase) (Bridge < ::Nirvana::Core::Scheduler>*, EnvironmentBridge*);
		}
		base;

		struct
		{
			void (*schedule) (Bridge <::Nirvana::Core::Scheduler>*, ::Nirvana::DeadlineTime, Bridge <::Nirvana::Runnable>*, EnvironmentBridge*);
			void (*back_off) (Bridge <::Nirvana::Core::Scheduler>*, ULong hint, EnvironmentBridge*);
		}
		epv;
	};

	const EPV& _epv () const
	{
		return (EPV&)Bridge <Interface>::_epv ();
	}

	static const Char* _primary_interface ()
	{
		return "IDL:Nirvana/Core/Scheduler:1.0";
	}

protected:
	Bridge (const EPV& epv) :
		Bridge <Interface> (epv.interface)
	{}

	Bridge ()
	{}
};

template <class T>
class Client <T, ::Nirvana::Core::Scheduler> :
	public ClientBase <T, ::Nirvana::Core::Scheduler>
{
public:
	void schedule (::Nirvana::DeadlineTime, ::Nirvana::Runnable_ptr);
	void back_off (ULong);
};

template <class T>
void Client <T, ::Nirvana::Core::Scheduler>::schedule (::Nirvana::DeadlineTime deadline, ::Nirvana::Runnable_ptr runnable)
{
	Environment _env;
	Bridge < ::Nirvana::Core::Scheduler>& _b = ClientBase <T, ::Nirvana::Core::Scheduler>::_bridge ();
	(_b._epv ().epv.run) (&_b, deadline, runnable, &_env);
	_env.check ();
}

template <class T>
void Client <T, ::Nirvana::Core::Scheduler>::back_off (ULong hint)
{
	Environment _env;
	Bridge < ::Nirvana::Core::Scheduler>& _b = ClientBase <T, ::Nirvana::Core::Scheduler>::_bridge ();
	(_b._epv ().epv.back_off) (&_b, hint, &_env);
	_env.check ();
}

template <class S>
class Skeleton <S, ::Nirvana::Core::Scheduler>
{
public:
	static const typename Bridge < ::Nirvana::Core::Scheduler>::EPV epv_;

	template <class Base>
	static Bridge <Interface>* _find_interface (Base& base, const Char* id)
	{
		if (RepositoryId::compatible (Bridge < ::Nirvana::Core::Scheduler>::_primary_interface (), id))
			return &S::template _narrow < ::Nirvana::Core::Scheduler> (base);
		else
			return false;
	}

protected:
	static void _schedule (Bridge < ::Nirvana::Core::Scheduler>* _b, ::Nirvana::DeadlineTime deadline, Bridge < ::Nirvana::Runnable>* runnable, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).schedule (deadline, runnable);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
	}

	static void _back_off (Bridge < ::Nirvana::Core::Scheduler>* _b, ULong hint, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).back_off (hint);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
	}
};

template <class S>
const Bridge < ::Nirvana::Core::Scheduler>::EPV Skeleton <S, ::Nirvana::Core::Scheduler>::epv_ = {
	{ // interface
		S::template _duplicate < ::Nirvana::Core::Scheduler>,
		S::template _release < ::Nirvana::Core::Scheduler>
	},
	{ // base
		S::template _wide <AbstractBase, ::Nirvana::Core::Scheduler>
	},
	{ // epv
		S::_schedule,
		S::_back_off
	}
};

// Standard implementation

template <class S>
class Servant <S, ::Nirvana::Core::Scheduler> :
	public Implementation <S, ::Nirvana::Core::Scheduler>
{};

// Static implementation

template <class S>
class ServantStatic <S, ::Nirvana::Core::Scheduler> :
	public ImplementationStatic <S, ::Nirvana::Core::Scheduler>
{};

// Tied implementation

template <class T>
class ServantTied <T, ::Nirvana::Core::Scheduler> :
	public ImplementationTied <T, ::Nirvana::Memory>
{
public:
	ServantTied (T* tp, Boolean release) :
		ImplementationTied <T, ::Nirvana::Core::Scheduler> (tp, release)
	{}
};

}
}

namespace Nirvana {
namespace Core {

class Scheduler :
	public ::CORBA::Nirvana::ClientInterface <Scheduler>,
	public ::CORBA::Nirvana::Client <Scheduler, ::CORBA::AbstractBase>
{
public:
	typedef Scheduler_ptr _ptr_type;

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
}

#endif
