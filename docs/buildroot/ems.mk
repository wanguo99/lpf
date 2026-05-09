################################################################################
#
# ems
#
################################################################################

EMS_VERSION = 1.0.0
EMS_SITE = $(call github,wanguo99,EMS,$(EMS_VERSION))
# 如果使用本地源码，取消下面的注释
# EMS_SITE = /path/to/local/ems
# EMS_SITE_METHOD = local

EMS_LICENSE = MIT
EMS_LICENSE_FILES = LICENSE
EMS_INSTALL_STAGING = YES

# 依赖项
EMS_DEPENDENCIES = host-pkgconf

# CMake 配置选项（兼容 Buildroot 标准参数）
# Buildroot 会自动传递以下参数：
# - CMAKE_INSTALL_PREFIX（通常为 /usr）
# - CMAKE_TOOLCHAIN_FILE（交叉编译工具链）
# - CMAKE_BUILD_TYPE（构建类型）
# - CMAKE_INSTALL_LIBDIR（库目录，如 lib 或 lib64）
# - CMAKE_INSTALL_BINDIR（可执行文件目录，通常为 bin）
EMS_CONF_OPTS = \
	-DBUILD_TESTING=$(if $(BR2_PACKAGE_EMS_TESTS),ON,OFF) \
	-DBUILD_SHARED_LIBS=$(if $(BR2_STATIC_LIBS),OFF,ON) \
	-DBUILD_SAMPLE_APP=$(if $(BR2_PACKAGE_EMS_SAMPLE_APP),ON,OFF) \
	-DBUILD_WATCHDOG_APP=$(if $(BR2_PACKAGE_EMS_WATCHDOG_APP),ON,OFF) \
	-DINSTALL_HEADERS=ON

# 安装配置文件
define EMS_INSTALL_CONFIG
	$(INSTALL) -D -m 0644 $(EMS_PKGDIR)/ems.conf \
		$(TARGET_DIR)/etc/ems.conf
endef
EMS_POST_INSTALL_TARGET_HOOKS += EMS_INSTALL_CONFIG

# 安装 init 脚本
define EMS_INSTALL_INIT_SYSV
	$(INSTALL) -D -m 0755 $(EMS_PKGDIR)/S90ems \
		$(TARGET_DIR)/etc/init.d/S90ems
endef

$(eval $(cmake-package))
