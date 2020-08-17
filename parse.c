#include "parse.h"
#include <string.h>

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

int
nlink_parse_string_attr(const struct nlattr *attr, char *value, size_t size)
{
	nlink_assert(attr);
	nlink_assert(value);
	nlink_assert(size > 1);

	size_t len;

	if (mnl_attr_validate(attr, MNL_TYPE_NUL_STRING))
		return -errno;

	len = mnl_attr_get_payload_len(attr);
	if (len > size)
		return -ERANGE;

	/*
	 * No need to check for size constraint since already done by
	 * MNL_TYPE_NUL_STRING attribute validation above.
	 */
	strcpy(value, mnl_attr_get_str(attr));

	return 0;
}
