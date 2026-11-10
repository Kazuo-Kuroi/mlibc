#include <abi-bits/errno.h>
#include <abi-bits/fcntl.h>
#include <abi-bits/stat.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <belka/syscall.hpp>
#include <mlibc/all-sysdeps.hpp>

/*
 * getcwd() walks up from the current directory, as System V's C library
 * does: Belka has no call that names it. Each parent is reached by a
 * longer "../.." path, and the entry naming the child is the one with
 * the child's device and inode. Inode numbers in a directory are not
 * those of a file system mounted on an entry, so across a mount point
 * every entry is examined.
 */

namespace {

/* The kernel's directory entry: d_ino, d_off, d_reclen, then the name. */
struct kernel_dirent {
	uint64_t d_ino;
	int64_t d_off;
	uint16_t d_reclen;
	char d_name[];
};

int stat_path(const char *path, bool follow, struct stat *st) {
	return belka::sc(follow ? belka::SYS_stat : belka::SYS_lstat, path, st).error;
}

/*
 * Find, in directory dir (a path), the name of the entry that is child;
 * the name is stored in name, of size namesz.
 */
int find_entry(const char *dir, const struct stat &dirst, const struct stat &child,
		char *name, size_t namesz) {
	alignas(8) char buf[2048];
	char path[PATH_MAX];
	size_t dirlen = strlen(dir);
	int error = ENOENT;

	auto r = belka::sc(belka::SYS_open, dir, O_RDONLY, 0);
	if (r.error)
		return r.error;
	int fd = static_cast<int>(r.value);
	for (;;) {
		auto n = belka::sc(belka::SYS_getdents, fd, buf, sizeof(buf));
		if (n.error) {
			error = n.error;
			break;
		}
		if (n.value == 0)
			break;
		for (long off = 0; off < n.value;) {
			auto d = reinterpret_cast<kernel_dirent *>(buf + off);
			off += d->d_reclen;
			if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
				continue;
			/* On the same device, the entry's own inode number decides. */
			if (dirst.st_dev == child.st_dev && d->d_ino != child.st_ino)
				continue;
			size_t len = strlen(d->d_name);
			if (dirlen + 1 + len + 1 > sizeof(path))
				continue;
			memcpy(path, dir, dirlen);
			path[dirlen] = '/';
			memcpy(path + dirlen + 1, d->d_name, len + 1);
			struct stat st;
			if (stat_path(path, false, &st) != 0 || st.st_dev != child.st_dev ||
					st.st_ino != child.st_ino)
				continue;
			if (len + 1 > namesz) {
				error = ENAMETOOLONG;
			} else {
				memcpy(name, d->d_name, len + 1);
				error = 0;
			}
			goto done;
		}
	}
done:
	belka::sc(belka::SYS_close, fd);
	return error;
}

} // namespace

namespace mlibc {

int Sysdeps<GetCwd>::operator()(char *buffer, size_t size) {
	char up[PATH_MAX];
	char name[NAME_MAX + 1];
	struct stat root, cur, parent;
	size_t pos, uplen;

	if (int e = stat_path("/", true, &root); e)
		return e;
	if (int e = stat_path(".", true, &cur); e)
		return e;
	if (size < 2)
		return ERANGE;
	pos = size - 1;
	buffer[pos] = '\0';
	strcpy(up, ".");
	uplen = 1;
	while (cur.st_dev != root.st_dev || cur.st_ino != root.st_ino) {
		if (uplen + 3 >= sizeof(up))
			return ENAMETOOLONG;
		memcpy(up + uplen, "/..", 4);
		uplen += 3;
		if (int e = stat_path(up, true, &parent); e)
			return e;
		if (int e = find_entry(up, parent, cur, name, sizeof(name)); e)
			return e;
		size_t len = strlen(name);
		if (len + 1 > pos)
			return ERANGE;
		pos -= len;
		memcpy(buffer + pos, name, len);
		buffer[--pos] = '/';
		cur = parent;
	}
	if (pos == size - 1)
		buffer[--pos] = '/';
	memmove(buffer, buffer + pos, size - pos);
	return 0;
}

} // namespace mlibc
