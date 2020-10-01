#include <nlink/nlink.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>
#include <errno.h>

/******************************************************************************
 * Netlink message handling
 ******************************************************************************/

int
nlink_parse_error_msg(const struct nlmsghdr *msg)
{
	nlink_assert(msg);
	nlink_assert(mnl_nlmsg_ok(msg, msg->nlmsg_len));

	const struct nlmsgerr *err = mnl_nlmsg_get_payload(msg);

	if (msg->nlmsg_len < mnl_nlmsg_size(sizeof(*err)))
		return -EBADMSG;

	/* Netlink subsystems returns the errno value with different signess */
	if (err->error < 0)
		return err->error;

	if (err->error > 0)
		return -err->error;

	/* This is an ACK message. */
	return -ENODATA;
}

int
nlink_parse_msg_head(const struct nlmsghdr *msg)
{
	nlink_assert(msg);
	nlink_assert(mnl_nlmsg_ok(msg, msg->nlmsg_len));

	if (msg->nlmsg_flags & NLM_F_DUMP_INTR)
		return -EINTR;

	if (msg->nlmsg_type >= NLMSG_MIN_TYPE)
		/* Message OK, caller should parse contained attributes. */
		return 0;

	switch (msg->nlmsg_type) {
	case NLMSG_NOOP:
		/* Empty message, caller should skip it. */
		return -ENOENT;

	case NLMSG_ERROR:
		/*
		 * Error messages are used to convey ACK messages. In this case,
		 * -ENODATA is returned and caller should stop processing
		 * current datagram.
		 */
		return nlink_parse_error_msg(msg);

	case NLMSG_DONE:
		/*
		 * End of message sequence, caller should stop processing
		 * current datagram.
		 */
		return -ENODATA;

	case NLMSG_OVERRUN:
		/*
		 * Current message corrupted due to data loss. Caller should
		 * throw remaining parts of datagram away and stop processing
		 * it.
		 */
		return -EOVERFLOW;

	default:
		nlink_assert(0);
	}

	/* Prevent from warning when assertions disabled. */
	return -EIO;
}

int
nlink_parse_msg(const struct nlmsghdr *msg,
                size_t                 size,
                nlink_parse_msg_fn    *parse,
                void                  *data)
{
	nlink_assert(mnl_nlmsg_ok(msg, (int)size));
	nlink_assert(parse);
	nlink_assert(size);

	int bytes = size;
	int ret;

	do {
		ret = nlink_parse_msg_head(msg);
		switch (ret) {
		case 0:
			/*
			 * Message header is consistent and message carries
			 * data: run the callback given in argument.
			 */
			ret = parse(ret, msg, data);
			if (ret)
				return ret;
			break;

		case -ENOENT:
			/* Empty message, skip to next one. */
			ret = 0;
			break;

		case -EINTR:
			/*
			 * A signal has raised before data availability: return
			 * as soon as possible to allow the caller to react as
			 * quick as possible.
			 */
			return -EINTR;

		case -ENODATA:
			/*
			 * End of message sequence / datagram. Give the caller a
			 * chance to react to acknowledgment and return since
			 * we are always provided with enough space to hold
			 * the largest supported netlink message size, i.e.
			 * we are sure datagram is over.
			 */
		default:
			return parse(ret, msg, data);
		}

		msg = mnl_nlmsg_next(msg, &bytes);
	} while (mnl_nlmsg_ok(msg, bytes));

	/*
	 * As multipart messages may span multiple datagrams, return
	 * -EINPROGRESS when the end of multipart message marker (NLMSG_DONE)
	 * has not yet been processed.
	 * This allows the caller to wait for the next datagram to keep parsing
	 * the current multipart message sequence.
	 */
	return (ret || !(msg->nlmsg_flags & NLM_F_MULTI)) ? ret : -EINPROGRESS;
}

void
nlink_sprint_msg(char                   string[NLINK_SPRINT_MSG_SIZE],
                 const struct nlmsghdr *msg)
{
	snprintf(string,
	         NLINK_SPRINT_MSG_SIZE,
	         "pid: %.010" PRIu32
	         " | seqno:%.010" PRIu32
	         " | type:%.05" PRIu16
	         " | flags:%c%c%c%c"
	         " | length:%.010" PRIu32,
	         msg->nlmsg_pid,
	         msg->nlmsg_seq,
	         msg->nlmsg_type,
	         (msg->nlmsg_flags & NLM_F_REQUEST) ?  'R' : '-',
	         (msg->nlmsg_flags & NLM_F_MULTI) ?  'M' : '-',
	         (msg->nlmsg_flags & NLM_F_ACK) ?  'A' : '-',
	         (msg->nlmsg_flags & NLM_F_ECHO) ?  'E' : '-',
	         msg->nlmsg_len);
}

/******************************************************************************
 * Netlink socket handling
 ******************************************************************************/

ssize_t
nlink_send_msg(const struct nlink_sock *sock, const struct nlmsghdr *msg)
{
	nlink_assert_sock(sock);
	nlink_assert(msg);
	nlink_assert(msg->nlmsg_len);

	size_t  len = msg->nlmsg_len;
	ssize_t ret;

	ret = mnl_socket_sendto(sock->mnl, msg, len);
	if (ret < 0) {
		nlink_assert(errno != EACCES);
		nlink_assert(errno != EBADF);
		nlink_assert(errno != EDESTADDRREQ);
		nlink_assert(errno != EFAULT);
		nlink_assert(errno != EINVAL);
		nlink_assert(errno != EISCONN);
		nlink_assert(errno != EMSGSIZE);
		nlink_assert(errno != ENOTCONN);
		nlink_assert(errno != ENOTSOCK);
		nlink_assert(errno != EOPNOTSUPP);
		nlink_assert(errno != EPIPE);

		/*
		 * Possible return code:
		 * - EAGAIN:     no more data room available for transmission
		 * - ECONNRESET: remote peer reset connection
		 * - EINTR:      interrupted by a signal before any data were
		 *               transmitted
		 * - ENOBUFS:    interface output queue full, indicating the
		 *               interface has stopped sending ; may also be
		 *               caused by transient congestion
		 * - ENOMEM:     memory allocation failed
		 */
		return -errno;
	}

	/*
	 * Zero return code should never happen...
	 * Can it ever happen ?? Return -ECONNRESET ??
	 */
	nlink_assert((size_t)ret == len);

	return 0;
}

ssize_t
nlink_recv_msg(const struct nlink_sock *sock, struct nlmsghdr *msg)
{
	nlink_assert_sock(sock);
	nlink_assert(msg);

	ssize_t ret;

	ret = mnl_socket_recvfrom(sock->mnl, msg, NLINK_XFER_MSG_SIZE);
	if (ret < 0) {
		nlink_assert(errno != EBADF);
		nlink_assert(errno != EFAULT);
		nlink_assert(errno != EINVAL);
		nlink_assert(errno != ENOTCONN);
		nlink_assert(errno != ENOTSOCK);
		nlink_assert(errno != ENOSPC);

		/*
		 * Possible return code:
		 * - EAGAIN:       no data available for receival
		 * - ECONNREFUSED: remote peer refused connection
		 * - EINTR:        interrupted by a signal before data were
		 *                 available for receival
		 * - ENOMEM:       memory allocation failed
		 */
		return -errno;
	}

	nlink_assert(ret);

	if (!mnl_nlmsg_ok(msg, ret))
		return -EBADMSG;

	if (!mnl_nlmsg_portid_ok(msg, sock->port_id))
		return -ESRCH;

	return ret;
}

int
nlink_sock_fd(const struct nlink_sock *sock)
{
	nlink_assert(sock);

	return mnl_socket_get_fd(sock->mnl);
}

int
nlink_join_route_group(struct nlink_sock *sock, enum rtnetlink_groups group)
{
	nlink_assert(sock);
	nlink_assert(group > RTNLGRP_NONE);
	nlink_assert(group <= RTNLGRP_MAX);

	if (mnl_socket_setsockopt(sock->mnl,
	                          NETLINK_ADD_MEMBERSHIP,
	                          &group,
	                          sizeof(group))) {
		nlink_assert(errno != EBADF);
		nlink_assert(errno != EFAULT);
		nlink_assert(errno != EINVAL);
		nlink_assert(errno != ENOPROTOOPT);
		nlink_assert(errno != ENOTSOCK);
		return -errno;
	}

	return 0;
}

int
nlink_leave_route_group(struct nlink_sock *sock, enum rtnetlink_groups group)
{
	nlink_assert(sock);
	nlink_assert(group > RTNLGRP_NONE);
	nlink_assert(group <= RTNLGRP_MAX);

	if (mnl_socket_setsockopt(sock->mnl,
	                          NETLINK_DROP_MEMBERSHIP,
	                          &group,
	                          sizeof(group))) {
		nlink_assert(errno != EBADF);
		nlink_assert(errno != EFAULT);
		nlink_assert(errno != EINVAL);
		nlink_assert(errno != ENOPROTOOPT);
		nlink_assert(errno != ENOTSOCK);
		return -errno;
	}

	return 0;
}

int
nlink_open_sock(struct nlink_sock *sock, int bus, int flags)
{
	nlink_assert(sock);

	int err;
	int cap = 1;

	sock->mnl = mnl_socket_open2(bus, flags);
	if (!sock->mnl) {
		nlink_assert(errno != EINVAL);

		return -errno;
	}

	/* Disable extended acknowledgement reporting. */
	if (mnl_socket_setsockopt(sock->mnl,
	                          NETLINK_CAP_ACK,
	                          &cap,
	                          sizeof(cap))) {
		nlink_assert(errno != EBADF);
		nlink_assert(errno != EFAULT);
		nlink_assert(errno != EINVAL);
		nlink_assert(errno != ENOPROTOOPT);
		nlink_assert(errno != ENOTSOCK);

		err = -errno;
		goto close;
	}

	if (mnl_socket_bind(sock->mnl, 0, MNL_SOCKET_AUTOPID)) {
		nlink_assert(errno != EBADF);
		nlink_assert(errno != EINVAL);
		nlink_assert(errno != ENOTSOCK);

		err = -errno;
		goto close;
	}

	sock->seqno = (uint32_t)time(NULL);
	sock->port_id = mnl_socket_get_portid(sock->mnl);

	return 0;

close:
	nlink_close_sock(sock);

	return err;
}

void
nlink_close_sock(const struct nlink_sock *sock)
{
	nlink_assert_sock(sock);

	int err;
	int fd = mnl_socket_get_fd(sock->mnl);

	err = mnl_socket_close(sock->mnl);
	while (err && (errno == EINTR))
		err = close(fd);
}
