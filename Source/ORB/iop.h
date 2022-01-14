/// \file
/// IOP - Interoperability definitions
/*
* Nirvana Core.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#ifndef NIRVANA_ORB_IOP_H_
#define NIRVANA_ORB_IOP_H_
#pragma once

#include <CORBA/CORBA.h>

namespace IOP {

  // Standard Protocol Profile tag values
  
  typedef uint32_t ProfileId;
  const ProfileId TAG_INTERNET_IOP = 0;
  const ProfileId TAG_MULTIPLE_COMPONENTS = 1;
  
  struct TaggedProfile {
    ProfileId tag;
    std::vector <CORBA::Octet> profile_data;
  };
  
  // an Interoperable Object Reference is a sequence of
  // object-specific protocol profiles, plus a type ID.
  
  struct IOR {
    const char* type_id;
    std::vector <TaggedProfile> profiles;
  };
  
  // Standard way of representing multicomponent profiles.
  // This would be encapsulated in a TaggedProfile.
  
  typedef unsigned long ComponentId;
  struct TaggedComponent {
    ComponentId tag;
    std::vector <CORBA::Octet> component_data;
  };
  typedef std::vector <TaggedComponent> MultipleComponentProfile;

  // Standard IOR components
  
  const ComponentId TAG_ORB_TYPE = 0;
  const ComponentId TAG_CODE_SETS = 1;
  const ComponentId TAG_POLICIES = 2;
  const ComponentId TAG_ALTERNATE_IIOP_ADDRESS = 3;
  const ComponentId TAG_ASSOCIATION_OPTIONS = 13;
  const ComponentId TAG_SEC_NAME = 14;
  const ComponentId TAG_SPKM_1_SEC_MECH = 15;
  const ComponentId TAG_SPKM_2_SEC_MECH = 16;
  const ComponentId TAG_KerberosV5_SEC_MECH = 17;
  const ComponentId TAG_CSI_ECMA_Secret_SEC_MECH = 18;
  const ComponentId TAG_CSI_ECMA_Hybrid_SEC_MECH = 19;
  const ComponentId TAG_SSL_SEC_TRANS = 20;
  const ComponentId TAG_CSI_ECMA_Public_SEC_MECH = 21;
  const ComponentId TAG_GENERIC_SEC_MECH = 22;
  const ComponentId TAG_JAVA_CODEBASE = 25;

  // Object services

  typedef unsigned long ServiceId;
  struct ServiceContext {
    ServiceId context_id;
    std::vector <CORBA::Octet> context_data;
  };

  typedef std::vector <ServiceContext> ServiceContextList;
  const ServiceId TransactionService = 0;
  const ServiceId CodeSets = 1;
  const ServiceId ChainBypassCheck = 2;
  const ServiceId ChainBypassInfo = 3;
  const ServiceId LogicalThreadId = 4;
  const ServiceId BI_DIR_IIOP = 5;
  const ServiceId SendingContextRunTime = 6;
  const ServiceId INVOCATION_POLICIES = 7;
  const ServiceId FORWARDED_IDENTITY = 8;
  const ServiceId UnknownExceptionInfo = 9;

} // namespace IOP

#endif
