config-in           := Config.in
config-h            := nlink/config.h

solibs              := libnlink.so
libnlink.so-objs     = nlink.o parse.o
libnlink.so-objs    += $(call kconf_enabled,NLINK_IFACE,iface.o)
libnlink.so-cflags   = $(EXTRA_CFLAGS) -Wall -Wextra -D_GNU_SOURCE -DPIC -fpic
libnlink.so-ldflags  = $(EXTRA_LDFLAGS) -shared -fpic -Wl,-soname,libnlink.so
libnlink.so-pkgconf  = libmnl $(call kconf_enabled,NLINK_ASSERT,libutils)

HEADERDIR           := $(CURDIR)/include
headers              = nlink/nlink.h
headers             += $(call kconf_enabled,NLINK_IFACE,nlink/iface.h)

define libnlink_pkgconf_tmpl
prefix=$(PREFIX)
exec_prefix=$${prefix}
libdir=$${exec_prefix}/lib
includedir=$${prefix}/include

Name: libnlink
Description: Easy Netlink library
Version: %%PKG_VERSION%%
Requires: libmnl $(call kconf_enabled,NLINK_ASSERT,libutils)
Cflags: -I$${includedir}
Libs: -L$${libdir} -lnlink
endef

pkgconfigs          := libnlink.pc
libnlink.pc-tmpl    := libnlink_pkgconf_tmpl
