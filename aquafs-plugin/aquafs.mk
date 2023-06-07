
aquafs_SOURCES-y = \
	fs/fs_aquafs.cc \
	fs/zbd_aquafs.cc \
	fs/io_aquafs.cc \
	fs/zonefs_aquafs.cc \
	fs/zbdlib_aquafs.cc

aquafs_HEADERS-y = \
	fs/fs_aquafs.h \
	fs/zbd_aquafs.h \
	fs/io_aquafs.h \
	fs/version.h \
	fs/metrics.h \
	fs/snapshot.h \
	fs/filesystem_utility.h \
	fs/zonefs_aquafs.h \
	fs/zbdlib_aquafs.h

aquafs_PKGCONFIG_REQUIRES-y += "libzbd >= 1.5.0"

AQUAFS_EXPORT_PROMETHEUS ?= n
aquafs_HEADERS-$(AQUAFS_EXPORT_PROMETHEUS) += fs/metrics_prometheus.h
aquafs_SOURCES-$(AQUAFS_EXPORT_PROMETHEUS) += fs/metrics_prometheus.cc
aquafs_CXXFLAGS-$(AQUAFS_EXPORT_PROMETHEUS) += -DAQUAFS_EXPORT_PROMETHEUS=1
aquafs_PKGCONFIG_REQUIRES-$(AQUAFS_EXPORT_PROMETHEUS) += ", prometheus-cpp-pull == 1.1.0"

aquafs_SOURCES += $(aquafs_SOURCES-y)
aquafs_HEADERS += $(aquafs_HEADERS-y)
aquafs_CXXFLAGS += $(aquafs_CXXFLAGS-y)
aquafs_LDFLAGS += -u aquafs_filesystem_reg

AQUAFS_ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

$(shell cd $(AQUAFS_ROOT_DIR) && ./generate-version.sh)
ifneq ($(.SHELLSTATUS),0)
$(error Generating AquaFS version failed)
endif

aquafs_PKGCONFIG_REQUIRES = $(aquafs_PKGCONFIG_REQUIRES-y)
