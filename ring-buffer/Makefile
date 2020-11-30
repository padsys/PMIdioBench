ifneq ($(V),1)
Q ?= @
endif

SOURCES = $(wildcard *.cc)
HEADERS = $(wildcard include/*.h)

OBJECTS = $(SOURCES:%.cc=%.o)
DEPS = $(SOURCES:%.cc=%.d)
PROGRAMS = $(SOURCES:%.cc=%.x)

CACHE_LINESIZE = $(shell getconf LEVEL1_DCACHE_LINESIZE)
CXX := $(shell which g++)
CFLAGS = -Wall -std=c++11 -Wl,--no-as-needed -g -O2 -march=native -DDCACHE1_LINESIZE=$(CACHE_LINESIZE)
ifeq ($(TIME_POP),y)
CFLAGS += -DTIME_POP
endif
ifeq ($(TIME_PUSH),y)
CFLAGS += -DTIME_PUSH
endif
INCLUDES = -I./include
LIBS = pmem pthread pmemobj
DEPFLAGS = -MMD -MP -MF $*.d.tmp
LDFLAGS = $(LIBS:%=-l%)

all : $(PROGRAMS)

%.x : %.o
	$(Q)echo "  LINK $@"; \
	$(CXX) $(LDFLAGS) -o $@ $<

%.o : %.cc $(HEADERS)
	$(Q)echo "  CXX  $@"; \
	$(CXX) -o $@ $(DEPFLAGS) $(CFLAGS) $(INCLUDES) -c $< && \
	mv -f $*.d.tmp $*.d && touch -c $@

.PHONY : clean
.PRECIOUS: %.o
clean :
	$(Q)rm -f $(PROGRAMS) $(OBJECTS) $(DEPS)

-include $(OBJECTS:.o=.d)
