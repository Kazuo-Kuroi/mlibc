#include <mlibc/lookup.hpp>
#include <mlibc/resolv_conf.hpp>
#include <mlibc/debug.hpp>
#include <mlibc/services.hpp>
#include <bits/ensure.h>

#include <frg/string.hpp>
#include <mlibc/allocator.hpp>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <poll.h>
#include <frg/scope_exit.hpp>
#include <mlibc/all-sysdeps.hpp>
#include <time.h>
#include <fcntl.h>

namespace mlibc {

namespace {
	constexpr unsigned int RCODE_SERVFAIL = 2;
	constexpr unsigned int RCODE_NXDOMAIN = 3;

	constexpr unsigned int RECORD_A = 1;
	constexpr unsigned int RECORD_CNAME = 5;
	constexpr unsigned int RECORD_PTR = 12;
	constexpr unsigned int RECORD_AAAA = 28;
	constexpr unsigned int CLASS_IN = 1;

	constexpr uint16_t FLAG_QR = 0x8000;
	constexpr uint16_t FLAG_TC = 0x0200;
	constexpr uint16_t FLAG_RD = 0x0100;

	// A reply over UDP, without EDNS, is 512 bytes at most (RFC 1035 4.2.1).
	constexpr size_t DNS_MAXUDP = 512;
	constexpr size_t DNS_HEADER = 12;
	constexpr int DNS_TRIES = 2;
	constexpr int DNS_TIMEOUT_MS = 2500;	// each try

	using string = frg::string<MemoryAllocator>;

	uint16_t get16(const uint8_t *p) {
		return static_cast<uint16_t>((p[0] << 8) | p[1]);
	}

	// A query's ID: unpredictable, from /dev/urandom, so that a reply
	// cannot easily be forged; failing that, from the clock.
	uint16_t new_id() {
		uint16_t id = 0;
		int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
		if (fd >= 0) {
			ssize_t n = read(fd, &id, sizeof(id));
			close(fd);
			if (n == sizeof(id))
				return id;
		}
		struct timespec ts;
		if (!mlibc::sysdep<ClockGet>(CLOCK_MONOTONIC, &ts.tv_sec, &ts.tv_nsec))
			id = static_cast<uint16_t>(ts.tv_nsec ^ (ts.tv_nsec >> 16));
		return static_cast<uint16_t>(id ^ getpid());
	}

	// A query for name, type qtype, class IN, recursion desired. False if
	// name is no domain name: a label empty or longer than 63 bytes, or
	// the whole longer than 253.
	bool make_query(string &q, uint16_t id, const char *name, uint16_t qtype) {
		size_t len = strlen(name);
		if (len > 0 && name[len - 1] == '.')
			len--;
		if (len == 0 || len > 253)
			return false;

		uint8_t hdr[DNS_HEADER] = {
			static_cast<uint8_t>(id >> 8), static_cast<uint8_t>(id),
			FLAG_RD >> 8, 0, 0, 1, 0, 0, 0, 0, 0, 0
		};
		q.resize(0);
		for (size_t i = 0; i < DNS_HEADER; i++)
			q += static_cast<char>(hdr[i]);
		for (size_t pos = 0; pos < len; ) {
			size_t end = pos;
			while (end < len && name[end] != '.')
				end++;
			if (end == pos || end - pos > 63)
				return false;
			q += static_cast<char>(end - pos);
			q += frg::string_view{name + pos, end - pos};
			pos = end + 1;
		}
		q += static_cast<char>(0);
		q += static_cast<char>(qtype >> 8);
		q += static_cast<char>(qtype & 0xFF);
		q += static_cast<char>(0);
		q += static_cast<char>(CLASS_IN);
		return true;
	}

	// The name at off in msg (len bytes), its compression pointers
	// followed, into *out if out is not null, its labels joined by dots.
	// The offset just past it where it starts, or -1 if it is malformed:
	// running past the message, a pointer loop, or longer than 255.
	long read_name(const uint8_t *msg, size_t len, size_t off, string *out) {
		long next = -1;
		size_t total = 0;
		int jumps = 0;

		if (out)
			out->resize(0);
		for (;;) {
			if (off >= len)
				return -1;
			uint8_t c = msg[off];
			if ((c & 0xC0) == 0xC0) {
				if (off + 1 >= len || ++jumps > 64)
					return -1;
				if (next < 0)
					next = static_cast<long>(off + 2);
				off = (static_cast<size_t>(c & 0x3F) << 8) | msg[off + 1];
				continue;
			}
			if ((c & 0xC0) != 0)
				return -1;	// the extended label types
			if (c == 0) {
				if (next < 0)
					next = static_cast<long>(off + 1);
				return next;
			}
			if (off + 1 + c > len || (total += c + 1u) > 255)
				return -1;
			if (out) {
				if (out->size())
					*out += '.';
				*out += frg::string_view{reinterpret_cast<const char *>(msg + off + 1), c};
			}
			off += 1 + c;
		}
	}

	// Whether the reply's question (at DNS_HEADER) is the query's, names
	// compared without regard to case.
	bool same_question(const uint8_t *r, size_t rlen, const string &q) {
		size_t qlen = q.size() - DNS_HEADER;
		auto qp = reinterpret_cast<const uint8_t *>(q.data()) + DNS_HEADER;
		if (rlen < DNS_HEADER + qlen)
			return false;
		for (size_t i = 0; i < qlen; i++) {
			uint8_t a = r[DNS_HEADER + i], b = qp[i];
			if (a != b && tolower(a) != tolower(b))
				return false;
		}
		return true;
	}

	// The time left of ms since start, in ms; 0 if none.
	int time_left(const struct timespec &start, int ms) {
		struct timespec now;
		if (int e = mlibc::sysdep<ClockGet>(CLOCK_MONOTONIC, &now.tv_sec, &now.tv_nsec); e)
			mlibc::panicLogger() << "mlibc: sys_clock_get() failed with error code: " << e << frg::endlog;
		long long gone = (now.tv_sec - start.tv_sec) * 1000LL +
			(now.tv_nsec - start.tv_nsec) / 1000000;
		return gone >= ms ? 0 : static_cast<int>(ms - gone);
	}

	// The query q to the name server (resolv.conf's first, or 127.0.0.1)
	// and its reply into r (DNS_MAXUDP bytes), its length in *rlen: one
	// with the query's ID and question, from the server's address and
	// port. 0, or -EAI_AGAIN if none came, -EAI_SYSTEM.
	int exchange(const string &q, uint8_t *r, size_t *rlen) {
		struct sockaddr_in sin = {};
		sin.sin_family = AF_INET;
		sin.sin_port = htons(53);

		mlibc::service_result serv_buf{getAllocator()};
		if (mlibc::lookup_serv_by_name(serv_buf, "domain", IPPROTO_UDP, SOCK_DGRAM, 0) > 0)
			sin.sin_port = htons(serv_buf[0].port);

		auto nameserver = get_nameserver();
		if (!inet_aton(nameserver ? nameserver->name.data() : "127.0.0.1", &sin.sin_addr))
			return -EAI_SYSTEM;

		int fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (fd < 0)
			return -EAI_SYSTEM;
		frg::scope_exit close_fd{[&] {
			close(fd);
		}};

		for (int t = 0; t < DNS_TRIES; t++) {
			if (sendto(fd, q.data(), q.size(), 0, reinterpret_cast<struct sockaddr *>(&sin),
					sizeof(sin)) != static_cast<ssize_t>(q.size()))
				return -EAI_SYSTEM;

			struct timespec start;
			if (int e = mlibc::sysdep<ClockGet>(CLOCK_MONOTONIC, &start.tv_sec, &start.tv_nsec); e)
				mlibc::panicLogger() << "mlibc: sys_clock_get() failed with error code: " << e << frg::endlog;
			int left;
			while ((left = time_left(start, DNS_TIMEOUT_MS)) > 0) {
				struct pollfd pfd = {fd, POLLIN, 0};
				int ready = poll(&pfd, 1, left);
				if (ready < 0 && errno != EINTR)
					return -EAI_SYSTEM;
				if (ready <= 0)
					continue;

				struct sockaddr_in from = {};
				socklen_t fromlen = sizeof(from);
				ssize_t n = recvfrom(fd, r, DNS_MAXUDP, 0,
						reinterpret_cast<struct sockaddr *>(&from), &fromlen);
				if (n < 0) {
					if (errno == EINTR || errno == EAGAIN)
						continue;
					if (errno == ECONNREFUSED)
						break;	// nobody there: try again, then give up
					return -EAI_SYSTEM;
				}
				// Not the server's, or not the reply to this query: ignored.
				if (from.sin_addr.s_addr != sin.sin_addr.s_addr ||
						from.sin_port != sin.sin_port)
					continue;
				if (static_cast<size_t>(n) < DNS_HEADER || r[0] != static_cast<uint8_t>(q[0]) ||
						r[1] != static_cast<uint8_t>(q[1]) || !(get16(r + 2) & FLAG_QR) ||
						get16(r + 4) != 1 || !same_question(r, n, q))
					continue;
				*rlen = static_cast<size_t>(n);
				return 0;
			}
		}
		return -EAI_AGAIN;
	}

	// The reply's answers, from just after its one question: fn(type,
	// owner, rdata offset, rdata length) for each of class IN. 0, or
	// -EAI_NONAME for NXDOMAIN, -EAI_AGAIN for SERVFAIL or a truncated
	// reply with no answers, -EAI_FAIL for another error or a malformed
	// reply.
	template<typename F>
	int each_answer(const uint8_t *r, size_t rlen, F fn) {
		uint16_t flags = get16(r + 2);
		unsigned int rcode = flags & 0xF;
		if (rcode == RCODE_NXDOMAIN)
			return -EAI_NONAME;
		if (rcode == RCODE_SERVFAIL)
			return -EAI_AGAIN;
		if (rcode != 0)
			return -EAI_FAIL;

		unsigned int ancount = get16(r + 6);
		if ((flags & FLAG_TC) && ancount == 0)
			return -EAI_AGAIN;
		long off = read_name(r, rlen, DNS_HEADER, nullptr);
		if (off < 0 || static_cast<size_t>(off) + 4 > rlen)
			return -EAI_FAIL;
		off += 4;

		string owner{getAllocator()};
		for (unsigned int i = 0; i < ancount; i++) {
			off = read_name(r, rlen, static_cast<size_t>(off), &owner);
			if (off < 0 || static_cast<size_t>(off) + 10 > rlen)
				return (flags & FLAG_TC) ? 0 : -EAI_FAIL;
			const uint8_t *rr = r + off;
			unsigned int type = get16(rr), cls = get16(rr + 2), rdlen = get16(rr + 8);
			size_t rdata = static_cast<size_t>(off) + 10;
			if (rdata + rdlen > rlen)
				return (flags & FLAG_TC) ? 0 : -EAI_FAIL;
			if (cls == CLASS_IN)
				fn(type, owner, rdata, rdlen);
			off = static_cast<long>(rdata + rdlen);
		}
		return 0;
	}
} // namespace

int lookup_name_dns(struct lookup_result &buf, const char *name,
		frg::string<MemoryAllocator> &canon_name, int family) {
	uint16_t qtype = family == AF_INET6 ? RECORD_AAAA : RECORD_A;
	string q{getAllocator()};
	if (!make_query(q, new_id(), name, qtype))
		return -EAI_NONAME;

	uint8_t r[DNS_MAXUDP];
	size_t rlen;
	if (int e = exchange(q, r, &rlen); e)
		return e;

	int e = each_answer(r, rlen, [&] (unsigned int type, const string &owner,
			size_t rdata, unsigned int rdlen) {
		if (type == RECORD_CNAME) {
			string target{getAllocator()};
			if (read_name(r, rlen, rdata, &target) >= 0) {
				buf.aliases.push(owner);
				canon_name = std::move(target);
			}
			return;
		}
		if ((type != RECORD_A || rdlen != 4) && (type != RECORD_AAAA || rdlen != 16))
			return;
		if (type != qtype)
			return;
		struct dns_addr_buf a;
		memcpy(a.addr, r + rdata, rdlen);
		a.family = type == RECORD_A ? AF_INET : AF_INET6;
		a.name = owner;
		buf.buf.push(std::move(a));
	});
	if (e)
		return e;
	return buf.buf.size();
}

int lookup_addr_dns(frg::span<char> name, frg::array<uint8_t, 16> &addr, int family) {
	// The address's name under in-addr.arpa or ip6.arpa: its bytes, or
	// its nibbles, last first.
	char rev[80];
	size_t n = 0;
	if (family == AF_INET) {
		for (int i = 3; i >= 0; i--)
			n += snprintf(rev + n, sizeof(rev) - n, "%u.", addr[i]);
		snprintf(rev + n, sizeof(rev) - n, "in-addr.arpa");
	} else if (family == AF_INET6) {
		static const char hex[] = "0123456789abcdef";
		for (int i = 15; i >= 0; i--) {
			rev[n++] = hex[addr[i] & 0xF];
			rev[n++] = '.';
			rev[n++] = hex[addr[i] >> 4];
			rev[n++] = '.';
		}
		snprintf(rev + n, sizeof(rev) - n, "ip6.arpa");
	} else {
		return -EAI_FAMILY;
	}

	string q{getAllocator()};
	if (!make_query(q, new_id(), rev, RECORD_PTR))
		return -EAI_FAIL;

	uint8_t r[DNS_MAXUDP];
	size_t rlen;
	if (int e = exchange(q, r, &rlen); e)
		return e;

	string ptr{getAllocator()};
	int found = 0;
	int e = each_answer(r, rlen, [&] (unsigned int type, const string &,
			size_t rdata, unsigned int) {
		if (type == RECORD_PTR && !found && read_name(r, rlen, rdata, &ptr) >= 0)
			found = 1;
	});
	if (e == -EAI_NONAME)
		return 0;
	if (e)
		return e;
	if (!found)
		return 0;
	if (ptr.size() >= name.size())
		return -EAI_OVERFLOW;
	memcpy(name.data(), ptr.data(), ptr.size());
	name.data()[ptr.size()] = '\0';
	return 1;
}

int lookup_name_hosts(struct lookup_result &buf, const char *name,
		frg::string<MemoryAllocator> &canon_name, int family) {
	auto file = fopen("/etc/hosts", "r");
	if (!file) {
		switch (errno) {
			case ENOENT:
			case ENOTDIR:
			case EACCES:
				return -EAI_SERVICE;
			default:
				return -EAI_SYSTEM;
		}
	}

	char line[128];
	int name_length = strlen(name);
	while (fgets(line, 128, file)) {
		char *pos;
		// same way to deal with comments as in services.cpp
		if ((pos = strchr(line, '#'))) {
			*pos++ = '\n';
			*pos = '\0';
		}

		for(pos = line + 1; (pos = strstr(pos, name)) &&
				(!isspace(pos[-1]) || !isspace(pos[name_length])); pos++);
		if (!pos)
			continue;

		for (pos = line; !isspace(*pos); pos++);
		*pos = '\0';

		struct dns_addr_buf buffer;

		if ((family == AF_UNSPEC || family == AF_INET) && inet_pton(AF_INET, line, buffer.addr)) {
			buffer.family = AF_INET;
		} else if((family == AF_UNSPEC || family == AF_INET6) && inet_pton(AF_INET6, line, buffer.addr)) {
			buffer.family = AF_INET6;
		} else {
			continue; // not a valid address
		}

		pos++;
		for(; *pos && isspace(*pos); pos++);
		char *end;
		for(end = pos; *end && !isspace(*end); end++);

		buffer.name = frg::string<MemoryAllocator>{pos,
			static_cast<size_t>(end - pos), getAllocator()};
		canon_name = buffer.name;

		buf.buf.push(std::move(buffer));

		pos = end;
		while (pos[1]) {
			for (; *pos && isspace(*pos); pos++);
			for (end = pos; *end && !isspace(*end); end++);
			auto name = frg::string<MemoryAllocator>{pos,
				static_cast<size_t>(end - pos), getAllocator()};
			buf.aliases.push(std::move(name));
			pos = end;
		}
	}

	fclose(file);
	return buf.buf.size();
}

int lookup_addr_hosts(frg::span<char> name, frg::array<uint8_t, 16> &addr, int family) {
	auto file = fopen("/etc/hosts", "r");
	if (!file) {
		switch (errno) {
			case ENOENT:
			case ENOTDIR:
			case EACCES:
				return -EAI_SERVICE;
			default:
				return -EAI_SYSTEM;
		}
	}

	// Buffer to hold ASCII version of address
	char addr_str[64];
	if(!inet_ntop(family, addr.data(), addr_str, sizeof(addr_str))) {
		switch(errno) {
			case EAFNOSUPPORT:
				return -EAI_FAMILY;
			case ENOSPC:
				return -EAI_OVERFLOW;
			default:
				return -EAI_FAIL;
		}
	}
	int addr_str_len = strlen(addr_str);

	char line[128];
	while (fgets(line, 128, file)) {
		char *pos;
		// same way to deal with comments as in services.cpp
		if ((pos = strchr(line, '#'))) {
			*pos++ = '\n';
			*pos = '\0';
		}
		if (strncmp(line, addr_str, addr_str_len))
			continue;

		for (pos = line + addr_str_len + 1; isspace(*pos); pos++);
		char *begin = pos;
		for (; !isspace(*pos); pos++);
		char *end = pos;

		size_t size = end - begin;
		if (size >= name.size())
			return -EAI_OVERFLOW;
		std::copy(begin, end, name.data());
		name.data()[size] = '\0';
		return 1;
	}
	return 0;
}

int lookup_name_null(struct lookup_result &buf, int flags, int family) {
	if (flags & AI_PASSIVE) {
		if (family != AF_INET6) {
			struct dns_addr_buf addr_buf;
			addr_buf.family = AF_INET;

			in_addr_t addr = INADDR_ANY;
			memcpy(&addr_buf.addr, &addr, 4);

			buf.buf.push_back(addr_buf);
		}
		if (family != AF_INET) {
			struct dns_addr_buf addr_buf;
			addr_buf.family = AF_INET6;

			struct in6_addr addr = IN6ADDR_ANY_INIT;
			memcpy(&addr_buf.addr, &addr, 16);

			buf.buf.push_back(addr_buf);
		}
	} else {
		if (family != AF_INET6) {
			struct dns_addr_buf addr_buf;
			addr_buf.family = AF_INET;

			in_addr_t addr = INADDR_LOOPBACK;
			memcpy(&addr_buf.addr, &addr, 4);

			buf.buf.push_back(addr_buf);
		}
		if (family != AF_INET) {
			struct dns_addr_buf addr_buf;
			addr_buf.family = AF_INET6;

			struct in6_addr addr = IN6ADDR_LOOPBACK_INIT;
			memcpy(&addr_buf.addr, &addr, 16);

			buf.buf.push_back(addr_buf);
		}
	}
	return buf.buf.size();
}

int lookup_name_ip(struct lookup_result &buf, const char *name, int family) {
	if (family == AF_INET) {
		in_addr_t addr = 0;
		int res = inet_pton(AF_INET, name, &addr);

		if (res <= 0)
			return -EAI_NONAME;

		struct dns_addr_buf addr_buf;
		addr_buf.family = AF_INET;
		memcpy(&addr_buf.addr, &addr, 4);

		buf.buf.push_back(addr_buf);
		return 1;
	}

	if (family == AF_INET6) {
		struct in6_addr addr{};
		int res = inet_pton(AF_INET6, name, &addr);

		if (res <= 0)
			return -EAI_NONAME;

		struct dns_addr_buf addr_buf;
		addr_buf.family = AF_INET6;
		memcpy(&addr_buf.addr, &addr, 16);

		buf.buf.push_back(addr_buf);
		return 1;
	}

	// If no family was specified we try ipv4 and then ipv6.
	in_addr_t addr4 = 0;
	int res = inet_pton(AF_INET, name, &addr4);

	if (res > 0) {
		struct dns_addr_buf addr_buf;
		addr_buf.family = AF_INET;
		memcpy(&addr_buf.addr, &addr4, 4);

		buf.buf.push_back(addr_buf);
		return 1;
	}

	struct in6_addr addr6{};
	res = inet_pton(AF_INET6, name, &addr6);

	if (res <= 0)
		return -EAI_NONAME;

	struct dns_addr_buf addr_buf;
	addr_buf.family = AF_INET6;
	memcpy(&addr_buf.addr, &addr6, 16);

	buf.buf.push_back(addr_buf);
	return 1;
}

} // namespace mlibc
