
#include <errno.h>
#include <sys/ioctl.h>

#include <bits/ensure.h>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/debug.hpp>

int ioctl(int fd, unsigned long request, ...) {
	va_list args;
	va_start(args, request);
	int result;
	void *arg;
#ifdef I_SETSIG
	if (request == I_SETSIG)
		arg = reinterpret_cast<void *>(static_cast<unsigned int>(va_arg(args, int)));
	else if (request == I_GETSIG)
		arg = va_arg(args, int *);
	else
#endif
		arg = va_arg(args, void *);
	if(int e = mlibc::sysdep_or_enosys<Ioctl>(fd, request, arg, &result); e) {
		va_end(args);
		errno = e;
		return -1;
	}
	va_end(args);
	return result;
}

