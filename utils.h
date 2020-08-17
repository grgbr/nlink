#include <net/ethernet.h>
#include <stdbool.h>

/**
 * nwu_hwaddr_is_laa() - Check wether a EUI-48 MAC address is locally
 *                       administered or not.
 */
static inline bool
nwu_hwaddr_is_laa(const struct ether_addr *addr)
{
	return !!(addr->ether_addr_octet[0] & 0x2);
}

/**
 * nwu_hwaddr_is_uaa() - Check wether a EUI-48 MAC address is universally
 *                       administered or not.
 */
static inline bool
nwu_hwaddr_is_uaa(const struct ether_addr *addr)
{
	return !nwu_hwaddr_is_laa(addr);
}

/**
 * nwu_hwaddr_is_mcast() - Check wether a EUI-48 MAC address is multicast or
 *                         not.
 */
static inline bool
nwu_hwaddr_is_mcast(const struct ether_addr *addr)
{
	return !!(addr->ether_addr_octet[0] & 0x1);
}

/**
 * nwu_hwaddr_is_ucast() - Check wether a EUI-48 MAC address isunicast or not.
 */
static inline bool
nwu_hwaddr_is_ucast(const struct ether_addr *addr)
{
	return !nwu_hwaddr_is_mcast(addr);
}
