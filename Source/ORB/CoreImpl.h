/// \file CoreImpl.h
#ifndef NIRVANA_ORB_CORE_COREIMPL_H_
#define NIRVANA_ORB_CORE_COREIMPL_H_

#include "../core.h"
#include <CORBA/Server.h>

namespace CORBA {
namespace Nirvana {
namespace Core {

/// \brief Core implementation of ServantBase and LocalObject.
template <class T, class I, class Proxy>
class CoreImpl :
	public Proxy,
	public LifeCycleNoCopy <T>,
	public ServantTraits <T>,
	public InterfaceImplBase <T, I>
{
public:
	using ServantTraits <T>::_implementation;
	using LifeCycleNoCopy <T>::__duplicate;
	using LifeCycleNoCopy <T>::__release;
	using Skeleton <T, I>::__non_existent;
	using ServantTraits <T>::_wide_object;

	template <class Base, class Derived>
	static Bridge <Base>* _wide (Bridge <Derived>* derived, String_in id, EnvironmentBridge* env)
	{
		return ServantTraits <T>::template _wide <Base, Derived> (derived, id, env);
	}

	template <>
	static Bridge <ReferenceCounter>* _wide <ReferenceCounter, I> (Bridge <I>* derived, String_in id, EnvironmentBridge* env)
	{
		return nullptr; // ReferenceCounter base is not implemented, return nullptr.
	}

	I_ptr <I> _get_ptr ()
	{
		return I_ptr <I> (&static_cast <I&> (static_cast <Bridge <I>&> (*this)));
	}

protected:
	template <class ... Args>
	CoreImpl (Args ... args) :
		Proxy (std::forward <Args> (args)...)
	{}
};

}
}
}

#endif
