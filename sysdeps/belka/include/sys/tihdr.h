#ifndef _SYS_TIHDR_H
#define _SYS_TIHDR_H

/*
 * The Transport Provider Interface, as Belka's kernel has it
 * (<sys/tihdr.h> there): the messages a program exchanges by putmsg and
 * getmsg with a transport provider such as /dev/udp or /dev/tcp, each a
 * struct below at the start of the control part, addresses after it at
 * the offsets given. Fields are 32 bits wide.
 */

#include <stdint.h>

typedef int32_t t_scalar_t;
typedef uint32_t t_uscalar_t;

/* Requests, from the user. */
#define T_CONN_REQ 0
#define T_CONN_RES 1
#define T_DISCON_REQ 2
#define T_DATA_REQ 3
#define T_EXDATA_REQ 4
#define T_INFO_REQ 5
#define T_BIND_REQ 6
#define T_UNBIND_REQ 7
#define T_UNITDATA_REQ 8
#define T_OPTMGMT_REQ 9
#define T_ORDREL_REQ 10

/* From the provider. */
#define T_CONN_IND 11
#define T_CONN_CON 12
#define T_DISCON_IND 13
#define T_DATA_IND 14
#define T_EXDATA_IND 15
#define T_INFO_ACK 16
#define T_BIND_ACK 17
#define T_ERROR_ACK 18
#define T_OK_ACK 19
#define T_UNITDATA_IND 20
#define T_UDERROR_IND 21
#define T_OPTMGMT_ACK 22
#define T_ORDREL_IND 23

/* Belka has these as Solaris does: addresses, and the acceptor's id. */
#define T_ADDR_REQ 25
#define T_ADDR_ACK 27
#define T_CAPABILITY_REQ 28
#define T_CAPABILITY_ACK 30

/* T_CAPABILITY_REQ's CAP_bits1: what to report. */
#define TC1_INFO 0x0001
#define TC1_ACCEPTOR_ID 0x0002

/* Kinds of service. */
#define T_COTS 1
#define T_COTS_ORD 2
#define T_CLTS 3

/* The interface's states. */
#define TS_UNBND 0
#define TS_WACK_BREQ 1
#define TS_WACK_UREQ 2
#define TS_IDLE 3
#define TS_WACK_OPTREQ 4
#define TS_WACK_CREQ 5
#define TS_WCON_CREQ 6
#define TS_WRES_CIND 7
#define TS_WACK_CRES 8
#define TS_DATA_XFER 9
#define TS_WIND_ORDREL 10
#define TS_WREQ_ORDREL 11
#define TS_WACK_DREQ6 12
#define TS_WACK_DREQ7 13
#define TS_WACK_DREQ9 14
#define TS_WACK_DREQ10 15
#define TS_WACK_DREQ11 16

/* T_ERROR_ACK's TLI_error. */
#define TBADADDR 1
#define TBADOPT 2
#define TACCES 3
#define TBADF 4
#define TNOADDR 5
#define TOUTSTATE 6
#define TBADSEQ 7
#define TSYSERR 8
#define TLOOK 9
#define TBADDATA 10
#define TBUFOVFLW 11
#define TFLOW 12
#define TNODATA 13
#define TNODIS 14
#define TNOUDERR 15
#define TBADFLAG 16
#define TNOREL 17
#define TNOTSUPPORT 18
#define TSTATECHNG 19
#define TADDRBUSY 23

/* T_INFO_ACK sizes: -1 unlimited, -2 not supported. */
#define T_INFINITE (-1)
#define T_INVALID (-2)

/* PROVIDER_flag. */
#define SENDZERO 0x0001  /* zero-length datagrams may be sent */

/* MGMT_flags, and an option's status. */
#define T_NEGOTIATE 0x0004
#define T_CHECK 0x0008
#define T_DEFAULT 0x0010
#define T_SUCCESS 0x0020
#define T_FAILURE 0x0040
#define T_CURRENT 0x0080
#define T_PARTSUCCESS 0x0100
#define T_READONLY 0x0200
#define T_NOTSUPPORT 0x0400

struct T_conn_req {
	t_scalar_t PRIM_type;
	t_scalar_t DEST_length;
	t_scalar_t DEST_offset;
	t_scalar_t OPT_length;
	t_scalar_t OPT_offset;
};

struct T_conn_res {
	t_scalar_t PRIM_type;
	t_uscalar_t ACCEPTOR_id;
	t_scalar_t OPT_length;
	t_scalar_t OPT_offset;
	t_scalar_t SEQ_number;
};

struct T_discon_req {
	t_scalar_t PRIM_type;
	t_scalar_t SEQ_number;
};

struct T_data_req {
	t_scalar_t PRIM_type;
	t_scalar_t MORE_flag;
};

struct T_ordrel_req {
	t_scalar_t PRIM_type;
};

struct T_conn_ind {
	t_scalar_t PRIM_type;
	t_scalar_t SRC_length;
	t_scalar_t SRC_offset;
	t_scalar_t OPT_length;
	t_scalar_t OPT_offset;
	t_scalar_t SEQ_number;
};

struct T_conn_con {
	t_scalar_t PRIM_type;
	t_scalar_t RES_length;
	t_scalar_t RES_offset;
	t_scalar_t OPT_length;
	t_scalar_t OPT_offset;
};

struct T_discon_ind {
	t_scalar_t PRIM_type;
	t_scalar_t DISCON_reason;  /* an errno */
	t_scalar_t SEQ_number;
};

struct T_ordrel_ind {
	t_scalar_t PRIM_type;
};

struct T_addr_req {
	t_scalar_t PRIM_type;
};

struct T_addr_ack {
	t_scalar_t PRIM_type;
	t_scalar_t LOCADDR_length;
	t_scalar_t LOCADDR_offset;
	t_scalar_t REMADDR_length;
	t_scalar_t REMADDR_offset;
};

struct T_info_req {
	t_scalar_t PRIM_type;
};

struct T_info_ack {
	t_scalar_t PRIM_type;
	t_scalar_t TSDU_size;
	t_scalar_t ETSDU_size;
	t_scalar_t CDATA_size;
	t_scalar_t DDATA_size;
	t_scalar_t ADDR_size;
	t_scalar_t OPT_size;
	t_scalar_t TIDU_size;
	t_scalar_t SERV_type;
	t_scalar_t CURRENT_state;
	t_scalar_t PROVIDER_flag;
};

struct T_bind_req {
	t_scalar_t PRIM_type;
	t_scalar_t ADDR_length;
	t_scalar_t ADDR_offset;
	t_uscalar_t CONIND_number;
};

struct T_bind_ack {
	t_scalar_t PRIM_type;
	t_scalar_t ADDR_length;
	t_scalar_t ADDR_offset;
	t_uscalar_t CONIND_number;
};

struct T_unbind_req {
	t_scalar_t PRIM_type;
};

struct T_ok_ack {
	t_scalar_t PRIM_type;
	t_scalar_t CORRECT_prim;
};

struct T_error_ack {
	t_scalar_t PRIM_type;
	t_scalar_t ERROR_prim;
	t_scalar_t TLI_error;
	t_scalar_t UNIX_error;
};

struct T_unitdata_req {
	t_scalar_t PRIM_type;
	t_scalar_t DEST_length;
	t_scalar_t DEST_offset;
	t_scalar_t OPT_length;
	t_scalar_t OPT_offset;
};

struct T_unitdata_ind {
	t_scalar_t PRIM_type;
	t_scalar_t SRC_length;
	t_scalar_t SRC_offset;
	t_scalar_t OPT_length;
	t_scalar_t OPT_offset;
};

struct T_uderror_ind {
	t_scalar_t PRIM_type;
	t_scalar_t DEST_length;
	t_scalar_t DEST_offset;
	t_scalar_t OPT_length;
	t_scalar_t OPT_offset;
	t_scalar_t ERROR_type;     /* an errno */
};

struct T_capability_req {
	t_scalar_t PRIM_type;
	t_uscalar_t CAP_bits1;
};

struct T_capability_ack {
	t_scalar_t PRIM_type;
	t_uscalar_t CAP_bits1;
	struct T_info_ack INFO_ack;
	t_uscalar_t ACCEPTOR_id;
};

/*
 * T_OPTMGMT_REQ and T_OPTMGMT_ACK: options after them, each a t_opthdr
 * and its value, len bytes in all, the next at len rounded up to a
 * multiple of 4. The levels and names are the sockets' (SOL_SOCKET,
 * IPPROTO_TCP). MGMT_flags asks T_NEGOTIATE (set these values) or
 * T_CURRENT (report them: headers alone); each option comes back with
 * its value and a status, and the acknowledgement's MGMT_flags is the
 * worst of them.
 */
struct T_optmgmt_req {
	t_scalar_t PRIM_type;
	t_scalar_t OPT_length;
	t_scalar_t OPT_offset;
	t_scalar_t MGMT_flags;
};

struct T_optmgmt_ack {
	t_scalar_t PRIM_type;
	t_scalar_t OPT_length;
	t_scalar_t OPT_offset;
	t_scalar_t MGMT_flags;
};

struct t_opthdr {
	t_uscalar_t len;	/* this header and the value */
	t_uscalar_t level;
	t_uscalar_t name;
	t_uscalar_t status;
};

union T_primitives {
	t_scalar_t type;
	struct T_conn_req conn_req;
	struct T_conn_res conn_res;
	struct T_discon_req discon_req;
	struct T_data_req data_req;
	struct T_ordrel_req ordrel_req;
	struct T_conn_ind conn_ind;
	struct T_conn_con conn_con;
	struct T_discon_ind discon_ind;
	struct T_ordrel_ind ordrel_ind;
	struct T_addr_req addr_req;
	struct T_addr_ack addr_ack;
	struct T_capability_req capability_req;
	struct T_capability_ack capability_ack;
	struct T_info_req info_req;
	struct T_info_ack info_ack;
	struct T_bind_req bind_req;
	struct T_bind_ack bind_ack;
	struct T_unbind_req unbind_req;
	struct T_ok_ack ok_ack;
	struct T_error_ack error_ack;
	struct T_unitdata_req unitdata_req;
	struct T_unitdata_ind unitdata_ind;
	struct T_uderror_ind uderror_ind;
	struct T_optmgmt_req optmgmt_req;
	struct T_optmgmt_ack optmgmt_ack;
};

#endif /* _SYS_TIHDR_H */
