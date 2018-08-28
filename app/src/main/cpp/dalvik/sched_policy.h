#ifndef __CUTILS_SCHED_POLICY_H
#define __CUTILS_SCHED_POLICY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Keep in sync with THREAD_GROUP_* in frameworks/base/core/java/android/os/Process.java */
typedef enum {
    SP_DEFAULT    = -1,
    SP_BACKGROUND = 0,
    SP_FOREGROUND = 1,
    SP_SYSTEM     = 2,  // can't be used with set_sched_policy()
    SP_AUDIO_APP  = 3,
    SP_AUDIO_SYS  = 4,
    SP_CNT,
    SP_MAX        = SP_CNT - 1,
    SP_SYSTEM_DEFAULT = SP_FOREGROUND,
} SchedPolicy;

/* Assign thread tid to the cgroup associated with the specified policy.
 * If the thread is a thread group leader, that is it's gettid() == getpid(),
 * then the other threads in the same thread group are _not_ affected.
 * On platforms which support gettid(), zero tid means current thread.
 * Return value: 0 for success, or -errno for error.
 */
extern int set_sched_policy(int tid, SchedPolicy policy);

/* Return the policy associated with the cgroup of thread tid via policy pointer.
 * On platforms which support gettid(), zero tid means current thread.
 * Return value: 0 for success, or -1 for error and set errno.
 */
extern int get_sched_policy(int tid, SchedPolicy *policy);

/* Return a displayable string corresponding to policy.
 * Return value: non-NULL NUL-terminated name of unspecified length;
 * the caller is responsible for displaying the useful part of the string.
 */
extern const char *get_sched_policy_name(SchedPolicy policy);

#ifdef __cplusplus
}
#endif

#endif /* __CUTILS_SCHED_POLICY_H */
