.PHONY: all

all: monitor

HOSTCC ?= gcc

OBJS = monitor.o

#HOSTCFLAGS += -Wall -Werror
HOSTCFLAGS += -D__XEN_TOOLS__ -g -O0
HOSTCFLAGS += -DXC_WANT_COMPAT_DEVICEMODEL_API -DXC_WANT_COMPAT_MAP_FOREIGN_API

%.o : %.c
	$(HOSTCC) -c $(HOSTCFLAGS) $(HOSTCPPFLAGS) $< -o $@

monitor: $(OBJS)
	$(HOSTCC) -o $@ $^ -lxenctrl -lxenstore -lxenevtchn

clean:
	$(RM) $(OBJS) monitor
