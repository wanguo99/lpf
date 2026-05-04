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

# CMake 配置选项
EMS_CONF_OPTS = \
	-DCMAKE_BUILD_TYPE=Release \
	-DBUILD_TESTING=OFF \
	-DBUILD_SHARED_LIBS=ON

# 如果启用测试，则编译测试程序
ifeq ($(BR2_PACKAGE_EMS_TESTS),y)
EMS_CONF_OPTS += -DBUILD_TESTING=ON
endif

# 安装示例应用
ifeq ($(BR2_PACKAGE_EMS_SAMPLE_APP),y)
define EMS_INSTALL_SAMPLE_APP
	$(INSTALL) -D -m 0755 $(@D)/build/release/bin/ems-sample-app \
		$(TARGET_DIR)/usr/bin/ems-sample-app
endef
EMS_POST_INSTALL_TARGET_HOOKS += EMS_INSTALL_SAMPLE_APP
endif

# 安装测试程序
ifeq ($(BR2_PACKAGE_EMS_TESTS),y)
define EMS_INSTALL_TESTS
	$(INSTALL) -D -m 0755 $(@D)/build/release/bin/ems-unit-test \
		$(TARGET_DIR)/usr/bin/ems-unit-test
endef
EMS_POST_INSTALL_TARGET_HOOKS += EMS_INSTALL_TESTS
endif

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
