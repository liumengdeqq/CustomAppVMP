# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library(
             native-lib
             SHARED
               src/main/cpp/native-lib.cpp
                          src/main/cpp/dalvik/open_memstream.cpp
                          src/main/cpp/dalvik/process_name.cpp
                          src/main/cpp/dalvik/sched_policy.cpp
                          src/main/cpp/dalvik/sockets.cpp
                          src/main/cpp/dalvik/DexCatch.cpp
                          src/main/cpp/dalvik/DexClass.cpp
                          src/main/cpp/dalvik/DexDebugInfo.cpp
                          src/main/cpp/dalvik/DexFile.cpp
                          src/main/cpp/dalvik/DexInlines.cpp
                          src/main/cpp/dalvik/DexOptData.cpp
                          src/main/cpp/dalvik/DexOpcodes.cpp
                          src/main/cpp/dalvik/DexProto.cpp
                          src/main/cpp/dalvik/DexUtf.cpp
                          src/main/cpp/dalvik/InstrUtils.cpp
                          src/main/cpp/dalvik/Leb128.cpp
                          src/main/cpp/dalvik/OptInvocation.cpp
                          src/main/cpp/dalvik/sha1.cpp
                          src/main/cpp/dalvik/SysUtil.cpp
                          src/main/cpp/dalvik/ZipArchive.cpp
                          src/main/cpp/dalvik/AllocTracker.cpp
                          src/main/cpp/dalvik/Atomic.cpp
                          src/main/cpp/dalvik/AtomicCache.cpp
                          src/main/cpp/dalvik/BitVector.cpp
                          src/main/cpp/dalvik/CheckJni.cpp
                          src/main/cpp/dalvik/Ddm.cpp
                          src/main/cpp/dalvik/Debugger.cpp
                          src/main/cpp/dalvik/DvmDex.cpp
                          src/main/cpp/dalvik/Exception.cpp
                          src/main/cpp/dalvik/Hash.cpp
                          src/main/cpp/dalvik/IndirectRefTable.cpp
                          src/main/cpp/dalvik/InitRefs.cpp
                          src/main/cpp/dalvik/InlineNative.cpp
                          src/main/cpp/dalvik/Inlines.cpp
                          src/main/cpp/dalvik/Intern.cpp
                          src/main/cpp/dalvik/Jni.cpp
                          src/main/cpp/dalvik/JarFile.cpp
                          src/main/cpp/dalvik/LinearAlloc.cpp
                          src/main/cpp/dalvik/Misc.cpp
                          src/main/cpp/dalvik/Native.cpp
                          src/main/cpp/dalvik/PointerSet.cpp
                          src/main/cpp/dalvik/Profile.cpp
                          src/main/cpp/dalvik/RawDexFile.cpp
                          src/main/cpp/dalvik/ReferenceTable.cpp
                          src/main/cpp/dalvik/SignalCatcher.cpp
                          src/main/cpp/dalvik/StdioConverter.cpp
                          src/main/cpp/dalvik/Sync.cpp
                          src/main/cpp/dalvik/Thread.cpp
                          src/main/cpp/dalvik/UtfString.cpp
                          src/main/cpp/dalvik/Alloc.cpp
                          src/main/cpp/dalvik/CardTable.cpp
                          src/main/cpp/dalvik/HeapBitmap.cpp
                          src/main/cpp/dalvik/HeapDebug.cpp
                          src/main/cpp/dalvik/Heap.cpp
                          src/main/cpp/dalvik/DdmHeap.cpp
                          src/main/cpp/dalvik/Verify.cpp
                          src/main/cpp/dalvik/Visit.cpp
                             src/main/cpp/dalvik/CodeVerify.cpp
                             src/main/cpp/dalvik/DexPrepare.cpp
                             src/main/cpp/dalvik/DexVerify.cpp
                             src/main/cpp/dalvik/Liveness.cpp
                             src/main/cpp/dalvik/Optimize.cpp
                             src/main/cpp/dalvik/RegisterMap.cpp
                             src/main/cpp/dalvik/VerifySubs.cpp
                             src/main/cpp/dalvik/VfyBasicBlock.cpp
                             src/main/cpp/dalvik/Hprof.cpp
                             src/main/cpp/dalvik/HprofClass.cpp
                             src/main/cpp/dalvik/HprofHeap.cpp
                             src/main/cpp/dalvik/HprofOutput.cpp
                             src/main/cpp/dalvik/HprofString.cpp
                             src/main/cpp/dalvik/Interp.cpp
                             src/main/cpp/dalvik/Stack.cpp
                             src/main/cpp/dalvik/ExpandBuf.cpp
                             src/main/cpp/dalvik/JdwpAdb.cpp
                             src/main/cpp/dalvik/JdwpConstants.cpp
                             src/main/cpp/dalvik/JdwpEvent.cpp
                             src/main/cpp/dalvik/JdwpHandler.cpp
                             src/main/cpp/dalvik/JdwpMain.cpp
                             src/main/cpp/dalvik/JdwpSocket.cpp
                             src/main/cpp/dalvik/BitConvert.cpp
                             src/main/cpp/dalvik/InterpC.cpp
                             src/main/cpp/dalvik/Mterp.cpp
                             src/main/cpp/dalvik/InternalNative.cpp
                             src/main/cpp/dalvik/dalvik_bytecode_OpcodeInfo.cpp
                             src/main/cpp/dalvik/dalvik_system_DexFile.cpp
                             src/main/cpp/dalvik/dalvik_system_VMDebug.cpp
                             src/main/cpp/dalvik/dalvik_system_VMRuntime.cpp
                             src/main/cpp/dalvik/dalvik_system_VMStack.cpp
                             src/main/cpp/dalvik/java_lang_Class.cpp
                             src/main/cpp/dalvik/java_lang_Double.cpp
                             src/main/cpp/dalvik/java_lang_Float.cpp
                             src/main/cpp/dalvik/java_lang_Math.cpp
                             src/main/cpp/dalvik/java_lang_Object.cpp
                             src/main/cpp/dalvik/java_lang_Runtime.cpp
                             src/main/cpp/dalvik/java_lang_String.cpp
                             src/main/cpp/dalvik/java_lang_System.cpp
                             src/main/cpp/dalvik/java_lang_Throwable.cpp
                             src/main/cpp/dalvik/java_lang_VMClassLoader.cpp
                             src/main/cpp/dalvik/java_lang_VMThread.cpp
                             src/main/cpp/dalvik/java_lang_reflect_AccessibleObject.cpp
                             src/main/cpp/dalvik/java_lang_reflect_Array.cpp
                             src/main/cpp/dalvik/java_lang_reflect_Constructor.cpp
                             src/main/cpp/dalvik/java_lang_reflect_Field.cpp
                             src/main/cpp/dalvik/java_lang_reflect_Method.cpp
                             src/main/cpp/dalvik/java_lang_reflect_Proxy.cpp
                             src/main/cpp/dalvik/java_util_concurrent_atomic_AtomicLong.cpp
                             src/main/cpp/dalvik/org_apache_harmony_dalvik_NativeTestTarget.cpp
                             src/main/cpp/dalvik/org_apache_harmony_dalvik_ddmc_DdmServer.cpp
                             src/main/cpp/dalvik/org_apache_harmony_dalvik_ddmc_DdmVmInternal.cpp
                             src/main/cpp/dalvik/sun_misc_Unsafe.cpp
                             src/main/cpp/dalvik/AccessCheck.cpp
                             src/main/cpp/dalvik/Array.cpp
                             src/main/cpp/dalvik/Class.cpp
                             src/main/cpp/dalvik/Object.cpp
                             src/main/cpp/dalvik/Resolve.cpp
                             src/main/cpp/dalvik/TypeCheck.cpp
                             src/main/cpp/dalvik/android.cpp
                             src/main/cpp/dalvik/linux.cpp
                             src/main/cpp/dalvik/Annotation.cpp
                             src/main/cpp/dalvik/Proxy.cpp
                             src/main/cpp/dalvik/Reflect.cpp)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native-lib
                       z
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )