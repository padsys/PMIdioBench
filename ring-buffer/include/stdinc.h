/*
 * Copyright (c) 2020, The Ohio State University. All rights reserved.
 *
 * This file is part of the PMIdioBench software package developed by
 * the team members of Prof. Xiaoyi Lu's group at The Ohio State University.
 *
 * For detailed copyright and licensing information, please refer to the license
 * file LICENSE in the top level directory.
 *
 */

/** @file
 * Standard C headers
 *
 * This file is intended to be included first by all other Q files.
 */

#ifndef Q_STDINC_H
#define Q_STDINC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Standard C */
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* POSIX */
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/time.h>

/* declare short versions of uint* types */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* define force_inline macro */
#ifdef _MSC_VER /* Visual Studio */
#define force_inline __forceinline
#elif defined (__GNUC__) /* ...also includes clang which claims to be gcc 4.2.1 */
#if __GNUC__ > 3 || ( __GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define force_inline inline __attribute__((always_inline))
#else
#define force_inline __inline__ /* pre-3.1 didn't have "always_inline" but __inline__ has been around for ages */
#endif
#elif defined (__cplusplus)
#define force_inline inline /* someone is compiling us with a C++ compiler; "inline" should be safe */
#elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define force_inline inline /* any C99 compiler should have it as well */
#else
#define force_inline /* oh well, best we can do... */
#endif

#ifdef __cplusplus
}
#endif

#endif /* Q_STDINC_H */
