#include <pthread.h>

/* 
 * Stub pthread_getattr_np because glibc’s implementation reads
 * /proc/self/maps, which isn’t available in the Flatpak apply_extra sandbox.
 * Always returns success with dummy stack info.
 */
int pthread_getattr_np(pthread_t thread, pthread_attr_t *attr)
{
    if (attr) {
        pthread_attr_init(attr);
        pthread_attr_setstack(attr, (void*)0x1000000, 8*1024*1024); // fake 8 MB stack
    }
    return 0;
}
