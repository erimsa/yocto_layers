DESCRIPTION = "Tibbo Project System (+gstreamer)"
LICENSE = "MIT"
PR = "r1"

require inc-tps-tin.inc

IMAGE_FEATURES = ""

IMAGE_FEATURES += "ssh-server-openssh"
IMAGE_INSTALL += "openssh-sftp-server"

IMAGE_INSTALL += "tps-tios-sp7021"
IMAGE_INSTALL += "tps-wan"
IMAGE_INSTALL += "gstreamer1.0-plugins-base"
IMAGE_INSTALL += "gstreamer1.0-plugins-good"
IMAGE_INSTALL += "gstreamer1.0-plugins-bad"
IMAGE_INSTALL += "gstreamer1.0-libav"

IMAGE_INSTALL += "v4l-utils"
IMAGE_INSTALL += "minicom"
IMAGE_INSTALL += "tcpdump"
IMAGE_INSTALL += "strace"
IMAGE_INSTALL += "littlevgl"
IMAGE_INSTALL += "test-freetype"

IMAGE_INSTALL += "iperf3"
#IMAGE_INSTALL += "node-sqlite3"
#IMAGE_INSTALL += "nodejs-npm"
IMAGE_INSTALL += "dvkgpio"

IMAGE_INSTALL += "gdbserver"
IMAGE_INSTALL += "tcf-agent"

#IMAGE_INSTALL += "alsa-utils alsa-states"
IMAGE_INSTALL += "bluez5 pulseaudio-server pulseaudio-misc ofono obexftp"
#IMAGE_INSTALL += "bluez5 pulseaudio-server pulseaudio-misc ofono bluez5-obex"
