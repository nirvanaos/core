#ifndef NIRVANA_TESTORB_OBJECTIMPL_H_
#define NIRVANA_TESTORB_OBJECTIMPL_H_

#include <CORBA/Object_s.h>
#include <CORBA/Implementation.h>

namespace Nirvana {
namespace Core {

using namespace ::CORBA;
using namespace ::CORBA::Nirvana;

class ObjectBase
{
public:
	ObjectBase (AbstractBase_ptr servant) :
		servant_ (servant)
	{}

	operator Bridge <AbstractBase>& () const
	{
		return *servant_;
	}

	AbstractBase_ptr abstract_base () const
	{
		return servant_;
	}

	Interface_ptr primary_interface () const
	{
		return servant_->_query_interface (nullptr);
	}

	const Char* primary_interface_id () const
	{
		return primary_interface ()->_epv ().interface_id;
	}

	// Object operations

	static BridgeMarshal <ImplementationDef>* __get_implementation (Bridge <Object>* obj, EnvironmentBridge* env)
	{
		return nullptr; // We dont implement it
	}

	InterfaceDef_ptr _get_interface () const
	{
		return InterfaceDef_ptr::nil ();	// TODO: Implement
	}

	Boolean _is_a (const Char* type_id) const;

	static Boolean __non_existent (Bridge <Object>*, EnvironmentBridge*)
	{
		return FALSE;
	}

	static Boolean __is_equivalent (Bridge <Object>* obj, BridgeMarshal <Object>* other, EnvironmentBridge*)
	{
		return obj == other;
	}

	ULong _hash (ULong maximum) const
	{
		return 0; // TODO: Implement.
	}
	// TODO: More Object operations shall be here...
	
private:
	AbstractBase_ptr servant_;
};

template <class S>
class ObjectImpl :
	public ObjectBase,
	public InterfaceImplBase <S, Object>,
	public Skeleton <S, AbstractBase> // Derive only for _wide() implementation.
{
public:
	ObjectImpl (AbstractBase_ptr servant) :
		ObjectBase (servant)
	{}

	static BridgeMarshal <ImplementationDef>* __get_implementation (Bridge <Object>* obj, EnvironmentBridge* env)
	{
		return ObjectBase::__get_implementation (obj, env);
	}

	static Boolean __non_existent (Bridge <Object>* obj, EnvironmentBridge* env)
	{
		return ObjectBase::__non_existent (obj, env);
	}

	static Boolean __is_equivalent (Bridge <Object>* obj, BridgeMarshal <Object>* other, EnvironmentBridge* env)
	{
		return ObjectBase::__is_equivalent (obj, other, env);
	}
};

}
}

#endif
