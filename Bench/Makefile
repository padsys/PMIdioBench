ifneq ($(V),1)
Q ?= @
endif

SOURCES = $(wildcard *.c)
HEADERS = $(wildcard include/*.h)

OBJECTS = $(SOURCES:%.c=%.o)
DEPS = $(SOURCES:%.c=%.d)
PROGRAMS = $(SOURCES:%.c=%.x)

CACHE_LINESIZE = $(shell getconf LEVEL1_DCACHE_LINESIZE)
CC := $(shell which gcc)
CFLAGS = -Wall -Wl,--no-as-needed -g -fopenmp -DCACHE_LINESIZE=$(CACHE_LINESIZE)
INCLUDES = -I./include
LIBS = pmem
DEPFLAGS = -MMD -MP -MF $*.d.tmp
LDFLAGS = $(LIBS:%=-l%)

all : $(PROGRAMS)

%.x : %.o
	$(Q)echo "  LINK $@"; \
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

%.o : %.c $(HEADERS)
	$(Q)echo "  CC   $@"; \
	$(CC) -o $@ $(DEPFLAGS) $(CFLAGS) $(INCLUDES) -c $< && \
	mv -f $*.d.tmp $*.d && touch -c $@

.PHONY : clean
.PRECIOUS: %.o
clean :
	$(Q)rm -f $(PROGRAMS) $(OBJECTS) $(DEPS)

-include $(OBJECTS:.o=.d)
