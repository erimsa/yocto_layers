DESCRIPTION = "Tibbo Project System (+gstreamer)"
LICENSE = "MIT"
PR = "r1"

require inc-tps-tin.inc
require inc-tps-depends.inc
require inc-tps-privdeps.inc

IMAGE_FEATURES = ""

IMAGE_FEATURES += "ssh-server-openssh"
IMAGE_INSTALL += "openssh-sftp-server"
IMAGE_FEATURES += "package-management"

IMAGE_INSTALL += "tps-tios-sp7021"

IMAGE_INSTALL += "gdbserver"
IMAGE_INSTALL += "tcf-agent"

IMAGE_INSTALL += "tps-wan"
IMAGE_INSTALL += "resize-helper"
IMAGE_INSTALL += "u-boot-fw-utils"
IMAGE_INSTALL += "tzdata-asia"
