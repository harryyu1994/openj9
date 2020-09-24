/*******************************************************************************
 * Copyright (c) 2000, 2020 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#ifndef J9_CPU_INCL
#define J9_CPU_INCL

/*
 * The following #define and typedef must appear before any #includes in this file
 */
#ifndef J9_CPU_CONNECTOR
#define J9_CPU_CONNECTOR
namespace J9 { class CPU; }
namespace J9 { typedef CPU CPUConnector; }
#endif

#include "env/OMRCPU.hpp"
#include "j9port.h"
#include "infra/Assert.hpp"                         // for TR_ASSERT

namespace J9
{
class OMR_EXTENSIBLE CPU : public OMR::CPUConnector
   {
protected:

   CPU() : OMR::CPUConnector() {}
   CPU(const OMRProcessorDesc& processorDescription) : OMR::CPUConnector(processorDescription) {}

   static OMRProcessorDesc _featureMasks;
   static bool _shouldUseFeatureMasks; 
   static void applyFeatureMasks(OMRProcessorDesc& processorDescription, const uint32_t* enabledFeatures, size_t size); 

public:

   const char *getProcessorVendorId() { TR_ASSERT(false, "Vendor ID not defined for this platform!"); return NULL; }
   uint32_t getProcessorSignature() { TR_ASSERT(false, "Processor Signature not defined for this platform!"); return 0; }

   OMRProcessorDesc getProcessorDescription();
   bool supportsFeature(uint32_t feature);
   };
}


#endif
