include $(TOPDIR)/rules.mk

PKG_NAME:=treelights
PKG_VERSION:=0.0.1
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/treelights
	SECTION:=base
	CATEGORY:=Utilities
	DEFAULT:=n
	TITLE:=Christmas tree lights controller
	URL:=http://black-swift.com
	DEPENDS:=+libstdcpp
endef

define Package/treelights/description
	Christmas tree lights controller
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
	$(call Build/Configure/Default,--with-linux-headers=$(LINUX_DIR))
endef

define Package/treelights/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/treelights $(1)/usr/bin/
endef

$(eval $(call BuildPackage,treelights))
