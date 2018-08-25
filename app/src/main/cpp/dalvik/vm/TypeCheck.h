//
// Created by liu meng on 2018/8/25.
//
#include "Inlines.h"
#include "object.h"
#include "Globals.h"
#include "Array.h"
#include "AtomicCache.h"
#ifndef CUSTOMAPPVMP_TYPECHECK_H
#define CUSTOMAPPVMP_TYPECHECK_H
#define BOOL_TO_INT(x)  (x)
#define INSTANCEOF_CACHE_SIZE   1024
int dvmImplements(const ClassObject* clazz, const ClassObject* interface)
{
    int i;

    assert(dvmIsInterfaceClass(interface));

    /*
     * All interfaces implemented directly and by our superclass, and
     * recursively all super-interfaces of those interfaces, are listed
     * in "iftable", so we can just do a linear scan through that.
     */
    for (i = 0; i < clazz->iftableCount; i++) {
        if (clazz->iftable[i].clazz == interface)
            return 1;
    }

    return 0;
}
INLINE int dvmIsSubClass(const ClassObject* sub, const ClassObject* clazz) {
    do {
        /*printf("###### sub='%s' clazz='%s'\n", sub->name, clazz->name);*/
        if (sub == clazz)
            return 1;
        sub = sub->super;
    } while (sub != NULL);

    return 0;
}

static int isArrayInstanceOfArray(const ClassObject* subElemClass, int subDim,
                                  const ClassObject* clazz)
{
    //assert(dvmIsArrayClass(sub));
    assert(dvmIsArrayClass(clazz));

    /* "If T is an array type TC[]... one of the following must be true:
     *   TC and SC are the same primitive type.
     *   TC and SC are reference types and type SC can be cast to TC [...]."
     *
     * We need the class objects for the array elements.  For speed we
     * tucked them into the class object.
     */
    assert(subDim > 0 && clazz->arrayDim > 0);
    if (subDim == clazz->arrayDim) {
        /*
         * See if "sub" is an instance of "clazz".  This handles the
         * interfaces, java.lang.Object, superclassing, etc.
         */
        return dvmInstanceof(subElemClass, clazz->elementClass);
    } else if (subDim > clazz->arrayDim) {
        /*
         * The thing we might be an instance of has fewer dimensions.  It
         * must be an Object or array of Object, or a standard array
         * interface or array of standard array interfaces (the standard
         * interfaces being java/lang/Cloneable and java/io/Serializable).
         */
        if (dvmIsInterfaceClass(clazz->elementClass)) {
            /*
             * See if the class implements its base element.  We know the
             * base element is an interface; if the array class implements
             * it, we know it's a standard array interface.
             */
            return dvmImplements(clazz, clazz->elementClass);
        } else {
            /*
             * See if this is an array of Object, Object[], etc.  We know
             * that the superclass of an array is always Object, so we
             * just compare the element type to that.
             */
            return (clazz->elementClass == clazz->super);
        }
    } else {
        /*
         * Too many []s.
         */
        return false;
    }
}
static int isArrayInstanceOf(const ClassObject* sub, const ClassObject* clazz)
{
    assert(dvmIsArrayClass(sub));

    /* "If T is an interface type, T must be one of the interfaces
     * implemented by arrays."
     *
     * I'm not checking that here, because dvmInstanceof tests for
     * interfaces first, and the generic dvmImplements stuff should
     * work correctly.
     */
    assert(!dvmIsInterfaceClass(clazz));     /* make sure */

    /* "If T is a class type, then T must be Object."
     *
     * The superclass of an array is always java.lang.Object, so just
     * compare against that.
     */
    if (!dvmIsArrayClass(clazz))
        return BOOL_TO_INT(clazz == sub->super);

    /*
     * If T is an array type TC[] ...
     */
    return isArrayInstanceOfArray(sub->elementClass, sub->arrayDim, clazz);
}
static inline int isInstanceof(const ClassObject* instance,
                               const ClassObject* clazz)
{
    if (dvmIsInterfaceClass(clazz)) {
        return dvmImplements(instance, clazz);
    } else if (dvmIsArrayClass(instance)) {
        return isArrayInstanceOf(instance, clazz);
    } else {
        return dvmIsSubClass(instance, clazz);
    }
}
int dvmInstanceofNonTrivial(const ClassObject* instance,
                            const ClassObject* clazz)
{
#define ATOMIC_CACHE_CALC isInstanceof(instance, clazz)
#define ATOMIC_CACHE_NULL_ALLOWED true
    return ATOMIC_CACHE_LOOKUP(gDvm.instanceofCache,
                               INSTANCEOF_CACHE_SIZE, instance, clazz);
#undef ATOMIC_CACHE_CALC
}
INLINE int dvmInstanceof(const ClassObject* instance, const ClassObject* clazz)
{
    if (instance == clazz) {
        if (CALC_CACHE_STATS)
            gDvm.instanceofCache->trivial++;
        return 1;
    } else
        return dvmInstanceofNonTrivial(instance, clazz);
}
#endif //CUSTOMAPPVMP_TYPECHECK_H
