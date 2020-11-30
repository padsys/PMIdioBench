ifneq ($(V),1)
Q ?= @
endif

SOURCES = $(wildcard *.c)
HEADERS = $(wildcard include/*.h)
HEADERS += linkedlist.h

OBJECTS = $(SOURCES:%.c=%.o)
DEPS = $(SOURCES:%.c=%.d)
PROGRAM = main

CACHE_LINESIZE = $(shell getconf LEVEL1_DCACHE_LINESIZE)
CC := $(shell which gcc)
CFLAGS = -std=c11 -Wall -Wl,--no-as-needed
CFLAGS += -DDCACHE1_LINESIZE=$(CACHE_LINESIZE)
CFLAGS += -D_REENTRANT
CFLAGS += -fno-strict-aliasing -w -g
ifeq ($(DEBUG),y)
CFLAGS += -O0 -fno-omit-frame-pointer -DDEBUG
else
CFLAGS += -O3 -march=native -DNDEBUG
endif
ifeq ($(EADR_AVAILABLE),y)
CFLAGS += -DEADR_AVAILABLE=1
else
CFLAGS += -DEADR_AVAILABLE=0
endif
ifeq ($(ENABLE_TIMER),y)
CFLAGS += -DCONFIG_TIMER=1
else
CFLAGS += -DCONFIG_TIMER=0
endif
INCLUDES = -I./include
LIBS = pthread rt m pmem
DEPFLAGS = -MMD -MP -MF $*.d.tmp
LDFLAGS = $(LIBS:%=-l%)

all : $(PROGRAM)

$(PROGRAM) : $(OBJECTS)
	$(Q)echo "  LINK $@"; \
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o : %.c $(HEADERS)
	$(Q)echo "  CC   $@"; \
	$(CC) -o $@ $(DEPFLAGS) $(CFLAGS) $(INCLUDES) -c $< && \
	mv -f $*.d.tmp $*.d && touch -c $@

.PHONY : clean
.PRECIOUS: %.o
clean :
	$(Q)rm -f $(PROGRAM) $(OBJECTS) $(DEPS)

-include $(OBJECTS:.o=.d)
