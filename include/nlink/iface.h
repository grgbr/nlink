#ifndef _NLINK_IFACE_H
#define _NLINK_IFACE_H

#include <nlink/nlink.h>
#include <libmnl/libmnl.h>
#include <linux/rtnetlink.h>

struct ether_addr;

struct nlink_iface {
	unsigned short           type;
	int                      index;
	uint8_t                  admin_state;
	const struct ether_addr *ucast_hwaddr;
	const struct ether_addr *bcast_hwaddr;
	const char              *name;
	size_t                   name_len;
	uint32_t                 mtu;
	uint32_t                 link;
	uint32_t                 master;
	uint8_t                  oper_state;
	uint32_t                 group;
	uint32_t                 promisc;
	uint8_t                  carrier_state;
};

static inline bool
nlink_iface_msg_isempty(const struct nlmsghdr *msg)
{
	nlink_assert(msg);

	return (mnl_nlmsg_get_payload_len(msg) <= sizeof(struct ifinfomsg));
}

extern int
nlink_iface_parse_msg(const struct nlmsghdr *msg, struct nlink_iface *iface);

extern int
nlink_iface_setup_msg_ucast_hwaddr(struct nlmsghdr         *msg,
                                   const struct ether_addr *hwaddr);

extern int
nlink_iface_setup_msg_name(struct nlmsghdr *msg, const char *name, size_t len);

extern int
nlink_iface_setup_msg_admin_state(struct nlmsghdr *msg, uint8_t admin_state);

extern int
nlink_iface_setup_msg_mtu(struct nlmsghdr *msg, uint32_t mtu);

extern void
nlink_iface_setup_new(struct nlmsghdr   *msg,
                      struct nlink_sock *sock,
                      unsigned short     type,
                      int                index);

#define NLINK_IFACE_DUMP_MSG_SIZE \
	(MNL_NLMSG_HDRLEN + sizeof(struct ifinfomsg))

extern void
nlink_iface_setup_dump(struct nlmsghdr   *msg,
                       struct nlink_sock *sock);

#endif /* _NLINK_IFACE_H */
