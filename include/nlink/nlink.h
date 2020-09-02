#ifndef _NLINK_H
#define _NLINK_H

#include <nlink/config.h>
#include <stdlib.h>
#include <libmnl/libmnl.h>

/* Old versions of libmnl don't define maximum receiver side message size. */
#ifndef MNL_SOCKET_DUMP_SIZE
#define MNL_SOCKET_DUMP_SIZE  (32768)
#endif

#define NLINK_XFER_MSG_SIZE   MNL_SOCKET_DUMP_SIZE
#define NLINK_SPRINT_MSG_SIZE (81U)

/******************************************************************************
 * Netlink assertion
 ******************************************************************************/

#if defined(CONFIG_NLINK_ASSERT)

#include <utils/assert.h>

#define nlink_assert(_expr) \
	uassert("nlink", _expr)

#else  /* !defined(CONFIG_NLINK_ASSERT) */

#define nlink_assert(_expr)

#endif /* defined(CONFIG_NLINK_ASSERT) */

/******************************************************************************
 * Netlink message parsing
 ******************************************************************************/

extern void
nlink_sprint_msg(char                   string[NLINK_SPRINT_MSG_SIZE],
                 const struct nlmsghdr *msg);

extern int
nlink_parse_error_msg(const struct nlmsghdr *msg);

extern int
nlink_parse_msg_head(const struct nlmsghdr *msg);

/******************************************************************************
 * Netlink message buffer (de)allocation
 ******************************************************************************/

static inline struct nlmsghdr *
nlink_alloc_msg(void)
{
	return malloc(NLINK_XFER_MSG_SIZE);
}

static inline void
nlink_free_msg(struct nlmsghdr *msg)
{
	free(msg);
}

/******************************************************************************
 * Netlink socket handling
 ******************************************************************************/

struct nlink_sock {
	uint32_t           seqno;
	unsigned int       port_id;
	struct mnl_socket *mnl;
};

#define nlink_assert_sock(_sock) \
	nlink_assert(_sock); \
	nlink_assert((_sock)->mnl)

extern int
nlink_sock_fd(const struct nlink_sock *sock);

static inline uint32_t
nlink_alloc_seqno(struct nlink_sock *sock)
{
	nlink_assert_sock(sock);

	return ++sock->seqno;
}

extern ssize_t
nlink_send_msg(const struct nlink_sock *sock, const struct nlmsghdr *msg);

extern ssize_t
nlink_recv_msg(const struct nlink_sock *sock, struct nlmsghdr *msg);

extern int
nlink_open_sock(struct nlink_sock *sock, int bus, int flags);

extern void
nlink_close_sock(const struct nlink_sock *sock);

#endif /* _NLINK_H */
