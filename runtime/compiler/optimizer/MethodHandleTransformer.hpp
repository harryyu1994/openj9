/*******************************************************************************
 * Copyright (c) 2021, 2021 IBM Corp. and others
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

#ifndef METHODHANDLETRANSFORMER_INCL
#define METHODHANDLETRANSFORMER_INCL

#include <set>
#include <vector>
#include "il/ParameterSymbol.hpp"
#include "compile/Compilation.hpp"
#include "infra/List.hpp"
#include "optimizer/Optimization.hpp"
#include "optimizer/OptimizationManager.hpp"
#include "env/KnownObjectTable.hpp"

/*
 * This transformer is used to transform OpenJDK MethodHandle related methods,
 * to make sure those methods are optimized for performance.
 *
 * Most of the transformation require knowledge of object-type node. Sources of
 * object info include the following:
 *  1. Prex arg info if under inlining
 *  2. Nodes with known object index
 *  3. Nodes whose value can be compile-time inferred and is a known object
 *
 * We'd like the object info be avaliable at places where we want to do transformation,
 * however, the objects will be stored into autos and autos will be used instead of
 * nodes with known object index. Thus we need to track the object info while walking
 * the trees.
 *
 * Data flow analysis is expensive and we can't afford it especially in ilgen. Most of
 * the methods we want to optimize should have only one store to an auto, i.e. most of
 * the autos are not shared. So we can track the object info in autos with a reverse
 * post-order traversal of CFG. Update object info while walking trees of a block, and
 * propagate it to the block's successor. When visiting a block, if there exist a
 * predecessor unvisited, simply clear the object info inherited from the successors.
 *
 * This opt will also try to discover as many known objects as possible.
 */

class TR_MethodHandleTransformer : public TR::Optimization
   {
   public:
   TR_MethodHandleTransformer(TR::OptimizationManager *manager)
      : TR::Optimization(manager),
      _numLocals(0),
      _currentObjectInfo(NULL)
      {}

   static TR::Optimization *create(TR::OptimizationManager *manager)
      {
      return new (manager->allocator()) TR_MethodHandleTransformer(manager);
      }

   virtual int32_t perform();
   virtual const char * optDetailString() const throw();

   // LocalObjectInfo is a super set of TR::KnownObjectTable::Index, with -2
   // added to represent UNUSED status in a block
   typedef int32_t LocalObjectInfo;
   static const LocalObjectInfo UNUSED = -2;
   static const LocalObjectInfo UNKNOWN = TR::KnownObjectTable::UNKNOWN;

   typedef TR::typed_allocator<LocalObjectInfo, TR::Region &> ObjectInfoAllocator;
   typedef std::vector<LocalObjectInfo, ObjectInfoAllocator> ObjectInfo;

   ObjectInfo * processBlock(TR::Block *block, ObjectInfo *objectInfo);
   // Merge ObjectInfo from second into first
   void mergeObjectInfo(ObjectInfo *first, ObjectInfo *second);
   // Assign local index to autos
   void collectLocals(TR_Array<List<TR::SymbolReference>> *autosListArray);

   // Given an address-typed node, try to figure out it's object info
   LocalObjectInfo getObjectInfoOfNode(TR::Node* node);

   // The folowing visit functions will visit different types of node, update object info,
   // and/or do transformations
   void visitIndirectLoad(TR::TreeTop* tt, TR::Node* node);
   void visitStoreToLocalVariable(TR::TreeTop* tt, TR::Node* node);
   void visitCall(TR::TreeTop* tt, TR::Node* node);
   void visitNode(TR::TreeTop* tt, TR::Node* node, TR::NodeChecklist &visitedNodes);

   void process_java_lang_invoke_MethodHandle_invokeBasic(TR::TreeTop* tt, TR::Node* node);
   void process_java_lang_invoke_MethodHandle_linkTo(TR::TreeTop* tt, TR::Node* node);

   private:
   uint32_t _numLocals; // Number of parms, autos and temps
   ObjectInfo * _currentObjectInfo;
   };

#endif
