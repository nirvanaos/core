// This file was generated from "ORB.idl"
// Nirvana IDL compiler version 1.0
#include <CORBA/Proxy/Proxy.h>
#include "../Source/ORB/IDL/ORB_s.h"

extern "C" NIRVANA_OLF_SECTION const Nirvana::ExportInterface _exp_CORBA_ORB_ObjectId;

namespace CORBA {
namespace Internal {

template <>
class TypeCodeTypeDef <&::_exp_CORBA_ORB_ObjectId> :
	public TypeCodeTypeDefImpl <&::_exp_CORBA_ORB_ObjectId, String>
{};

template <>
const Char TypeCodeName <TypeCodeTypeDef <&::_exp_CORBA_ORB_ObjectId> >::name_ [] = "ObjectId";

}
}
NIRVANA_EXPORT (_exp_CORBA_ORB_ObjectId, "IDL:CORBA/ORB/ObjectId:1.0", CORBA::TypeCode, CORBA::Internal::TypeCodeTypeDef <&_exp_CORBA_ORB_ObjectId>)

extern "C" NIRVANA_OLF_SECTION const Nirvana::ExportInterface _exp_CORBA_ORB_ObjectIdList;

namespace CORBA {
namespace Internal {

template <>
class TypeCodeTypeDef <&::_exp_CORBA_ORB_ObjectIdList> :
	public TypeCodeTypeDefImpl <&::_exp_CORBA_ORB_ObjectIdList, Sequence <Definitions <ORB>::ObjectId> >
{};

template <>
const Char TypeCodeName <TypeCodeTypeDef <&::_exp_CORBA_ORB_ObjectIdList> >::name_ [] = "ObjectIdList";

}
}
NIRVANA_EXPORT (_exp_CORBA_ORB_ObjectIdList, "IDL:CORBA/ORB/ObjectIdList:1.0", CORBA::TypeCode, CORBA::Internal::TypeCodeTypeDef <&_exp_CORBA_ORB_ObjectIdList>)
NIRVANA_EXPORT (_exp_CORBA_ORB_InvalidName, CORBA::Internal::RepIdOf <CORBA::Internal::Definitions <CORBA::ORB>::InvalidName>::repository_id_, CORBA::TypeCode, CORBA::Internal::TypeCodeException <CORBA::Internal::Definitions <CORBA::ORB>::InvalidName, false>)
