sysdep_supported_options = {
	'posix': true,
	'glibc': true,
	'bsd': true,
}

# No GNU IFUNCs: static programs are linked by linkers that do not
# support them (the illumos link-editor), and resolve no IRELATIVE
# relocations. The generic string functions are used instead.
internal_conf.set10('MLIBC_IFUNCS_SUPPORTED', false)

rtld_include_dirs += include_directories('include')
libc_include_dirs += include_directories('include')

rtld_sources += files(
	'generic/generic.cpp',
)

libc_sources += files(
	'generic/entry.cpp',
	'generic/generic.cpp',
	'generic/signal.cpp',
	'generic/cwd.cpp',
	'generic/thread.cpp',
	host_machine.cpu_family() / 'sigtramp.S',
	host_machine.cpu_family() / 'thread.S',
)

# The ABI headers, installed from the files under abis/ rather than
# through the include/abi-bits links (which mlibc's own build uses), so
# that every meson implementation installs the header itself: muon
# copies a link as a link.
abi_belka_headers = [
	'errno', 'fcntl', 'nlink_t', 'seek-whence', 'signal', 'sigset_t',
	'stat', 'utsname', 'vm-flags', 'wait',
]
abi_linux_headers = [
	'access', 'aio', 'auxv', 'blkcnt_t', 'blksize_t', 'clockid_t',
	'dev_t', 'fd_set', 'fsblkcnt_t', 'fsfilcnt_t', 'gid_t', 'in', 'ino_t',
	'ioctls', 'ipc', 'limits', 'mode_t', 'mqueue', 'msg', 'pid_t', 'poll',
	'resource', 'riscv-hwprobe', 'rlim_t', 'route', 'sa_family_t',
	'sched_param', 'sem', 'shm', 'sig-limits', 'sigevent', 'sigval',
	'sockaddr_storage', 'socket', 'socklen_t', 'statvfs', 'suseconds_t',
	'termios', 'time', 'uid_t', 'utmp-defines', 'utmpx',
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
