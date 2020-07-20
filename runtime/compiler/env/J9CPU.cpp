/*******************************************************************************
 * Copyright (c) 2000, 2018 IBM Corp. and others
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

#pragma csect(CODE,"J9J9CPU#C")
#pragma csect(STATIC,"J9J9CPU#S")
#pragma csect(TEST,"J9J9CPU#T")

#include "env/CompilerEnv.hpp"
#include "env/CPU.hpp"
#include "env/VMJ9.h"
#include "j9.h"
#include "j9port.h"


J9ProcessorDesc*
J9::CPU::TO_PORTLIB_getJ9ProcessorDesc()
   {
   static int processorTypeInitialized = 0;
   static J9ProcessorDesc  processorDesc;

   if(processorTypeInitialized == 0)
      {
      PORT_ACCESS_FROM_PORT(TR::Compiler->portLib);

      //Just make sure we initialize the J9ProcessorDesc at least once.
      j9sysinfo_get_processor_description(&processorDesc);

      processorTypeInitialized = 1;
      }
   return &processorDesc;
   }

void
J9::CPU::printCPU()
   {
   OMRPORT_ACCESS_FROM_OMRPORT(TR::Compiler->omrPortLib);
   int length = 0;
   char buff[500];
   int buffSize = 500;
   for (size_t i = 0; i < OMRPORT_SYSINFO_FEATURES_SIZE; i++)
      {
      for (int j = 0; j < 32; j++) 
         {
         if (_processorDescription.features[i] & (1<<j))
            {
            if (length > 0)
               {
               strncat(buff, " ", buffSize);
               length += 1;
               }
            uint32_t feature = i * 32 + j;
            const char * feature_name = omrsysinfo_get_processor_feature_name(feature);
            strncat(buff, feature_name, buffSize);
            length += strlen(feature_name);
            }
         }
      }
   printf ("%s\n", buff);
   return;
   }
