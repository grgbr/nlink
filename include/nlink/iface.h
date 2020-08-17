#ifndef _NLINK_IFACE_H
#define _NLINK_IFACE_H

#include <nlink/nlink.h>

struct nlmsghdr;
struct ether_addr;

extern int
nlink_iface_setup_msg_name(struct nlmsghdr *msg, const char *name, size_t len);

extern int
nlink_iface_setup_msg_addr(struct nlmsghdr *msg, const struct ether_addr *addr);

extern int
nlink_iface_setup_msg_mtu(struct nlmsghdr *msg, uint32_t mtu);

extern int
nlink_iface_setup_msg_oper_state(struct nlmsghdr *msg, uint8_t oper_state);

extern void
nlink_iface_setup_msg(struct nlmsghdr   *msg,
                      struct nlink_sock *sock,
                      unsigned short     iftype,
                      int                ifid);

#endif /* _NLINK_IFACE_H */
