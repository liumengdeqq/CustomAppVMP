//
// Created by liu meng on 2018/8/20.
//

#ifndef CUSTOMAPPVMP_JDWP_H
#define CUSTOMAPPVMP_JDWP_H
enum JdwpTransportType {
    kJdwpTransportUnknown = 0,
    kJdwpTransportSocket,       /* transport=dt_socket */
    kJdwpTransportAndroidAdb,   /* transport=dt_android_adb */
};
#endif //CUSTOMAPPVMP_JDWP_H
