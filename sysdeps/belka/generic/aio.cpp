#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include <belka/syscall.hpp>
#include <mlibc/all-sysdeps.hpp>

namespace {

enum class State { free, queued, running, done };
enum class Op { read, write, sync };

struct Request {
	const struct aiocb *key;
	State state;
	Op op;
	uint64_t ticket;
	int fd;
	off_t offset;
	void *buffer;
	size_t bytes;
	struct sigevent event;
	ssize_t result;
	int error;
};

Request requests[64];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
bool worker_started;
uint64_t next_ticket;
alignas(4) int completed;

void block_signals(sigset_t *old) {
	sigset_t all;
	sigfillset(&all);
	pthread_sigmask(SIG_SETMASK, &all, old);
}

// Result queries may run from a signal handler. Block signals while
// taking the registry lock and keep their paths free of allocation.
class Guard {
public:
	Guard() {
		block_signals(&old_);
		pthread_mutex_lock(&lock);
	}
	~Guard() {
		pthread_mutex_unlock(&lock);
		pthread_sigmask(SIG_SETMASK, &old_, nullptr);
	}
private:
	sigset_t old_;
};

Request *find(const struct aiocb *cb) {
	for (auto &r : requests)
		if (r.state != State::free && r.key == cb)
			return &r;
	return nullptr;
}

Request *next_request() {
	Request *oldest = nullptr;
	for (auto &r : requests)
		if (r.state == State::queued && (!oldest || r.ticket < oldest->ticket))
			oldest = &r;
	return oldest;
}

void *worker(void *) {
	block_signals(nullptr);
	for (;;) {
		pthread_mutex_lock(&lock);
		Request *r;
		while (!(r = next_request()))
			pthread_cond_wait(&changed, &lock);
		r->state = State::running;
		pthread_mutex_unlock(&lock);

		ssize_t result = -1;
		do {
			switch (r->op) {
			case Op::read:
				result = pread(r->fd, r->buffer, r->bytes, r->offset);
				break;
			case Op::write:
				if (fcntl(r->fd, F_GETFL) & O_APPEND)
					result = write(r->fd, r->buffer, r->bytes);
				else
					result = pwrite(r->fd, r->buffer, r->bytes, r->offset);
				break;
			case Op::sync:
				result = fsync(r->fd);
				break;
			}
		} while (result < 0 && errno == EINTR);
		int error = result < 0 ? errno : 0;
		auto event = r->event;

		pthread_mutex_lock(&lock);
		close(r->fd);
		r->fd = -1;
		r->result = result;
		r->error = error;
		r->state = State::done;
		__atomic_add_fetch(&completed, 1, __ATOMIC_RELEASE);
		pthread_mutex_unlock(&lock);

		mlibc::sysdep<FutexWake>(&completed, true);
		if (event.sigev_notify == SIGEV_SIGNAL) {
			const struct timespec retry = {0, 1000000};
			while (belka::sc(belka::SYS_aio_notify, event.sigev_signo,
					&event.sigev_value).error == EAGAIN)
				nanosleep(&retry, nullptr);
		}
	}
}

int submit(struct aiocb *cb, Op op) {
	if (!cb || (op != Op::sync && (cb->aio_reqprio != 0 || cb->aio_offset < 0)))
		return EINVAL;
	const auto event = cb->aio_sigevent;
	if (event.sigev_notify != SIGEV_NONE && event.sigev_notify != SIGEV_SIGNAL)
		return EINVAL;
	if (event.sigev_notify == SIGEV_SIGNAL &&
			(event.sigev_signo <= 0 || event.sigev_signo >= _NSIG))
		return EINVAL;

	Guard guard;
	if (find(cb))
		return EINVAL;
	Request *slot = nullptr;
	for (auto &r : requests)
		if (r.state == State::free) {
			slot = &r;
			break;
		}
	if (!slot)
		return EAGAIN;

	auto duplicate = belka::sc(belka::SYS_aio_dup, cb->aio_fildes);
	if (duplicate.error)
		return duplicate.error == EMFILE ? EAGAIN : duplicate.error;
	int fd = static_cast<int>(duplicate.value);
	int error = 0;
	if (!worker_started) {
		pthread_attr_t attr;
		pthread_t thread;
		error = pthread_attr_init(&attr);
		if (!error) {
			error = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			if (!error)
				error = pthread_create(&thread, &attr, worker, nullptr);
			pthread_attr_destroy(&attr);
		}
		if (!error)
			worker_started = true;
	}
	if (error) {
		close(fd);
		return error;
	}

	*slot = {cb, State::queued, op, next_ticket++, fd, 0, nullptr, 0, event, 0, 0};
	if (op != Op::sync) {
		slot->offset = cb->aio_offset;
		slot->buffer = const_cast<void *>(cb->aio_buf);
		slot->bytes = cb->aio_nbytes;
	}
	pthread_cond_signal(&changed);
	return 0;
}

} // namespace

namespace belka {

static void aio_fork_prepare(sigset_t *old) {
	block_signals(old);
	pthread_mutex_lock(&lock);
}

static void aio_fork_finish(const sigset_t *old, bool child) {
	if (child) {
		for (auto &r : requests)
			r.state = State::free;
		pthread_mutex_t fresh_lock = PTHREAD_MUTEX_INITIALIZER;
		pthread_cond_t fresh_changed = PTHREAD_COND_INITIALIZER;
		lock = fresh_lock;
		changed = fresh_changed;
		worker_started = false;
	} else {
		pthread_mutex_unlock(&lock);
	}
	pthread_sigmask(SIG_SETMASK, old, nullptr);
}

} // namespace belka

namespace mlibc {

int Sysdeps<Fork>::operator()(pid_t *child) {
	sigset_t mask;
	belka::aio_fork_prepare(&mask);
	auto result = belka::sc(belka::SYS_fork1);
	belka::aio_fork_finish(&mask, !result.error && result.value2);
	if (result.error)
		return result.error;
	*child = result.value2 ? 0 : static_cast<pid_t>(result.value);
	return 0;
}

int Sysdeps<AioRead>::operator()(struct aiocb *cb) {
	return submit(cb, Op::read);
}

int Sysdeps<AioWrite>::operator()(struct aiocb *cb) {
	return submit(cb, Op::write);
}

int Sysdeps<AioFsync>::operator()(int op, struct aiocb *cb) {
	if (op != O_SYNC && op != O_DSYNC)
		return EINVAL;
	return submit(cb, Op::sync);
}

int Sysdeps<AioError>::operator()(const struct aiocb *cb, int *out) {
	Guard guard;
	auto *r = find(cb);
	if (!r)
		return EINVAL;
	*out = r->state == State::done ? r->error : EINPROGRESS;
	return 0;
}

int Sysdeps<AioReturn>::operator()(struct aiocb *cb, ssize_t *out) {
	Guard guard;
	auto *r = find(cb);
	if (!r || r->state != State::done)
		return EINVAL;
	*out = r->result;
	r->state = State::free;
	return r->error;
}

int Sysdeps<AioSuspend>::operator()(const struct aiocb *const list[],
		int count, const struct timespec *timeout) {
	struct timespec start;
	if (count < 0 || (count != 0 && !list) || (timeout &&
			(timeout->tv_sec < 0 || timeout->tv_nsec < 0 || timeout->tv_nsec >= 1000000000)))
		return EINVAL;
	if (timeout && clock_gettime(CLOCK_MONOTONIC, &start) < 0)
		return errno;

	for (;;) {
		int generation;
		{
			Guard guard;
			bool any = false;
			for (int i = 0; i < count; i++) {
				if (!list[i])
					continue;
				auto *r = find(list[i]);
				if (!r)
					return EINVAL;
				if (r->state == State::done)
					return 0;
				any = true;
			}
			if (!any)
				return 0;
			generation = __atomic_load_n(&completed, __ATOMIC_ACQUIRE);
		}

		struct timespec remaining;
		if (timeout) {
			struct timespec now;
			if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
				return errno;
			remaining.tv_sec = timeout->tv_sec - (now.tv_sec - start.tv_sec);
			remaining.tv_nsec = timeout->tv_nsec - (now.tv_nsec - start.tv_nsec);
			if (remaining.tv_nsec < 0) {
				--remaining.tv_sec;
				remaining.tv_nsec += 1000000000;
			} else if (remaining.tv_nsec >= 1000000000) {
				++remaining.tv_sec;
				remaining.tv_nsec -= 1000000000;
			}
			if (remaining.tv_sec < 0)
				return EAGAIN;
		}
		int error = sysdep<FutexWait>(&completed, generation, timeout ? &remaining : nullptr);
		if (error == ETIMEDOUT)
			return EAGAIN;
		if (error && error != EAGAIN)
			return error;
	}
}

} // namespace mlibc
