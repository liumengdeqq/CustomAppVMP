//
// Created by liu meng on 2018/8/20.
//
#include "Resolve.h"
#include "DexFile.h"
#include "UtfString.h"
StringObject* dvmResolveString(const ClassObject* referrer, u4 stringIdx)
{
    DvmDex* pDvmDex = referrer->pDvmDex;
    StringObject* strObj;
    StringObject* internStrObj;
    const char* utf8;
    u4 utf16Size;

    LOGVV("+++ resolving string, referrer is %s", referrer->descriptor);

    /*
     * Create a UTF-16 version so we can trivially compare it to what's
     * already interned.
     */
    utf8 = dexStringAndSizeById(pDvmDex->pDexFile, stringIdx, &utf16Size);
    strObj = dvmCreateStringFromCstrAndLength(utf8, utf16Size);
    if (strObj == NULL) {
        /* ran out of space in GC heap? */
        assert(dvmCheckException(dvmThreadSelf()));
        goto bail;
    }

    /*
     * Add it to the intern list.  The return value is the one in the
     * intern list, which (due to race conditions) may or may not be
     * the one we just created.  The intern list is synchronized, so
     * there will be only one "live" version.
     *
     * By requesting an immortal interned string, we guarantee that
     * the returned object will never be collected by the GC.
     *
     * A NULL return here indicates some sort of hashing failure.
     */
    internStrObj = dvmLookupImmortalInternedString(strObj);
    dvmReleaseTrackedAlloc((Object*) strObj, NULL);
    strObj = internStrObj;
    if (strObj == NULL) {
        assert(dvmCheckException(dvmThreadSelf()));
        goto bail;
    }

    /* save a reference so we can go straight to the object next time */
    dvmDexSetResolvedString(pDvmDex, stringIdx, strObj);

    bail:
    return strObj;
}
