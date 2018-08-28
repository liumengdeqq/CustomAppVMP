//
// Created by liu meng on 2018/8/28.
//

#include "customlog.h"
#include "sockets.h"

#ifdef HAVE_ANDROID_OS
/* For the socket trust (credentials) check */
#include <private/android_filesystem_config.h>
#endif

bool socket_peer_is_trusted(int fd)
{
#ifdef HAVE_ANDROID_OS
    struct ucred cr;
    socklen_t len = sizeof(cr);
    int n = getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cr, &len);

    if (n != 0) {
        ALOGE("could not get socket credentials: %s\n", strerror(errno));
        return false;
    }

    if ((cr.uid != AID_ROOT) && (cr.uid != AID_SHELL)) {
        ALOGE("untrusted userid on other end of socket: userid %d\n", cr.uid);
        return false;
    }
#endif

    return true;
}
