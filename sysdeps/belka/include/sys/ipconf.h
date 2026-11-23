#ifndef _SYS_IPCONF_H
#define _SYS_IPCONF_H

/*
 * Configuring IPv4 through a /dev/ip stream's I_STR ioctls, as Belka's
 * kernel has it (<sys/ipconf.h> there). Addresses are in network order.
 */

#include <stdint.h>

#define IPCONF_NAMELEN 16

#define IP_IFATTACH (('I' << 8) | 1)	/* struct ip_ifattach */
#define IP_IFADDR (('I' << 8) | 2)	/* struct ip_ifaddr */
#define IP_ROUTEADD (('I' << 8) | 3)	/* struct ip_route */
#define IP_ROUTEDEL (('I' << 8) | 4)	/* struct ip_route */
#define IP_IFLIST (('I' << 8) | 5)	/* struct ip_ifinfo[] */
#define IP_ROUTELIST (('I' << 8) | 6)	/* struct ip_route[] */
#define IP_ARPLIST (('I' << 8) | 7)	/* struct ip_arpinfo[] */
#define IP_BINDPROTO (('I' << 8) | 8)	/* int: the protocol */

/* An interface from two linked DLPI streams (their multiplexor ids). */
struct ip_ifattach {
	char ia_name[IPCONF_NAMELEN];
	int32_t ia_ipmux;
	int32_t ia_arpmux;
};

struct ip_ifaddr {
	char ifa_name[IPCONF_NAMELEN];
	uint32_t ifa_addr;
	uint32_t ifa_mask;
};

/* A route; rt_gateway 0: the destination is on the interface. */
struct ip_route {
	uint32_t rt_dst;
	uint32_t rt_mask;
	uint32_t rt_gateway;
	char rt_ifname[IPCONF_NAMELEN];      /* "": by gateway */
};

#define IPIF_UP 0x0001
#define IPIF_BROADCAST 0x0002
#define IPIF_LOOPBACK 0x0008
#define IPIF_RUNNING 0x0040

struct ip_ifinfo {
	char ii_name[IPCONF_NAMELEN];
	uint32_t ii_addr;
	uint32_t ii_mask;
	uint32_t ii_flags;
	uint32_t ii_mtu;
	uint8_t ii_hwaddr[6];
	uint8_t ii_hwlen;
	uint8_t ii_pad;
	uint64_t ii_ipackets;
	uint64_t ii_opackets;
	uint64_t ii_ierrors;
	uint64_t ii_drops;
};

#define IPARP_COMPLETE 0x0001

struct ip_arpinfo {
	char ai_ifname[IPCONF_NAMELEN];
	uint32_t ai_addr;
	uint8_t ai_hwaddr[6];
	uint16_t ai_flags;
	uint32_t ai_expire;      /* seconds left */
};

#endif /* _SYS_IPCONF_H */
