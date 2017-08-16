include $(ROOT)/build/common.mk

NAME      := emul_unhandleable
CATEGORY  := monitor
TEST-ENVS := hvm64

TEST-EXTRA-CFG := extra.cfg.in

obj-perenv += main.o

obj-monitor += monitor.o

include $(ROOT)/build/gen.mk
