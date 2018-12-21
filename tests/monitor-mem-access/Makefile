include $(ROOT)/build/common.mk

NAME                   := monitor-mem-access
CATEGORY               := monitor
TEST-ENVS              := hvm64
CLASS                  := xtf.monitor_test.MonitorTestInfo
TEST-EXTRA-INFO        := monitor_args='--address=0x\$$(nm --defined-only @@VM_PATH@@ | grep test_fn | cut -d \  -f 1)'

obj-perenv += main.o

obj-monitor += monitor.o

include $(ROOT)/build/gen.mk

