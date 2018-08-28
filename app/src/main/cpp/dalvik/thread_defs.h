#ifndef ANDROID_THREAD_DEFS_H
#define ANDROID_THREAD_DEFS_H


#if defined(__cplusplus)
extern "C" {
#endif
#define HAL_PRIORITY_URGENT_DISPLAY     (-8)
enum {
    /*
     * ***********************************************
     * ** Keep in sync with android.os.Process.java **
     * ***********************************************
     *
     * This maps directly to the "nice" priorities we use in Android.
     * A thread priority should be chosen inverse-proportionally to
     * the amount of work the thread is expected to do. The more work
     * a thread will do, the less favorable priority it should get so that
     * it doesn't starve the system. Threads not behaving properly might
     * be "punished" by the kernel.
     * Use the levels below when appropriate. Intermediate values are
     * acceptable, preferably use the {MORE|LESS}_FAVORABLE constants below.
     */
            ANDROID_PRIORITY_LOWEST         =  19,

    /* use for background tasks */
            ANDROID_PRIORITY_BACKGROUND     =  10,

    /* most threads run at normal priority */
            ANDROID_PRIORITY_NORMAL         =   0,

    /* threads currently running a UI that the user is interacting with */
            ANDROID_PRIORITY_FOREGROUND     =  -2,

    /* the main UI thread has a slightly more favorable priority */
            ANDROID_PRIORITY_DISPLAY        =  -4,

    /* ui service treads might want to run at a urgent display (uncommon) */
            ANDROID_PRIORITY_URGENT_DISPLAY =  HAL_PRIORITY_URGENT_DISPLAY,

    /* all normal audio threads */
            ANDROID_PRIORITY_AUDIO          = -16,

    /* service audio threads (uncommon) */
            ANDROID_PRIORITY_URGENT_AUDIO   = -19,

    /* should never be used in practice. regular process might not
     * be allowed to use this level */
            ANDROID_PRIORITY_HIGHEST        = -20,

    ANDROID_PRIORITY_DEFAULT        = ANDROID_PRIORITY_NORMAL,
    ANDROID_PRIORITY_MORE_FAVORABLE = -1,
    ANDROID_PRIORITY_LESS_FAVORABLE = +1,
};

#if defined(__cplusplus)
}
#endif

#endif /* ANDROID_THREAD_DEFS_H */
