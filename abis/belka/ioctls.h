#ifndef _ABIBITS_IOCTLS_H
#define _ABIBITS_IOCTLS_H

/* System V's terminal and file ioctls, as Belka's kernel takes them. */
#define TCGETA 0x5401
#define TCSETA 0x5402
#define TCSETAW 0x5403
#define TCSETAF 0x5404
#define TCSBRK 0x5405
#define TCXONC 0x5406
#define TCFLSH 0x5407
#define TCGETS 0x540D
#define TCSETS 0x540E
#define TCSETSW 0x540F
#define TCSETSF 0x5410
#define TIOCSWINSZ 0x5467
#define TIOCGWINSZ 0x5468

#define TIOCGPGRP 0x7414
#define TIOCSPGRP 0x7415
#define TIOCGSID 0x7416
#define TIOCSCTTY 0x7484
#define TIOCSTI 0x7417
#define TIOCMSET 0x741A
#define TIOCMBIS 0x741B
#define TIOCMBIC 0x741C
#define TIOCMGET 0x741D

#define FIOCLEX 0x20006601
#define FIONCLEX 0x20006602
#define FIONBIO 0x8004667E
#define FIONREAD 0x4004667F

#endif /* _ABIBITS_IOCTLS_H */
