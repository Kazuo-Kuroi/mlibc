#include <errno.h>
#include <stropts.h>

#include <belka/syscall.hpp>

/*
 * getmsg and putmsg are System V's calls 85 and 86; getpmsg and putpmsg
 * are Belka's 246 and 247. isastream asks the stream head a question
 * only a stream answers.
 */

static int result(belka::Result r) {
	if (r.error) {
		errno = r.error;
		return -1;
	}
	return static_cast<int>(r.value);
}

int getmsg(int fd, struct strbuf *ctlptr, struct strbuf *dataptr, int *flagsp) {
	return result(belka::sc(belka::SYS_getmsg, fd, ctlptr, dataptr, flagsp));
}

int getpmsg(int fd, struct strbuf *ctlptr, struct strbuf *dataptr, int *bandp,
		int *flagsp) {
	return result(belka::sc(belka::SYS_getpmsg, fd, ctlptr, dataptr, bandp, flagsp));
}

int putmsg(int fd, const struct strbuf *ctlptr, const struct strbuf *dataptr,
		int flags) {
	return result(belka::sc(belka::SYS_putmsg, fd, ctlptr, dataptr, flags));
}

int putpmsg(int fd, const struct strbuf *ctlptr, const struct strbuf *dataptr,
		int band, int flags) {
	return result(belka::sc(belka::SYS_putpmsg, fd, ctlptr, dataptr, band, flags));
}

int isastream(int fd) {
	auto r = belka::sc(belka::SYS_ioctl, fd, I_CANPUT, 0);
	if (r.error == ENOSTR)
		return 0;
	if (r.error) {
		errno = r.error;
		return -1;
	}
	return 1;
}
