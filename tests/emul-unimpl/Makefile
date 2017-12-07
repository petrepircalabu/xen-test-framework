include $(ROOT)/build/common.mk

NAME      		:= emul-unimpl
CATEGORY  		:= monitor
TEST-ENVS 		:= hvm64
CLASS	  		:= xtf.monitor_test.MonitorTestInfo
TEST-EXTRA-INFO	:= monitor_args='--address=0x\$$(nm --defined-only @@VM_PATH@@ | grep test_fn | cut -d \  -f 1)'

TEST-EXTRA-CFG := extra.cfg.in

LDS_TEMPLATE = link.lds.S

obj-perenv += main.o

obj-monitor += monitor.o

include $(ROOT)/build/gen.mk
