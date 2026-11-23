sysdep_supported_options = {
	'posix': true,
	'posix-aio': false,
	'glibc': true,
	'bsd': true,
}

# No GNU IFUNCs: static programs are linked by linkers that do not
# support them (the illumos link-editor), and resolve no IRELATIVE
# relocations. The generic string functions are used instead.
internal_conf.set10('MLIBC_IFUNCS_SUPPORTED', false)

# getopt() as POSIX and System V have it: it stops at the first operand,
# rather than moving later options before it as glibc's does (BSD and
# System V programs, find above all, take what follows an operand as
# their own). getopt_long keeps GNU's permutation.
internal_conf.set10('MLIBC_POSIX_GETOPT', true)
internal_conf.set10('MLIBC_SPROC_OPTION', true)

rtld_include_dirs += include_directories('include')
libc_include_dirs += include_directories('include')

rtld_sources += files(
	'generic/generic.cpp',
)

libc_sources += files(
	'generic/aio.cpp',
	'generic/entry.cpp',
	'generic/generic.cpp',
	'generic/signal.cpp',
	'generic/cwd.cpp',
	'generic/mount.cpp',
	'generic/stropts.cpp',
	'generic/thread.cpp',
	'generic/sproc.cpp',
	host_machine.cpu_family() / 'sigtramp.S',
	host_machine.cpu_family() / 'thread.S',
	host_machine.cpu_family() / 'sproc.S',
)

# The ABI headers, installed from the files under abis/ rather than
# through the include/abi-bits links (which mlibc's own build uses), so
# that every meson implementation installs the header itself: muon
# copies a link as a link.
abi_belka_headers = [
	'errno', 'fcntl', 'ioctls', 'nlink_t', 'poll', 'resource',
	'seek-whence', 'sig-limits', 'signal', 'sigset_t', 'stat', 'statvfs',
	'termios', 'utmpx', 'utsname', 'vm-flags', 'wait',
]
abi_linux_headers = [
	'access', 'aio', 'auxv', 'blkcnt_t', 'blksize_t', 'clockid_t',
	'dev_t', 'fd_set', 'fsblkcnt_t', 'fsfilcnt_t', 'gid_t', 'in', 'ino_t',
	'ipc', 'limits', 'mode_t', 'mqueue', 'msg', 'pid_t',
	'riscv-hwprobe', 'rlim_t', 'route', 'sa_family_t',
	'sched_param', 'sem', 'shm', 'sigevent', 'sigval',
	'sockaddr_storage', 'socket', 'socklen_t', 'suseconds_t',
	'time', 'uid_t', 'utmp-defines',
]

if not no_headers
	abi_headers = []
	foreach h : abi_belka_headers
		abi_headers += '../../abis/belka/' + h + '.h'
	endforeach
	foreach h : abi_linux_headers
		abi_headers += '../../abis/linux/' + h + '.h'
	endforeach
	install_headers(abi_headers, subdir: 'abi-bits')
	install_headers('include/sys/dlpi.h', 'include/sys/ipconf.h',
		'include/sys/mount.h', 'include/sys/prctl.h',
		subdir: 'sys')
	install_headers('include/stropts.h')
endif

if not headers_only
	foreach crt : ['crt1', 'crti', 'crtn']
		custom_target(crt,
			build_by_default: true,
			command: c_compiler.cmd_array() + ['-c', '-o', '@OUTPUT@', '@INPUT@'],
			input: host_machine.cpu_family() / 'crt-src' / crt + '.S',
			output: crt + '.o',
			install: true,
			install_dir: get_option('libdir')
		)
	endforeach
endif
