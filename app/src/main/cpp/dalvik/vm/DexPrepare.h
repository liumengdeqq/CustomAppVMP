//
// Created by liu meng on 2018/8/20.
//

#ifndef CUSTOMAPPVMP_DEXPREPARE_H
#define CUSTOMAPPVMP_DEXPREPARE_H
enum DexOptimizerMode {
    OPTIMIZE_MODE_UNKNOWN = 0,
    OPTIMIZE_MODE_NONE,         /* never optimize (except "essential") */
    OPTIMIZE_MODE_VERIFIED,     /* only optimize verified classes (default) */
    OPTIMIZE_MODE_ALL,          /* optimize verified & unverified (risky) */
    OPTIMIZE_MODE_FULL          /* fully opt verified classes at load time */
};
#endif //CUSTOMAPPVMP_DEXPREPARE_H
