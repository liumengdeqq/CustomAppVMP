//
// Created by liu meng on 2018/8/20.
//

#ifndef CUSTOMAPPVMP_DEXPREPARE_H
#define CUSTOMAPPVMP_DEXPREPARE_H
enum VerifyError {
    VERIFY_ERROR_NONE = 0,      /* no error; must be zero */
    VERIFY_ERROR_GENERIC,       /* VerifyError */

    VERIFY_ERROR_NO_CLASS,      /* NoClassDefFoundError */
    VERIFY_ERROR_NO_FIELD,      /* NoSuchFieldError */
    VERIFY_ERROR_NO_METHOD,     /* NoSuchMethodError */
    VERIFY_ERROR_ACCESS_CLASS,  /* IllegalAccessError */
    VERIFY_ERROR_ACCESS_FIELD,  /* IllegalAccessError */
    VERIFY_ERROR_ACCESS_METHOD, /* IllegalAccessError */
    VERIFY_ERROR_CLASS_CHANGE,  /* IncompatibleClassChangeError */
    VERIFY_ERROR_INSTANTIATION, /* InstantiationError */
};
#define VERIFY_OK(_failure) ((_failure) == VERIFY_ERROR_NONE)
enum DexOptimizerMode {
    OPTIMIZE_MODE_UNKNOWN = 0,
    OPTIMIZE_MODE_NONE,         /* never optimize (except "essential") */
    OPTIMIZE_MODE_VERIFIED,     /* only optimize verified classes (default) */
    OPTIMIZE_MODE_ALL,          /* optimize verified & unverified (risky) */
    OPTIMIZE_MODE_FULL          /* fully opt verified classes at load time */
};


#endif //CUSTOMAPPVMP_DEXPREPARE_H
