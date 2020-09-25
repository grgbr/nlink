#ifndef _NLINK_PARSE_H
#define _NLINK_PARSE_H

#include <nlink/nlink.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>

struct nlattr;

extern int
nlink_parse_uint8_attr(const struct nlattr *attr, uint8_t *value);

extern int
nlink_parse_uint16_attr(const struct nlattr *attr, uint16_t *value);

extern int
nlink_parse_uint32_attr(const struct nlattr *attr, uint32_t *value);

extern int
nlink_parse_uint64_attr(const struct nlattr *attr, uint64_t *value);

extern ssize_t
nlink_parse_string_attr(const struct nlattr  *attr,
                        const char          **str,
                        size_t                size);


static inline int
nlink_parse_binary_attr(const struct nlattr *attr)
{
	nlink_assert(attr);

	if (mnl_attr_validate(attr, MNL_TYPE_BINARY))
		return -errno;

	return 0;
}

extern const struct ether_addr *
nlink_parse_hwaddr_attr(const struct nlattr *attr);

#endif /* _NLINK_PARSE_H */
