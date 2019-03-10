/// The Nirvana project.
/// Core scheduler interfaces.

#ifndef NIRVANA_CORE_SCHEDULER_H_
#define NIRVANA_CORE_SCHEDULER_H_

#include <ORB.h>

/// \interface Executor

namespace Nirvana {
namespace Core {

class Executor;
typedef ::CORBA::Nirvana::T_ptr <Executor> Executor_ptr;
typedef ::CORBA::Nirvana::T_var <Executor> Executor_var;
typedef ::CORBA::Nirvana::T_out <Executor> Executor_out;

}
}

namespace CORBA {
namespace Nirvana {

template <>
class Bridge < ::Nirvana::Core::Executor> :
	public Bridge <Interface>
{
public:
	struct EPV
	{
		Bridge <Interface>::EPV interface;

		struct
		{
			Bridge <AbstractBase>* (*CORBA_AbstractBase) (Bridge < ::Nirvana::Core::Executor>*, EnvironmentBridge*);
		}
		base;

		struct
		{
			void (*execute) (Bridge <::Nirvana::Core::Executor>*, ::Nirvana::DeadlineTime, EnvironmentBridge*);
		}
		epv;
	};

	const EPV& _epv () const
	{
		return (EPV&)Bridge <Interface>::_epv ();
	}

	static const Char* _primary_interface ()
	{
		return "IDL:Nirvana/Core/Executor:1.0";
	}

protected:
	Bridge (const EPV& epv) :
		Bridge <Interface> (epv.interface)
	{}
};

template <class T>
class Client <T, ::Nirvana::Core::Executor> :
	public T
{
public:
	void execute (::Nirvana::DeadlineTime deadline);
};

template <class T>
void Client <T, ::Nirvana::Core::Executor>::execute (::Nirvana::DeadlineTime deadline)
{
	Environment _env;
	Bridge < ::Nirvana::Core::Executor>& _b = ClientBase <T, ::Nirvana::Core::Executor>::_bridge ();
	(_b._epv ().epv.execute) (&_b, deadline, &_env);
	_env.check ();
}

template <class S>
class Skeleton <S, ::Nirvana::Core::Executor>
{
public:
	static const typename Bridge < ::Nirvana::Core::Executor>::EPV epv_;

	template <class Base>
	static Bridge <Interface>* _query_interface (Base& base, const Char* id)
	{
		if (RepositoryId::compatible (Bridge < ::Nirvana::Core::Executor>::_primary_interface (), id))
			return &S::template _narrow < ::Nirvana::Core::Executor> (base);
		else
			return false;
	}

protected:
	static void _execute (Bridge < ::Nirvana::Core::Executor>* _b, ::Nirvana::DeadlineTime deadline, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).execute (deadline);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
	}
};

template <class S>
const Bridge < ::Nirvana::Core::Executor>::EPV Skeleton <S, ::Nirvana::Core::Executor>::epv_ = {
	{ // interface
		S::template _duplicate < ::Nirvana::Core::Executor>,
		S::template _release < ::Nirvana::Core::Executor>
	},
	{ // base
		S::template _wide <AbstractBase, ::Nirvana::Core::Executor>
	},
	{ // epv
		S::_execute,
	}
};

// Standard implementation

template <class S>
class Servant <S, ::Nirvana::Core::Executor> :
	public Implementation <S, ::Nirvana::Core::Executor>
{};

// Static implementation

template <class S>
class ServantStatic <S, ::Nirvana::Core::Executor> :
	public ImplementationStatic <S, ::Nirvana::Core::Executor>
{};

// Tied implementation

template <class T>
class ServantTied <T, ::Nirvana::Core::Executor> :
	public ImplementationTied <T, ::Nirvana::Core::Executor>
{
public:
	ServantTied (T* tp, Boolean release) :
		ImplementationTied <T, ::Nirvana::Core::Executor> (tp, release)
	{}
};

}
}

namespace Nirvana {
namespace Core {

class Executor :
	public ::CORBA::Nirvana::ClientInterface <Executor>,
	public ::CORBA::Nirvana::ClientInterface <Executor, ::CORBA::AbstractBase>
{
public:
	typedef Executor_ptr _ptr_type;

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

/// \interface Scheduler

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
			void (*schedule) (Bridge <::Nirvana::Core::Scheduler>*, ::Nirvana::DeadlineTime, Bridge < ::Nirvana::Core::Executor>*, ::Nirvana::DeadlineTime, EnvironmentBridge*);
			void (*back_off) (Bridge <::Nirvana::Core::Scheduler>*, ULong, EnvironmentBridge*);
			void (*core_free) (Bridge <::Nirvana::Core::Scheduler>*, EnvironmentBridge*);
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
};

template <class T>
class Client <T, ::Nirvana::Core::Scheduler> :
	public T
{
public:
	void schedule (::Nirvana::DeadlineTime deadline, ::Nirvana::Core::Executor_ptr executor, ::Nirvana::DeadlineTime old);
	void back_off (ULong);
	void core_free ();
};

template <class T>
void Client <T, ::Nirvana::Core::Scheduler>::schedule (::Nirvana::DeadlineTime deadline, ::Nirvana::Core::Executor_ptr executor, ::Nirvana::DeadlineTime old)
{
	Environment _env;
	Bridge < ::Nirvana::Core::Scheduler>& _b = ClientBase <T, ::Nirvana::Core::Scheduler>::_bridge ();
	(_b._epv ().epv.schedule) (&_b, deadline, executor, old, &_env);
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

template <class T>
void Client <T, ::Nirvana::Core::Scheduler>::core_free ()
{
	Environment _env;
	Bridge < ::Nirvana::Core::Scheduler>& _b = ClientBase <T, ::Nirvana::Core::Scheduler>::_bridge ();
	(_b._epv ().epv.core_free) (&_b, &_env);
	_env.check ();
}

template <class S>
class Skeleton <S, ::Nirvana::Core::Scheduler>
{
public:
	static const typename Bridge < ::Nirvana::Core::Scheduler>::EPV epv_;

	template <class Base>
	static Bridge <Interface>* _query_interface (Base& base, const Char* id)
	{
		if (RepositoryId::compatible (Bridge < ::Nirvana::Core::Scheduler>::_primary_interface (), id))
			return &S::template _narrow < ::Nirvana::Core::Scheduler> (base);
		else
			return false;
	}

protected:
	static void _schedule (Bridge < ::Nirvana::Core::Scheduler>* _b, ::Nirvana::DeadlineTime deadline, 
												 Bridge < ::Nirvana::Core::Executor>* executor, ::Nirvana::DeadlineTime old, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).schedule (deadline, executor, old);
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

	static void _core_free (Bridge < ::Nirvana::Core::Scheduler>* _b, EnvironmentBridge* _env)
	{
		try {
			return S::_implementation (_b).core_free ();
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
		S::_back_off,
		S::_core_free
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
	public ImplementationTied <T, ::Nirvana::Core::Scheduler>
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
	public ::CORBA::Nirvana::ClientInterfacePseudo <Scheduler>,
	public ::CORBA::Nirvana::ClientInterfaceBase <Scheduler, ::CORBA::AbstractBase>
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
