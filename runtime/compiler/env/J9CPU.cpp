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

#pragma csect(CODE,"J9J9CPU#C")
#pragma csect(STATIC,"J9J9CPU#S")
#pragma csect(TEST,"J9J9CPU#T")

#include "control/CompilationRuntime.hpp"
#include "env/CompilerEnv.hpp"
#include "env/CPU.hpp"
#include "env/VMJ9.h"

OMRProcessorDesc J9::CPU::_supportedFeatureMasks = {OMR_PROCESSOR_UNDEFINED, OMR_PROCESSOR_UNDEFINED, {}};
bool J9::CPU::_isSupportedFeatureMasksEnabled = false;

OMRProcessorDesc
J9::CPU::getProcessorDescription()
   {
#if defined(J9VM_OPT_JITSERVER)
   if (auto stream = TR::CompilationInfo::getStream())
      {
      auto *vmInfo = TR::compInfoPT->getClientData()->getOrCacheVMInfo(stream);
      return vmInfo->_processorDescription;
      }
#endif /* defined(J9VM_OPT_JITSERVER) */
   return _processorDescription;
   }

void
J9::CPU::enableFeatureMasks()
   {
   // Assume all features will be utilized by default
   memset(_supportedFeatureMasks.features, ~0, OMRPORT_SYSINFO_FEATURES_SIZE*sizeof(uint32_t));
   _isSupportedFeatureMasksEnabled = true;
   }

TR::CPU
J9::CPU::customize(OMRProcessorDesc processorDescription)
   {
   if (_isSupportedFeatureMasksEnabled)
      {
      // mask out any cpu features that the compiler doesn't care about
      for (size_t i = 0; i < OMRPORT_SYSINFO_FEATURES_SIZE; i++)
         {
         processorDescription.features[i] &= _supportedFeatureMasks.features[i];
         }
      }
   return TR::CPU(processorDescription);
   }

TR::CPU
J9::CPU::detect(OMRPortLibrary * const omrPortLib)
   {   
   OMRPORT_ACCESS_FROM_OMRPORT(omrPortLib);
   OMRProcessorDesc processorDescription;
   omrsysinfo_get_processor_description(&processorDescription);

   for (int k = 0; k < 5; k++)
      {
      printf ("J9::CPU::detect!! before customize: processorDescription %d   0b", k);
      for (int i = 31; i >= 0; i--)
         {
         if ((1 << i) & processorDescription.features[k])
            printf ("1");
         else
            printf ("0");
         }

      for (int i = 31; i >= 0; i--)
         {
         if ((1 << i) & processorDescription.features[k])
            printf ("  %s  ", omrsysinfo_get_processor_feature_name(k*32 + i));
         }
      printf ("\n");
      }

   PORT_ACCESS_FROM_PORT(TR::Compiler->portLib);
   J9ProcessorDesc processorDescriptionOld;
   j9sysinfo_get_processor_description(&processorDescriptionOld);

   for (int k = 0; k < 5; k++)
      {
      printf ("J9::CPU::detect!! before customize old version: processorDescriptionOld %d   0b", k);
      for (int i = 31; i >= 0; i--)
         {
         if ((1 << i) & processorDescriptionOld.features[k])
            printf ("1");
         else
            printf ("0");
         }

      for (int i = 31; i >= 0; i--)
         {
         if ((1 << i) & processorDescriptionOld.features[k])
            printf ("  %s  ", omrsysinfo_get_processor_feature_name(k*32 + i));
         }
      printf ("\n");
      }

   TR::CPU::enableFeatureMasks();
   TR::CPU cpu = TR::CPU::customize(processorDescription);

   for (int k = 0; k < 5; k++)
      {
      printf ("J9::CPU::detect!! after customize: feature Masks %d   0b", k);
      for (int i = 31; i >= 0; i--)
         {
         if ((1 << i) & _supportedFeatureMasks.features[k])
            printf ("1");
         else
            printf ("0");
         }

      for (int i = 31; i >= 0; i--)
         {
         if ((1 << i) & _supportedFeatureMasks.features[k])
            printf ("  %s  ", omrsysinfo_get_processor_feature_name(k*32 + i));
         }

      printf ("\n");
      }

   OMRProcessorDesc processorDescription2 = cpu.getProcessorDescription();
   for (int k = 0; k < 5; k++)
      {
      printf ("J9::CPU::detect!! After customize: processorDescription %d   0b", k);
      for (int i = 31; i >= 0; i--)
         {
         if ((1 << i) & processorDescription2.features[k])
            printf ("1");
         else
            printf ("0");
         }

      for (int i = 31; i >= 0; i--)
         {
         if ((1 << i) & processorDescription2.features[k])
            printf ("  %s  ", omrsysinfo_get_processor_feature_name(k*32 + i));
         }

      printf ("\n");
      }
   return cpu;
   }

bool
J9::CPU::supportsFeature(uint32_t feature)
   {
   OMRPORT_ACCESS_FROM_OMRPORT(TR::Compiler->omrPortLib);

   static bool disableCPUDetectionTest = feGetEnv("TR_DisableCPUDetectionTest");
   if (!disableCPUDetectionTest && _isSupportedFeatureMasksEnabled)
      {
      TR_ASSERT_FATAL(TRUE == omrsysinfo_processor_has_feature(&_supportedFeatureMasks, feature), "New processor feature usage detected, please add feature %d to _supportedFeatureMasks via TR::CPU::enableFeatureMasks()\n", feature);
      }

   return TRUE == omrsysinfo_processor_has_feature(&_processorDescription, feature);
   }
