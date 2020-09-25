#include <nlink/iface.h>
#include "parse.h"
#include <utils/net.h>
#include <string.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/rtnetlink.h>

#define nlink_iface_assert_oper_state(_state) \
	nlink_assert(_state != IF_OPER_NOTPRESENT); \
	nlink_assert(_state != IF_OPER_TESTING)

static int
nlink_iface_parse_ucast_hwaddr(const struct nlattr *attr,
                               struct nlink_iface  *iface)
{
	nlink_assert(attr);
	nlink_assert(iface);

	iface->ucast_hwaddr = nlink_parse_hwaddr_attr(attr);

	return (iface->ucast_hwaddr) ? 0 : -errno;
}

static int
nlink_iface_parse_bcast_hwaddr(const struct nlattr *attr,
                               struct nlink_iface  *iface)
{
	nlink_assert(attr);
	nlink_assert(iface);

	iface->bcast_hwaddr = nlink_parse_hwaddr_attr(attr);

	return (iface->bcast_hwaddr) ? 0 : -errno;
}

static int
nlink_iface_parse_name(const struct nlattr *attr, struct nlink_iface *iface)
{
	nlink_assert(attr);
	nlink_assert(iface);

	ssize_t size;

	size = nlink_parse_string_attr(attr, &iface->name, IFNAMSIZ);
	if (size <= 0)
		return size ? size : -EBADMSG;

	iface->name_len = size - 1;

	return 0;
}

static int
nlink_iface_parse_mtu(const struct nlattr *attr, struct nlink_iface *iface)
{
	nlink_assert(attr);
	nlink_assert(iface);

	int err;

	err = nlink_parse_uint32_attr(attr, &iface->mtu);
	if (err)
		return err;

	nlink_assert(unet_mtu_isok(iface->mtu));

	return 0;
}

static int
nlink_iface_parse_link(const struct nlattr *attr, struct nlink_iface *iface)
{
	nlink_assert(attr);
	nlink_assert(iface);

	int err;

	err = nlink_parse_uint32_attr(attr, &iface->link);
	if (err)
		return err;

	nlink_assert(iface->link > 0);

	return 0;
}

static int
nlink_iface_parse_master(const struct nlattr *attr, struct nlink_iface *iface)
{
	nlink_assert(attr);
	nlink_assert(iface);

	int err;

	err = nlink_parse_uint32_attr(attr, &iface->master);
	if (err)
		return err;

	nlink_assert(iface->master > 0);

	return 0;
}

static int
nlink_iface_parse_oper_state(const struct nlattr *attr,
                             struct nlink_iface  *iface)
{
	nlink_assert(attr);
	nlink_assert(iface);

	int err;

	err = nlink_parse_uint8_attr(attr, &iface->oper_state);
	if (err)
		return err;

	nlink_iface_assert_oper_state(iface->oper_state);

	return 0;
}

static int
nlink_iface_parse_group(const struct nlattr *attr, struct nlink_iface *iface)
{
	nlink_assert(attr);
	nlink_assert(iface);

	return nlink_parse_uint32_attr(attr, &iface->group);
}

static int
nlink_iface_parse_promisc(const struct nlattr *attr, struct nlink_iface *iface)
{
	nlink_assert(attr);
	nlink_assert(iface);

	return nlink_parse_uint32_attr(attr, &iface->promisc);
}

static int
nlink_iface_parse_carrier(const struct nlattr *attr, struct nlink_iface *iface)
{
	nlink_assert(attr);
	nlink_assert(iface);

	int err;

	err = nlink_parse_uint8_attr(attr, &iface->carrier);
	if (err)
		return err;

	nlink_iface_assert_oper_state(iface->carrier);

	return 0;
}

typedef int (nlink_iface_parse_attr_fn)(const struct nlattr *attr,
                                        struct nlink_iface  *iface);

static nlink_iface_parse_attr_fn * const nlink_iface_attr_parsers[] = {
	[IFLA_ADDRESS]     = nlink_iface_parse_ucast_hwaddr,
	[IFLA_BROADCAST]   = nlink_iface_parse_bcast_hwaddr,
	[IFLA_IFNAME]      = nlink_iface_parse_name,
	[IFLA_MTU]         = nlink_iface_parse_mtu,
	[IFLA_LINK]        = nlink_iface_parse_link,
	[IFLA_MASTER]      = nlink_iface_parse_master,
	[IFLA_OPERSTATE]   = nlink_iface_parse_oper_state,
	[IFLA_GROUP]       = nlink_iface_parse_group,
	[IFLA_PROMISCUITY] = nlink_iface_parse_promisc,
	[IFLA_CARRIER]     = nlink_iface_parse_carrier
};

static const struct nlink_iface nlink_iface_null = {
	.type         = ARPHRD_VOID,
	.index        = 0,
	.ucast_hwaddr = NULL,
	.bcast_hwaddr = NULL,
	.name         = NULL,
	.name_len     = 0,
	.mtu          = 0,
	.link         = 0,
	.master       = 0,
	.oper_state   = IF_OPER_UNKNOWN,
	.group        = 0,
	.promisc      = 0,
	.carrier      = IF_OPER_UNKNOWN
};

int
nlink_iface_parse_msg(const struct nlmsghdr *msg, struct nlink_iface *iface)
{
	nlink_assert(msg);
	nlink_assert(!(msg->nlmsg_flags & NLM_F_DUMP_INTR));
	nlink_assert(msg->nlmsg_type == RTM_NEWLINK);

	const struct ifinfomsg *info;
	const struct nlattr    *attr;
	int                     ret = -ENODEV;

	*iface = nlink_iface_null;

	info = (struct ifinfomsg *)mnl_nlmsg_get_payload(msg);
	nlink_assert(info->ifi_type < ARPHRD_NONE);
	nlink_assert(info->ifi_index > 0);
	
	iface->type = info->ifi_type;
	iface->index = info->ifi_index;

	mnl_attr_for_each(attr, msg, sizeof(*info)) {
		uint16_t type;

		type = mnl_attr_get_type(attr);
		if (type < array_nr(nlink_iface_attr_parsers)) {
			ret = nlink_iface_attr_parsers[type](attr, iface);
			if (ret)
				return ret;
		}
	}

	if (!iface->name)
		return -ENODEV;

	return 0;
}

int
nlink_iface_setup_msg_ucast_hwaddr(struct nlmsghdr         *msg,
                                   const struct ether_addr *hwaddr)
{
	nlink_assert(msg);
	nlink_assert(msg->nlmsg_len >=
	             mnl_nlmsg_size(sizeof(struct ifinfomsg)));
	nlink_assert(unet_hwaddr_is_laa(hwaddr));
	nlink_assert(unet_hwaddr_is_ucast(hwaddr));

	if (!mnl_attr_put_check(msg,
	                        NLINK_XFER_MSG_SIZE,
	                        IFLA_ADDRESS,
	                        sizeof(*hwaddr),
	                        hwaddr))
		return -EMSGSIZE;

	return 0;
}

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

int
nlink_iface_setup_msg_mtu(struct nlmsghdr *msg, uint32_t mtu)
{
	nlink_assert(msg);
	nlink_assert(msg->nlmsg_len >=
	             mnl_nlmsg_size(sizeof(struct ifinfomsg)));
	nlink_assert(unet_mtu_isok(mtu));

	if (!mnl_attr_put_u32_check(msg,
	                            NLINK_XFER_MSG_SIZE,
	                            IFLA_MTU,
	                            mtu))
		return -EMSGSIZE;

	return 0;
}

void
nlink_iface_setup_new(struct nlmsghdr   *msg,
                      struct nlink_sock *sock,
                      unsigned short     type,
                      int                index)
{
	nlink_assert(msg);
	nlink_assert(sock);
	nlink_assert(index > 0);
	nlink_assert(type != ARPHRD_VOID);
	nlink_assert(type != ARPHRD_NONE);

	struct ifinfomsg *info;

	mnl_nlmsg_put_header(msg);
	msg->nlmsg_type = RTM_NEWLINK;
	msg->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	msg->nlmsg_seq = nlink_alloc_seqno(sock);
	msg->nlmsg_pid = sock->port_id;

	info = mnl_nlmsg_put_extra_header(msg, sizeof(*info));
	info->ifi_family = AF_UNSPEC;
	info->ifi_type = type;
	info->ifi_index = index;
	info->ifi_flags = 0;
	info->ifi_change = 0;
}

void
nlink_iface_setup_dump(struct nlmsghdr   *msg,
                       struct nlink_sock *sock)
{
	struct ifinfomsg *info;

	mnl_nlmsg_put_header(msg);
	msg->nlmsg_type = RTM_GETLINK;
	msg->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	msg->nlmsg_seq = nlink_alloc_seqno(sock);
	msg->nlmsg_pid = sock->port_id;

	info = mnl_nlmsg_put_extra_header(msg, sizeof(*info));
	info->ifi_family = AF_UNSPEC;
}
