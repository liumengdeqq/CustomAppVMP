/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * All-inclusive internal header file.  Include this to get everything useful.
 */
#ifndef DALVIK_DALVIK_H_
#define DALVIK_DALVIK_H_

#include "Common.h"
#include "Inlines.h"
#include "Misc.h"
#include "Bits.h"
#include "BitVector.h"
#include "SysUtil.h"
#include "DexDebugInfo.h"
#include "DexFile.h"
#include "DexProto.h"
#include "DexUtf.h"
#include "ZipArchive.h"
#include "DvmDex.h"
#include "RawDexFile.h"
#include "Sync.h"
#include "Object.h"
#include "Native.h"
#include "InternalNative.h"

#include "DalvikVersion.h"
#include "Debugger.h"
#include "Profile.h"
#include "UtfString.h"
#include "Intern.h"
#include "ReferenceTable.h"
#include "IndirectRefTable.h"
#include "AtomicCache.h"
#include "Thread.h"
#include "Ddm.h"
#include "Hash.h"
#include "Stack.h"
#include "Class.h"
#include "Resolve.h"
#include "Array.h"
#include "Exception.h"
#include "Alloc.h"
#include "CardTable.h"
#include "HeapDebug.h"
#include "WriteBarrier.h"
#include "AccessCheck.h"
#include "JarFile.h"
#include "Jdwp.h"
#include "SignalCatcher.h"
#include "StdioConverter.h"
#include "JniInternal.h"
#include "LinearAlloc.h"
#include "DexVerify.h"
#include "DexPrepare.h"
#include "RegisterMap.h"
#include "DexOpcodes.h"
#include "InstrUtils.h"
#include "AllocTracker.h"
#include "PointerSet.h"
#include <unistd.h>
#if defined(WITH_JIT)
#include "compiler/Compiler.h"
#endif
#include "Globals.h"
#include "Reflect.h"
#include "TypeCheck.h"
#include "Atomic.h"
#include "Interp.h"
#include "InlineNative.h"
#include "ObjectInlines.h"
#include "customlog.h"
#include "atomic-inline.h"
#include "FindInterface.h"
#endif  // DALVIK_DALVIK_H_
