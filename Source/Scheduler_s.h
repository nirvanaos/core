#ifndef NIRVANA_CORE_SCHEDULER_S_H_
#define NIRVANA_CORE_SCHEDULER_S_H_

#include "Scheduler_c.h"
#include <CORBA/Servant.h>

namespace CORBA {
namespace Nirvana {

// Executor skeleton

template <class S>
class Skeleton <S, ::Nirvana::Core::Executor>
{
public:
	static const typename Bridge < ::Nirvana::Core::Executor>::EPV epv_;

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
		Bridge < ::Nirvana::Core::Executor>::interface_id_,
		S::template __duplicate < ::Nirvana::Core::Executor>,
		S::template __release < ::Nirvana::Core::Executor>
	},
	{ // epv
		S::_execute
	}
};

// Standard implementation

template <class S>
class Servant <S, ::Nirvana::Core::Executor> :
	public ImplementationPseudo <S, ::Nirvana::Core::Executor>
{};

// Static implementation

template <class S>
class ServantStatic <S, ::Nirvana::Core::Executor> :
	public ImplementationStaticPseudo <S, ::Nirvana::Core::Executor>
{};

// Scheduler skeleton

template <class S>
class Skeleton <S, ::Nirvana::Core::Scheduler>
{
public:
	static const typename Bridge < ::Nirvana::Core::Scheduler>::EPV epv_;

protected:
	static void _schedule (Bridge < ::Nirvana::Core::Scheduler>* _b, ::Nirvana::DeadlineTime deadline,
												 BridgeMarshal < ::Nirvana::Core::Executor>* executor, ::Nirvana::DeadlineTime old, EnvironmentBridge* _env)
	{
		try {
			S::_implementation (_b).schedule (deadline, executor, old);
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
	}

	static void _core_free (Bridge < ::Nirvana::Core::Scheduler>* _b, EnvironmentBridge* _env)
	{
		try {
			S::_implementation (_b).core_free ();
		} catch (const Exception& e) {
			_env->set_exception (e);
		} catch (...) {
			_env->set_unknown_exception ();
		}
	}

	static void _shutdown (Bridge < ::Nirvana::Core::Scheduler>* _b, EnvironmentBridge* _env)
	{
		try {
			S::_implementation (_b).shutdown ();
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
		Bridge < ::Nirvana::Core::Scheduler>::interface_id_,
		S::template __duplicate < ::Nirvana::Core::Scheduler>,
		S::template __release < ::Nirvana::Core::Scheduler>
	},
	{ // epv
		S::_schedule,
		S::_core_free,
		S::_shutdown
	}
};

// Standard implementation

template <class S>
class Servant <S, ::Nirvana::Core::Scheduler> :
	public ImplementationPseudo <S, ::Nirvana::Core::Scheduler>
{};

// Static implementation

template <class S>
class ServantStatic <S, ::Nirvana::Core::Scheduler> :
	public ImplementationStaticPseudo <S, ::Nirvana::Core::Scheduler>
{};

}
}

#endif
