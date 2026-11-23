#ifndef _SYS_DLPI_H
#define _SYS_DLPI_H

/*
 * The Data Link Provider Interface, version 2, as Belka's kernel has it
 * (<sys/dlpi.h> there): the primitives, their values and layouts.
 */

#include <stdint.h>

typedef uint32_t t_uscalar_t;
typedef int32_t t_scalar_t;

#define DL_VERSION_2 0x02

/* Primitives. */
#define DL_INFO_REQ 0x00
#define DL_BIND_REQ 0x01
#define DL_UNBIND_REQ 0x02
#define DL_INFO_ACK 0x03
#define DL_BIND_ACK 0x04
#define DL_ERROR_ACK 0x05
#define DL_OK_ACK 0x06
#define DL_UNITDATA_REQ 0x07
#define DL_UNITDATA_IND 0x08
#define DL_UDERROR_IND 0x09
#define DL_UDQOS_REQ 0x0a
#define DL_ATTACH_REQ 0x0b
#define DL_DETACH_REQ 0x0c
#define DL_SUBS_UNBIND_REQ 0x15
#define DL_SUBS_BIND_REQ 0x1b
#define DL_SUBS_BIND_ACK 0x1c
#define DL_ENABMULTI_REQ 0x1d
#define DL_DISABMULTI_REQ 0x1e
#define DL_PROMISCON_REQ 0x1f
#define DL_PROMISCOFF_REQ 0x20
#define DL_PHYS_ADDR_REQ 0x31
#define DL_PHYS_ADDR_ACK 0x32
#define DL_SET_PHYS_ADDR_REQ 0x33
#define DL_GET_STATISTICS_REQ 0x34
#define DL_GET_STATISTICS_ACK 0x35

/* DL_ERROR_ACK's dl_errno. */
#define DL_BADSAP 0x00
#define DL_BADADDR 0x01
#define DL_ACCESS 0x02
#define DL_OUTSTATE 0x03
#define DL_SYSERR 0x04
#define DL_BADCORR 0x05
#define DL_BADDATA 0x06
#define DL_UNSUPPORTED 0x07
#define DL_BADPPA 0x08
#define DL_BADPRIM 0x09
#define DL_BADQOSPARAM 0x0a
#define DL_BADQOSTYPE 0x0b
#define DL_BADTOKEN 0x0c
#define DL_BOUND 0x0d
#define DL_INITFAILED 0x0e
#define DL_NOADDR 0x0f
#define DL_NOTINIT 0x10
#define DL_UNDELIVERABLE 0x11
#define DL_NOTSUPPORTED 0x12
#define DL_TOOMANY 0x13
#define DL_NOTENAB 0x14
#define DL_BUSY 0x15

/* States. */
#define DL_UNBOUND 0x00
#define DL_BIND_PENDING 0x01
#define DL_UNBIND_PENDING 0x02
#define DL_IDLE 0x03
#define DL_UNATTACHED 0x04
#define DL_ATTACH_PENDING 0x05
#define DL_DETACH_PENDING 0x06

/* Media. */
#define DL_CSMACD 0x00
#define DL_ETHER 0x04

/* Service modes, provider styles. */
#define DL_CODLS 0x01
#define DL_CLDLS 0x02
#define DL_STYLE1 0x0500
#define DL_STYLE2 0x0501

/* Promiscuous levels. */
#define DL_PROMISC_PHYS 0x01
#define DL_PROMISC_SAP 0x02
#define DL_PROMISC_MULTI 0x03

/* DL_PHYS_ADDR_REQ's types. */
#define DL_FACT_PHYS_ADDR 0x01
#define DL_CURR_PHYS_ADDR 0x02

typedef struct {
	t_uscalar_t dl_primitive;
} dl_info_req_t;

typedef struct {
	t_uscalar_t dl_primitive;
	t_uscalar_t dl_max_sdu;
	t_uscalar_t dl_min_sdu;
	t_uscalar_t dl_addr_length;
	t_uscalar_t dl_mac_type;
	t_uscalar_t dl_reserved;
	t_uscalar_t dl_current_state;
	t_scalar_t dl_sap_length;
	t_uscalar_t dl_service_mode;
	t_uscalar_t dl_qos_length;
	t_uscalar_t dl_qos_offset;
	t_uscalar_t dl_qos_range_length;
	t_uscalar_t dl_qos_range_offset;
	t_uscalar_t dl_provider_style;
	t_uscalar_t dl_addr_offset;
	t_uscalar_t dl_version;
	t_uscalar_t dl_brdcst_addr_length;
	t_uscalar_t dl_brdcst_addr_offset;
	t_uscalar_t dl_growth;
} dl_info_ack_t;

typedef struct {
	t_uscalar_t dl_primitive;
	t_uscalar_t dl_ppa;
} dl_attach_req_t;

typedef struct {
	t_uscalar_t dl_primitive;
	t_uscalar_t dl_sap;
	t_uscalar_t dl_max_conind;
	uint16_t	dl_service_mode;
	uint16_t	dl_conn_mgmt;
	t_uscalar_t dl_xidtest_flg;
} dl_bind_req_t;

typedef struct {
	t_uscalar_t dl_primitive;
	t_uscalar_t dl_sap;
	t_uscalar_t dl_addr_length;
	t_uscalar_t dl_addr_offset;
	t_uscalar_t dl_max_conind;
	t_uscalar_t dl_xidtest_flg;
} dl_bind_ack_t;

typedef struct {
	t_uscalar_t dl_primitive;
} dl_unbind_req_t;

typedef struct {
	t_uscalar_t dl_primitive;
	t_uscalar_t dl_correct_primitive;
} dl_ok_ack_t;

typedef struct {
	t_uscalar_t dl_primitive;
	t_uscalar_t dl_error_primitive;
	t_uscalar_t dl_errno;
	t_uscalar_t dl_unix_errno;
} dl_error_ack_t;

typedef struct {
	t_scalar_t dl_min;
	t_scalar_t dl_max;
} dl_priority_t;

typedef struct {
	t_uscalar_t dl_primitive;
	t_uscalar_t dl_dest_addr_length;
	t_uscalar_t dl_dest_addr_offset;
	dl_priority_t dl_priority;
} dl_unitdata_req_t;

typedef struct {
	t_uscalar_t dl_primitive;
	t_uscalar_t dl_dest_addr_length;
	t_uscalar_t dl_dest_addr_offset;
	t_uscalar_t dl_src_addr_length;
	t_uscalar_t dl_src_addr_offset;
	t_uscalar_t dl_group_address;
} dl_unitdata_ind_t;

typedef struct {
	t_uscalar_t dl_primitive;
	t_uscalar_t dl_dest_addr_length;
	t_uscalar_t dl_dest_addr_offset;
	t_uscalar_t dl_unix_errno;
	t_uscalar_t dl_errno;
} dl_uderror_ind_t;

typedef struct {
	t_uscalar_t dl_primitive;
	t_uscalar_t dl_level;
} dl_promiscon_req_t;

typedef struct {
	t_uscalar_t dl_primitive;
	t_uscalar_t dl_level;
} dl_promiscoff_req_t;

typedef struct {
	t_uscalar_t dl_primitive;
	t_uscalar_t dl_addr_type;
} dl_phys_addr_req_t;

typedef struct {
	t_uscalar_t dl_primitive;
	t_uscalar_t dl_addr_length;
	t_uscalar_t dl_addr_offset;
} dl_phys_addr_ack_t;

#define DL_INFO_REQ_SIZE sizeof(dl_info_req_t)
#define DL_INFO_ACK_SIZE sizeof(dl_info_ack_t)
#define DL_ATTACH_REQ_SIZE sizeof(dl_attach_req_t)
#define DL_BIND_REQ_SIZE sizeof(dl_bind_req_t)
#define DL_BIND_ACK_SIZE sizeof(dl_bind_ack_t)
#define DL_UNBIND_REQ_SIZE sizeof(dl_unbind_req_t)
#define DL_OK_ACK_SIZE sizeof(dl_ok_ack_t)
#define DL_ERROR_ACK_SIZE sizeof(dl_error_ack_t)
#define DL_UNITDATA_REQ_SIZE sizeof(dl_unitdata_req_t)
#define DL_UNITDATA_IND_SIZE sizeof(dl_unitdata_ind_t)
#define DL_UDERROR_IND_SIZE sizeof(dl_uderror_ind_t)
#define DL_PROMISCON_REQ_SIZE sizeof(dl_promiscon_req_t)
#define DL_PROMISCOFF_REQ_SIZE sizeof(dl_promiscoff_req_t)
#define DL_PHYS_ADDR_REQ_SIZE sizeof(dl_phys_addr_req_t)
#define DL_PHYS_ADDR_ACK_SIZE sizeof(dl_phys_addr_ack_t)

#endif /* _SYS_DLPI_H */
