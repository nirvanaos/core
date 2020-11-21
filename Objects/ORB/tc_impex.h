#ifndef NIRVANA_ORB_CORE_TC_IMPEX_H_
#define NIRVANA_ORB_CORE_TC_IMPEX_H_

#include <Nirvana/OLF.h>

#define TC_IMPEX_EX(t, id, ...) namespace CORBA {\
extern const ::Nirvana::ImportInterfaceT <TypeCode> _tc_##t = { ::Nirvana::OLF_IMPORT_INTERFACE, nullptr, nullptr, STATIC_BRIDGE (TypeCode, __VA_ARGS__) };\
} NIRVANA_EXPORT (_exp_CORBA_tc_##t, id, CORBA::TypeCode, __VA_ARGS__);

// Import and export for type code
#define TC_IMPEX(t, ...) TC_IMPEX_EX (t, "CORBA/_tc_" #t, __VA_ARGS__)
#define TC_IMPEX_BY_ID(T) TC_IMPEX_EX (T, CORBA::TC_##T::RepositoryType::repository_id_, CORBA::TC_##T)

// Import and export for interface exception
#define INTERFACE_EXC_IMPEX(ns, I, E) namespace ns {\
const ::Nirvana::ImportInterfaceT <CORBA::TypeCode> I::_tc_##E = { ::Nirvana::OLF_IMPORT_INTERFACE, 0, 0, STATIC_BRIDGE (CORBA::TypeCode, CORBA::Nirvana::TypeCodeException <ns::I::E>) };\
} NIRVANA_EXPORT (_exp_##ns##_##I##_##E, ns::I::E::repository_id_, CORBA::TypeCode, CORBA::Nirvana::TypeCodeException <ns::I::E>);

#endif
