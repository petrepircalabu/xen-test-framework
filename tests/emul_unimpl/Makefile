include $(ROOT)/build/common.mk

NAME      := emul_unimpl
CATEGORY  := monitor
TEST-ENVS := hvm64

TEST-EXTRA-CFG := extra.cfg.in

LDS_TEMPLATE = link.lds.S

obj-perenv += main.o

obj-monitor += monitor.o

include $(ROOT)/build/gen.mk
