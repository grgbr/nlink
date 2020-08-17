#include <nlink/iface.h>
#include "utils.h"
#include <string.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/rtnetlink.h>

////////////////////////////////////////////////////////////////////////////////

#if 0
int
nlink_parse_hwaddr_attr(const struct nlattr *attr, struct ether_addr *addr)
{
	rtnl_assert(attr);
	rtnl_assert(addr);

	int err;

	err = nlink_parse_binary_attr(attr);
	if (err)
		return err;

	if (mnl_attr_get_payload_len(attr) != sizeof(*addr))
		return -ERANGE;

	*addr = *(typeof(addr))mnl_attr_get_payload(attr);

	return 0;
}

int
nlink_parse_map_attr(const struct nlattr *attr, struct rtnl_link_ifmap *map)
{
	rtnl_assert(attr);
	rtnl_assert(map);

	uint16_t len;

	len = mnl_attr_get_payload_len(attr);
	if (len < sizeof(*map))
		return -ERANGE;

	if (len != sizeof(*map))
		rtnl_warn("unexpected IFLA map size (%hd != %zu)",
		          len,
		          sizeof(*map));

	*map = *(typeof(map))mnl_attr_get_payload(attr);

	return 0;
}

int
nlink_parse_stats_attr(const struct nlattr *attr, struct rtnl_link_stats *stats)
{
	rtnl_assert(attr);
	rtnl_assert(stats);

	uint16_t len;

	len = mnl_attr_get_payload_len(attr);
	if (len < sizeof(*stats))
		return -ERANGE;

	if (len != sizeof(*stats))
		rtnl_warn("unexpected IFLA stats size (%hd != %zu)",
		          len,
		          sizeof(*stats));

	*stats = *(typeof(stats))mnl_attr_get_payload(attr);

	return 0;
}

int
nlink_parse_stats64_attr(const struct nlattr      *attr,
                         struct rtnl_link_stats64 *stats)
{
	rtnl_assert(attr);
	rtnl_assert(stats);

	uint16_t len;

	len = mnl_attr_get_payload_len(attr);
	if (len < sizeof(*stats))
		return -ERANGE;

	if (len != sizeof(*stats))
		rtnl_warn("unexpected IFLA stats64 size (%hd != %zu)",
		          len,
		          sizeof(*stats));

	*stats = *(typeof(stats))mnl_attr_get_payload(attr);

	return 0;
}

ssize_t
nlink_parse_phys_port_id_attr(const struct nlattr *attr,
                             unsigned char        id[RTNL_PHYS_PORT_ID_LEN])
{
	rtnl_assert(attr);
	rtnl_assert(id);

	int    err;
	size_t len;

	err = ifla_parse_binary_attr(attr);
	if (err)
		return err;

	len = mnl_attr_get_payload_len(attr);
	if (len > RTNL_PHYS_PORT_ID_LEN)
		return -ERANGE;

	memset(id, 0, RTNL_PHYS_PORT_ID_LEN);
	memcpy(id, mnl_attr_get_payload(attr), len);

	return len;
}

#endif

////////////////////////////////////////////////////////////////////////////////

int
nlink_iface_setup_msg_name(struct nlmsghdr *msg, const char *name, size_t len)
{
	nlink_assert(msg);
	nlink_assert(msg->nlmsg_len >=
	             mnl_nlmsg_size(sizeof(struct ifinfomsg)));
	nlink_assert(name);
	nlink_assert(*name);
	nlink_assert(len);
	nlink_assert(len < IFNAMSIZ);
	nlink_assert(strnlen(name, IFNAMSIZ) == len);

	if (!mnl_attr_put_check(msg,
	                        NLINK_XFER_MSG_SIZE,
	                        IFLA_IFNAME,
	                        len,
	                        name))
		return -EMSGSIZE;

	return 0;
}

int
nlink_iface_setup_msg_addr(struct nlmsghdr *msg, const struct ether_addr *addr)
{
	nlink_assert(msg);
	nlink_assert(msg->nlmsg_len >=
	             mnl_nlmsg_size(sizeof(struct ifinfomsg)));
	nlink_assert(nwu_hwaddr_is_laa(addr));
	nlink_assert(nwu_hwaddr_is_ucast(addr));

	if (!mnl_attr_put_check(msg,
	                        NLINK_XFER_MSG_SIZE,
	                        IFLA_ADDRESS,
	                        sizeof(*addr),
	                        addr))
		return -EMSGSIZE;

	return 0;
}

int
nlink_iface_setup_msg_mtu(struct nlmsghdr *msg, uint32_t mtu)
{
	nlink_assert(msg);
	nlink_assert(msg->nlmsg_len >=
	             mnl_nlmsg_size(sizeof(struct ifinfomsg)));
	nlink_assert(mtu);
	nlink_assert(mtu <= IP_MAXPACKET);

	if (!mnl_attr_put_u32_check(msg,
	                            NLINK_XFER_MSG_SIZE,
	                            IFLA_MTU,
	                            mtu))
		return -EMSGSIZE;

	return 0;
}

int
nlink_iface_setup_msg_oper_state(struct nlmsghdr *msg, uint8_t oper_state)
{
	nlink_assert(msg);
	nlink_assert(msg->nlmsg_len >=
	             mnl_nlmsg_size(sizeof(struct ifinfomsg)));

	switch (oper_state) {
	case IF_OPER_UP:
	case IF_OPER_DOWN:
		break;

	default:
		nlink_assert(0);
	}

	if (!mnl_attr_put_u8_check(msg,
	                           NLINK_XFER_MSG_SIZE,
	                           IFLA_OPERSTATE,
	                           oper_state))
		return -EMSGSIZE;

	return 0;
}

void
nlink_iface_setup_msg(struct nlmsghdr   *msg,
                      struct nlink_sock *sock,
                      unsigned short     iftype,
                      int                ifid)
{
	nlink_assert(msg);
	nlink_assert(sock);
	nlink_assert(ifid > 0);
	nlink_assert(iftype != ARPHRD_VOID);
	nlink_assert(iftype != ARPHRD_NONE);

	struct ifinfomsg *info;

	mnl_nlmsg_put_header(msg);
	msg->nlmsg_type = RTM_NEWLINK;
	msg->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	msg->nlmsg_seq = nlink_alloc_seqno(sock);
	msg->nlmsg_pid = sock->port_id;

	info = mnl_nlmsg_put_extra_header(msg, sizeof(*info));
	info->ifi_family = AF_UNSPEC;
	info->ifi_type = iftype;
	info->ifi_index = ifid;
	info->ifi_flags = 0;
	info->ifi_change = 0;
}
