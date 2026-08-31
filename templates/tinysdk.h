#ifndef TINYDISTRO_TINYSDK_H
#define TINYDISTRO_TINYSDK_H

/*
 * TinyDistro SDK
 * AppBuilder template
 *
 * TinyDistro v2.3.0
 *
 * This file is used as the base template for tsdkgen.
 * Applications include the generated tinysdk.h.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TINYSDK_VERSION_MAJOR 2
#define TINYSDK_VERSION_MINOR 3
#define TINYSDK_VERSION_PATCH 0

#define TINYSDK_VERSION "2.3.0"
#define TINYSDK_NAME "TinyDistro SDK"

#define TINYDISTRO_NAME "TinyDistro"
#define TINYDISTRO_VERSION "2.3.0"

#define TSDK_SUCCESS 0
#define TSDK_ERROR   -1

#define TSDK_TRUE  1
#define TSDK_FALSE 0

typedef int tsdk_status_t;
typedef int tsdk_pid_t;
typedef uint64_t tsdk_size_t;
typedef uint64_t tsdk_time_t;

typedef struct
{
    const char *name;
    const char *version;
    const char *sdk_version;
} tsdk_info_t;

typedef struct
{
    uint64_t total;
    uint64_t used;
    uint64_t free;
} tsdk_memory_info_t;

typedef struct
{
    int available;
    const char *hostname;
} tsdk_network_info_t;

typedef struct
{
    tsdk_pid_t pid;
    const char *name;
} tsdk_process_info_t;


/*
 * Application API
 */

int tsdk_init(void);

void tsdk_shutdown(void);

const char *tsdk_version(void);

const char *tsdk_app_name(void);

const tsdk_info_t *tsdk_info(void);


/*
 * Console API
 */

int tsdk_print(const char *message);

int tsdk_puts(const char *message);

int tsdk_putchar(int character);

int tsdk_getchar(void);

int tsdk_printf(const char *format, ...);


/*
 * Filesystem API
 */

int tsdk_file_exists(const char *path);

long tsdk_file_read(
    const char *path,
    void *buffer,
    unsigned long size
);

long tsdk_file_write(
    const char *path,
    const void *buffer,
    unsigned long size
);

int tsdk_file_remove(const char *path);

int tsdk_mkdir(const char *path);

long tsdk_file_size(const char *path);


/*
 * Process API
 */

tsdk_pid_t tsdk_process_id(void);

const char *tsdk_process_name(void);

void tsdk_process_exit(int status);

int tsdk_process_running(tsdk_pid_t pid);


/*
 * Memory API
 */

void *tsdk_malloc(unsigned long size);

void *tsdk_calloc(
    unsigned long count,
    unsigned long size
);

void *tsdk_realloc(
    void *pointer,
    unsigned long size
);

void tsdk_free(void *pointer);

unsigned long tsdk_meminfo(void);

int tsdk_memory_info(
    tsdk_memory_info_t *info
);


/*
 * System API
 */

const char *tsdk_system_name(void);

const char *tsdk_system_version(void);

unsigned long tsdk_uptime(void);

int tsdk_get_info(tsdk_info_t *info);


/*
 * Network API
 */

int tsdk_network_available(void);

const char *tsdk_hostname(void);

int tsdk_network_info(
    tsdk_network_info_t *info
);


/*
 * Device API
 */

int tsdk_device_exists(const char *name);


/*
 * Time API
 */

tsdk_time_t tsdk_time(void);

void tsdk_sleep(unsigned long milliseconds);


/*
 * TinyDistro application entry point
 *
 * Applications normally provide:
 *
 * int main(int argc, char **argv);
 */

#ifndef TINYSDK_NO_MAIN_DECL
int main(int argc, char **argv);
#endif


/*
 * Compiler helpers
 */

#if defined(__GNUC__) || defined(__clang__)
#define TSDK_UNUSED __attribute__((unused))
#define TSDK_NORETURN __attribute__((noreturn))
#define TSDK_WEAK __attribute__((weak))
#else
#define TSDK_UNUSED
#define TSDK_NORETURN
#define TSDK_WEAK
#endif


/*
 * SDK feature detection
 */

#define TSDK_HAS_APPLICATION 1
#define TSDK_HAS_CONSOLE     1
#define TSDK_HAS_FILESYSTEM  1
#define TSDK_HAS_PROCESS     1
#define TSDK_HAS_MEMORY      1
#define TSDK_HAS_SYSTEM      1
#define TSDK_HAS_NETWORK     1
#define TSDK_HAS_DEVICE      1
#define TSDK_HAS_TIME        1


/*
 * TinyDistro build information
 */

#define TSDK_BUILD_TARGET "TinyDistro"
#define TSDK_BUILD_SYSTEM "TinyDistro"
#define TSDK_BUILD_ABI    "tinydistro"


/*
 * Application convenience macros
 */

#define TSDK_OK TSDK_SUCCESS

#define TSDK_EXIT_SUCCESS 0
#define TSDK_EXIT_FAILURE 1

#define TSDK_UNUSED_ARG(x) \
    ((void)(x))

#define TSDK_MIN(a,b) \
    ((a) < (b) ? (a) : (b))

#define TSDK_MAX(a,b) \
    ((a) > (b) ? (a) : (b))


/*
 * Static SDK information.
 *
 * These values are intentionally compile-time constants.
 * No additional runtime data is required.
 */

static const tsdk_info_t tsdk_build_info =
{
    TINYDISTRO_NAME,
    TINYDISTRO_VERSION,
    TSDK_VERSION
};


/*
 * TinyDistro SDK inline helpers
 */

static inline const char *
tsdk_distro_name(void)
{
    return TINYDISTRO_NAME;
}

static inline const char *
tsdk_distro_version(void)
{
    return TINYDISTRO_VERSION;
}

static inline const char *
tsdk_sdk_name(void)
{
    return TINYSDK_NAME;
}

static inline int
tsdk_is_success(tsdk_status_t status)
{
    return status == TSDK_SUCCESS;
}

static inline int
tsdk_is_error(tsdk_status_t status)
{
    return status < 0;
}


/*
 * API groups
 */

#define TSDK_APPLICATION_API 1
#define TSDK_CONSOLE_API     2
#define TSDK_FILESYSTEM_API  3
#define TSDK_PROCESS_API     4
#define TSDK_MEMORY_API      5
#define TSDK_SYSTEM_API      6
#define TSDK_NETWORK_API     7
#define TSDK_DEVICE_API      8
#define TSDK_TIME_API        9


/*
 * End of TinyDistro SDK template.
 */

#ifdef __cplusplus
}
#endif

#endif
