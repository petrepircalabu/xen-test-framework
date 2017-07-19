.PHONY: all

all: monitor

HOSTCC ?= gcc

OBJS = monitor.o

#HOSTCFLAGS += -Wall -Werror
HOSTCFLAGS += -D__XEN_TOOLS__
HOSTCFLAGS += -DXC_WANT_COMPAT_DEVICEMODEL_API

%.o : %.c
	$(HOSTCC) -c $(HOSTCFLAGS) $(HOSTCPPFLAGS) $< -o $@

monitor: $(OBJS)
	$(HOSTCC) -o $@ $^ -lxenctrl

clean:
	$(RM) $(OBJS) monitor
