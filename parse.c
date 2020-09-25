#include "parse.h"
#include <string.h>
#include <net/ethernet.h>

int
nlink_parse_uint8_attr(const struct nlattr *attr, uint8_t *value)
{
	nlink_assert(attr);
	nlink_assert(value);

	if (mnl_attr_validate(attr, MNL_TYPE_U8))
		return -errno;

	*value = mnl_attr_get_u8(attr);

	return 0;
}

int
nlink_parse_uint16_attr(const struct nlattr *attr, uint16_t *value)
{
	nlink_assert(attr);
	nlink_assert(value);

	if (mnl_attr_validate(attr, MNL_TYPE_U16))
		return -errno;

	*value = mnl_attr_get_u16(attr);

	return 0;
}

int
nlink_parse_uint32_attr(const struct nlattr *attr, uint32_t *value)
{
	nlink_assert(attr);
	nlink_assert(value);

	if (mnl_attr_validate(attr, MNL_TYPE_U32))
		return -errno;

	*value = mnl_attr_get_u32(attr);

	return 0;
}

int
nlink_parse_uint64_attr(const struct nlattr *attr, uint64_t *value)
{
	nlink_assert(attr);
	nlink_assert(value);

	if (mnl_attr_validate(attr, MNL_TYPE_U64))
		return -errno;

	*value = mnl_attr_get_u64(attr);

	return 0;
}

ssize_t
nlink_parse_string_attr(const struct nlattr  *attr,
                        const char          **str,
                        size_t                size)
{
	nlink_assert(attr);
	nlink_assert(str);
	nlink_assert(size > 1);

	size_t sz;

	if (mnl_attr_validate(attr, MNL_TYPE_NUL_STRING))
		return -errno;

	sz = mnl_attr_get_payload_len(attr);
	if (!sz || (sz > size))
		return -ERANGE;

	*str = mnl_attr_get_str(attr);

	return sz;
}

const struct ether_addr *
nlink_parse_hwaddr_attr(const struct nlattr *attr)
{
	nlink_assert(attr);

	int err;

	err = nlink_parse_binary_attr(attr);
	if (err) {
		errno = -err;
		return NULL;
	}

	if (mnl_attr_get_payload_len(attr) != sizeof(struct ether_addr)) {
		errno = ERANGE;
		return NULL;
	}

	return (struct ether_addr *)mnl_attr_get_payload(attr);
}
