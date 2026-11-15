#pragma once

#include <abi-bits/errno.h>
#include <abi-bits/pid_t.h>
#include <abi-bits/seek-whence.h>
#include <abi-bits/utmp-defines.h>
#include <bits/ensure.h>
#include <bits/timeval.h>
#include <bits/size_t.h>
#include <mlibc/all-sysdeps.hpp>
#include <string.h>
#include <type_traits>
#include <utmpx.h>

namespace mlibc {

template <class S>
concept UtmpStruct = requires(S s) {
	std::is_same_v<decltype(s.ut_user), char[]>;
	std::is_same_v<decltype(s.ut_id), char[]>;
	std::is_same_v<decltype(s.ut_line), char[]>;
	std::is_same_v<decltype(s.ut_pid), pid_t>;
	std::is_same_v<decltype(s.ut_type), short int>;
	std::is_same_v<decltype(s.ut_tv), struct timeval>;
};

template <UtmpStruct U>
int getUtmpEntry(int fd, U *res) {
	ssize_t progress = 0;
	ssize_t read = 0;
	char *ptr = reinterpret_cast<char *>(res);

	int err = sysdep<Read>(fd, ptr, sizeof(U), &read);
	if(err)
		return err;

	if(read == sizeof(U))
		return 0;
	else if(read == 0)
		return ESRCH;

	progress = read;

	while(read) {
		err = sysdep<Read>(fd, ptr + progress, sizeof(U) - progress, &read);
		if(err)
			return err;

		progress += read;

		if(progress == sizeof(U))
			return 0;
	}

	return ESRCH;
}

template <UtmpStruct U>
int getUtmpEntryById(int fd, const U *id, U *res) {
	while(true) {
		U tmp;
		if(int e = getUtmpEntry(fd, &tmp); e)
			return e;

		if(id->ut_type == RUN_LVL || id->ut_type == BOOT_TIME
		|| id->ut_type == NEW_TIME || id->ut_type == OLD_TIME) {
			if(tmp.ut_type == id->ut_type) {
				memcpy(res, &tmp, sizeof(U));
				return 0;
			}
		} else if(id->ut_type == INIT_PROCESS || id->ut_type == LOGIN_PROCESS
		|| id->ut_type == USER_PROCESS || id->ut_type == DEAD_PROCESS) {
			if(!memcmp(tmp.ut_id, id->ut_id, sizeof(U::ut_id))) {
				memcpy(res, &tmp, sizeof(U));
				return 0;
			}
		}
	}
}

template <UtmpStruct U>
int getUtmpEntryByType(int fd, const U *id, U *res) {
	while(true) {
		U tmp;
		if(int e = getUtmpEntry(fd, &tmp); e)
			return e;

		// POSIX: the next USER_PROCESS or LOGIN_PROCESS entry with the line:
		// the entry's type counts, not the key's, which need not be set.
		if(tmp.ut_type == USER_PROCESS || tmp.ut_type == LOGIN_PROCESS) {
			if(!strncmp(tmp.ut_line, id->ut_line, sizeof(U::ut_line))) {
				memcpy(res, &tmp, sizeof(U));
				return 0;
			}
		}
	}
}

// Whether entry e is the one key names, by getutxid()'s rules.
template <UtmpStruct U>
bool utmpEntryMatches(const U *key, const U *e) {
	if(key->ut_type == RUN_LVL || key->ut_type == BOOT_TIME
	|| key->ut_type == NEW_TIME || key->ut_type == OLD_TIME)
		return e->ut_type == key->ut_type;
	if(key->ut_type == INIT_PROCESS || key->ut_type == LOGIN_PROCESS
	|| key->ut_type == USER_PROCESS || key->ut_type == DEAD_PROCESS)
		return (e->ut_type == INIT_PROCESS || e->ut_type == LOGIN_PROCESS
		|| e->ut_type == USER_PROCESS || e->ut_type == DEAD_PROCESS)
		&& !memcmp(e->ut_id, key->ut_id, sizeof(U::ut_id));
	return false;
}

template <UtmpStruct U>
int putUtmpEntry(int fd, const U *ut);

// pututxline(): write ut over the entry it names, from the start of the
// file, or add it at the end if there is none.
template <UtmpStruct U>
int replaceUtmpEntry(int fd, const U *ut) {
	off_t pos;
	if(int e = sysdep<Seek>(fd, 0, SEEK_SET, &pos); e)
		return e;
	while(true) {
		U tmp;
		int e = getUtmpEntry(fd, &tmp);
		if(e == ESRCH)
			break;
		if(e)
			return e;
		if(utmpEntryMatches(ut, &tmp)) {
			off_t discard;
			if(int e2 = sysdep<Seek>(fd, pos, SEEK_SET, &discard); e2)
				return e2;
			size_t progress = 0;
			const char *ptr = reinterpret_cast<const char *>(ut);
			while(progress < sizeof(U)) {
				ssize_t written = 0;
				if(int e3 = sysdep<Write>(fd, ptr + progress, sizeof(U) - progress, &written); e3)
					return e3;
				progress += written;
			}
			return 0;
		}
		pos += sizeof(U);
	}
	return putUtmpEntry(fd, ut);
}

template <UtmpStruct U>
int putUtmpEntry(int fd, const U *ut) {
	size_t progress = 0;
	char *ptr = (char *) ut;

	off_t discard;
	if(int e = sysdep<Seek>(fd, 0, SEEK_END, &discard); e)
		return e;

	while(progress < sizeof(U)) {
		ssize_t written = 0;
		if(int e = sysdep<Write>(fd, ptr + progress, sizeof(U) - progress, &written); e)
			return e;
		progress += written;
	}

	return 0;
}

struct utmpx *getutxline_r(const struct utmpx *__ut, struct utmpx *__buffer);

} // namespace mlibc
