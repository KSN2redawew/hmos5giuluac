/* Minimal hilog wrapper for the sweet display HALs (links against libhilog.so). */
#ifndef SWEET_LOG_H
#define SWEET_LOG_H
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif
int HiLogPrint(int type, int level, unsigned int domain, const char *tag, const char *fmt, ...)
    __attribute__((__format__(os_log, 5, 6)));
#ifdef __cplusplus
}
#endif
#define SWEET_LOG_DOMAIN 0xD002590
#ifndef SWEET_LOG_TAG
#define SWEET_LOG_TAG "SweetHAL"
#endif
/* LOG_CORE = 3, levels: DEBUG 3, INFO 4, WARN 5, ERROR 6 */
#define SLOGI(fmt, ...) HiLogPrint(3, 4, SWEET_LOG_DOMAIN, SWEET_LOG_TAG, "%{public}s: " fmt, __func__, ##__VA_ARGS__)
#define SLOGW(fmt, ...) HiLogPrint(3, 5, SWEET_LOG_DOMAIN, SWEET_LOG_TAG, "%{public}s: " fmt, __func__, ##__VA_ARGS__)
#define SLOGE(fmt, ...) HiLogPrint(3, 6, SWEET_LOG_DOMAIN, SWEET_LOG_TAG, "%{public}s: " fmt, __func__, ##__VA_ARGS__)
#endif
