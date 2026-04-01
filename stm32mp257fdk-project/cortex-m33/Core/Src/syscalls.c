/**
 * @file  syscalls.c
 * @brief Minimal newlib syscall stubs for bare-metal Cortex-M33.
 *
 * Providing these as strong symbols overrides the weak stubs in nosys.specs,
 * eliminating the "not implemented and will always fail" linker warnings.
 *
 * None of these are expected to be called at runtime: the firmware uses
 * uart_log_printf() directly instead of printf/fprintf, and there is no
 * filesystem or OS.  Any accidental call will hit Error_Handler().
 */

#include "main.h"
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Silence unused-parameter warnings */
#define UNUSED(x) ((void)(x))

int _close(int fd)
{
    UNUSED(fd);
    errno = EBADF;
    return -1;
}

off_t _lseek(int fd, off_t offset, int whence)
{
    UNUSED(fd);
    UNUSED(offset);
    UNUSED(whence);
    errno = EBADF;
    return (off_t)-1;
}

ssize_t _read(int fd, void *buf, size_t len)
{
    UNUSED(fd);
    UNUSED(buf);
    UNUSED(len);
    errno = EBADF;
    return -1;
}

ssize_t _write(int fd, const void *buf, size_t len)
{
    UNUSED(fd);
    UNUSED(buf);
    UNUSED(len);
    errno = EBADF;
    return -1;
}

int _fstat(int fd, struct stat *st)
{
    UNUSED(fd);
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fd)
{
    UNUSED(fd);
    return 1;
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    UNUSED(pid);
    UNUSED(sig);
    errno = EINVAL;
    return -1;
}

__attribute__((noreturn)) void _exit(int status)
{
    UNUSED(status);
    Error_Handler();
    /* unreachable — Error_Handler spins forever */
    for (;;) {}
}
