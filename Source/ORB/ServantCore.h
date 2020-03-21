#ifndef NIRVANA_TESTORB_SERVANTCORE_H_
#define NIRVANA_TESTORB_SERVANTCORE_H_

#include <CORBA/ServantBase_s.h>
#include "ObjectCore.h"

namespace Nirvana {
namespace Core {

class ServantCore :
	public ::CORBA::Nirvana::ImplementationPseudo <ServantCore, ::CORBA::Nirvana::ServantBase>,
	public ::CORBA::Nirvana::LifeCycleNoCopy <ServantCore>
{
public:
	ServantCore (::CORBA::Nirvana::ServantBase_ptr servant) :
		object_ (servant)
	{}

	operator ::CORBA::Nirvana::Bridge < ::CORBA::Object>& ()
	{
		return object_;
	}

	operator ::CORBA::Nirvana::Bridge <AbstractBase>& ()
	{
		return object_;
	}

	PortableServer::POA_ptr _default_POA () const;

	InterfaceDef_ptr _get_interface () const
	{
		return static_cast <const ObjectBase&> (object_)._get_interface ();
	}

	Boolean _is_a (const Char* type_id)
	{
		return static_cast <const ObjectBase&> (object_)._is_a (type_id);
	}

	Boolean _non_existent () const
	{
		return !object_.is_active_;
	}

private:
	ObjectCore object_;
};

}
}

#endif
