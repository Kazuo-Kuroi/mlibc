#include <errno.h>
#include <stdarg.h>
#include <sys/mount.h>

#include <belka/syscall.hpp>

/*
 * mount(spec, dir, mflag[, fstype, dataptr, datalen]) and umount(file):
 * Belka's calls 21 and 22, System V's. With MS_DATA the three that
 * follow are given; without it the kernel refuses the call, so they are
 * read only then.
 */
int mount(const char *spec, const char *dir, int mflag, ...) {
	const char *fstype = nullptr, *data = nullptr;
	int datalen = 0;

	if (mflag & MS_DATA) {
		va_list ap;
		va_start(ap, mflag);
		fstype = va_arg(ap, const char *);
		data = va_arg(ap, const char *);
		datalen = va_arg(ap, int);
		va_end(ap);
	}
	auto r = belka::sc(belka::SYS_mount, spec, dir, mflag, fstype, data, datalen);
	if (r.error) {
		errno = r.error;
		return -1;
	}
	return 0;
}

int umount(const char *file) {
	auto r = belka::sc(belka::SYS_umount, file);
	if (r.error) {
		errno = r.error;
		return -1;
	}
	return 0;
}
