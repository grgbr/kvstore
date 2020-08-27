config-in             := Config.in
config-h              := kvstore/config.h

solibs                := libkvstore.so
libkvstore.so-objs     = store.o
libkvstore.so-objs    += $(call kconf_enabled,KVSTORE_LOG,log.o)
libkvstore.so-objs    += $(call kconf_enabled,KVSTORE_FILE,file.o)
libkvstore.so-objs    += $(call kconf_enabled,KVSTORE_ATTR,attr.o)
libkvstore.so-objs    += $(call kconf_enabled,KVSTORE_STRIDX,stridx.o)
libkvstore.so-objs    += $(call kconf_enabled,KVSTORE_AUTOIDX,autoidx.o)
libkvstore.so-cflags  := $(EXTRA_CFLAGS) -Wall -Wextra -D_GNU_SOURCE -DPIC -fpic
libkvstore.so-ldflags  = $(EXTRA_LDFLAGS) \
                         -shared -fpic -Wl,-soname,libkvstore.so \
                         -ldb
libkvstore.so-pkgconf  = $(call kconf_enabled,KVSTORE_ASSERT,libutils)
libkvstore.so-pkgconf += $(call kconf_enabled,KVSTORE_FILE,libutils)

HEADERDIR             := $(CURDIR)/include
headers                = kvstore/store.h
headers               += $(call kconf_enabled,KVSTORE_ATTR,kvstore/attr.h)
headers               += $(call kconf_enabled,KVSTORE_FILE,kvstore/file.h)
headers               += $(call kconf_enabled,KVSTORE_STRIDX,kvstore/stridx.h)
headers               += $(call kconf_enabled,KVSTORE_AUTOIDX,kvstore/autoidx.h)

bins                  := kvs_test
kvs_test-objs         := test.o
kvs_test-cflags       := $(EXTRA_CFLAGS) -Wall -Wextra -D_GNU_SOURCE
kvs_test-ldflags      := $(EXTRA_LDFLAGS) -lkvstore
kvs_test-pkgconf       = $(call kconf_enabled,KVSTORE_BTRACE,libbtrace)

define libkvstore_pkgconf_tmpl
prefix=$(PREFIX)
exec_prefix=$${prefix}
libdir=$${exec_prefix}/lib
includedir=$${prefix}/include

Name: libkvstore
Description: Key/Value store library
Version: %%PKG_VERSION%%
Requires.private: $(sort $(libkvstore.so-pkgconf))
Cflags: -I$${includedir}
Libs.private: -ldb
Libs: -L$${libdir} -lkvstore
endef

pkgconfigs         := libkvstore.pc
libkvstore.pc-tmpl := libkvstore_pkgconf_tmpl
