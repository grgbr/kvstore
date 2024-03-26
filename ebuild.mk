config-in             := Config.in
config-h              := kvstore/config.h

solibs                := libkvstore.so
libkvstore.so-objs     = store.o
libkvstore.so-objs    += $(call kconf_enabled,KVSTORE_LOG,log.o)
libkvstore.so-objs    += $(call kconf_enabled,KVSTORE_FILE,file.o)
libkvstore.so-objs    += $(call kconf_enabled,KVSTORE_ATTR,attr.o)
libkvstore.so-objs    += $(call kconf_enabled,KVSTORE_STRREC,strrec.o)
libkvstore.so-objs    += $(call kconf_enabled,KVSTORE_AUTOREC,autorec.o)
libkvstore.so-objs    += $(call kconf_enabled,KVSTORE_TABLE,table.o)
libkvstore.so-objs    += $(call kconf_enabled,KVSTORE_REPO,repo.o)
libkvstore.so-cflags  := $(EXTRA_CFLAGS) -Wall -Wextra -D_GNU_SOURCE -DPIC -fpic
libkvstore.so-ldflags  = $(EXTRA_LDFLAGS) \
                         -shared -fpic -Wl,-soname,libkvstore.so \
                         -ldb
libkvstore.so-pkgconf  = $(call kconf_enabled,KVSTORE_ASSERT,libutils)
libkvstore.so-pkgconf += $(call kconf_enabled,KVSTORE_FILE,libutils)
libkvstore.so-pkgconf += $(call kconf_enabled,KVSTORE_LOG,libstroll)

HEADERDIR             := $(CURDIR)/include
headers                = kvstore/store.h
headers               += $(call kconf_enabled,KVSTORE_ATTR,kvstore/attr.h)
headers               += $(call kconf_enabled,KVSTORE_FILE,kvstore/file.h)
headers               += $(call kconf_enabled,KVSTORE_STRREC,kvstore/strrec.h)
headers               += $(call kconf_enabled,KVSTORE_AUTOREC,kvstore/autorec.h)
headers               += $(call kconf_enabled,KVSTORE_TABLE,kvstore/table.h)
headers               += $(call kconf_enabled,KVSTORE_REPO,kvstore/repo.h)

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
Version: $(VERSION)
Requires: $(sort $(call kconf_enabled,KVSTORE_ASSERT,libutils) \
                 $(call kconf_enabled,KVSTORE_FILE,libutils))
Requires.private: $(call kconf_enabled,KVSTORE_LOG,libstroll)
Cflags: -I$${includedir}
Libs: -L$${libdir} -Wl,--push-state,--as-needed -lkvstore -Wl,--pop-state
Libs.private: -ldb
endef

pkgconfigs         := libkvstore.pc
libkvstore.pc-tmpl := libkvstore_pkgconf_tmpl

################################################################################
# Source code tags generation
################################################################################

tagfiles := $(shell find $(CURDIR) -type f)
