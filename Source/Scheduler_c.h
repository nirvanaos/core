// The Nirvana project.
// Core scheduler interfaces.

#ifndef NIRVANA_CORE_SCHEDULER_C_H_
#define NIRVANA_CORE_SCHEDULER_C_H_

#include <CORBA/AbstractBase_c.h>
#include <Nirvana/Nirvana.h>

//! \interface Executor

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
	public BridgeMarshal < ::Nirvana::Core::Executor>
{
public:
	struct EPV
	{
		Bridge <Interface>::EPV interface;

		struct
		{
			BASE_STRUCT_ENTRY (CORBA::AbstractBase, CORBA_AbstractBase)
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

	static const Char interface_id_ [];

protected:
	Bridge (const EPV& epv) :
		BridgeMarshal < ::Nirvana::Core::Executor> (epv.interface)
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
	Bridge < ::Nirvana::Core::Executor>& _b (T::_get_bridge (_env));
	(_b._epv ().epv.execute) (&_b, deadline, &_env);
	_env.check ();
}

}
}

namespace Nirvana {
namespace Core {

class Executor : public ::CORBA::Nirvana::ClientInterface <Executor, ::CORBA::AbstractBase>
{};

}
}

//! \interface Scheduler

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
	public BridgeMarshal < ::Nirvana::Core::Scheduler>
{
public:
	struct EPV
	{
		Bridge <Interface>::EPV interface;

		struct
		{
			BASE_STRUCT_ENTRY (CORBA::AbstractBase, CORBA_AbstractBase)
		}
		base;

		struct
		{
			void (*schedule) (Bridge <::Nirvana::Core::Scheduler>*, ::Nirvana::DeadlineTime, BridgeMarshal < ::Nirvana::Core::Executor>*, ::Nirvana::DeadlineTime, EnvironmentBridge*);
			void (*core_free) (Bridge <::Nirvana::Core::Scheduler>*, EnvironmentBridge*);
		}
		epv;
	};

	const EPV& _epv () const
	{
		return (EPV&)Bridge <Interface>::_epv ();
	}

	static const Char interface_id_ [];

protected:
	Bridge (const EPV& epv) :
		BridgeMarshal < ::Nirvana::Core::Scheduler> (epv.interface)
	{}
};

template <class T>
class Client <T, ::Nirvana::Core::Scheduler> :
	public T
{
public:
	void schedule (::Nirvana::DeadlineTime deadline, ::Nirvana::Core::Executor_ptr executor, ::Nirvana::DeadlineTime old);
	void core_free ();
};

template <class T>
void Client <T, ::Nirvana::Core::Scheduler>::schedule (::Nirvana::DeadlineTime deadline, ::Nirvana::Core::Executor_ptr executor, ::Nirvana::DeadlineTime old)
{
	Environment _env;
	Bridge < ::Nirvana::Core::Scheduler>& _b (T::_get_bridge (_env));
	(_b._epv ().epv.schedule) (&_b, deadline, executor, old, &_env);
	_env.check ();
}

template <class T>
void Client <T, ::Nirvana::Core::Scheduler>::core_free ()
{
	Environment _env;
	Bridge < ::Nirvana::Core::Scheduler>& _b (T::_get_bridge (_env));
	(_b._epv ().epv.core_free) (&_b, &_env);
	_env.check ();
}

}
}

namespace Nirvana {
namespace Core {

class Scheduler : public ::CORBA::Nirvana::ClientInterface <Scheduler, ::CORBA::AbstractBase>
{};

}
}

#endif
