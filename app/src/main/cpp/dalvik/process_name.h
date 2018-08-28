//
// Created by liu meng on 2018/8/27.
//

#ifndef __PROCESS_NAME_H
#define __PROCESS_NAME_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sets the current process name.
 *
 * Warning: This leaks a string every time you call it. Use judiciously!
 */
void set_process_name(const char* process_name);

/** Gets the current process name. */
const char* get_process_name(void);

#ifdef __cplusplus
}
#endif

#endif /* __PROCESS_NAME_H */
