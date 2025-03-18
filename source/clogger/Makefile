# @file      Makefile
#   Makefile both for mingw on windows
#
# @copyright Copyright(c) 2024, mapaware.top
# @since 2024-10-29 13:01:23
# @date 2025-03-19 17:23:45
########################################################################
# Linux, CYGWIN_NT, MSYS_NT, ...
shuname = "$(shell uname)"
OSARCH ?= $(shell echo $(shuname)|awk -F '-' '{ print $$1 }')

# Note: cygwin is not supported! use mingw instead
ifeq ($(OSARCH), CYGWIN_NT)
	OSARCH=CYGWIN64
	$(error cygwin is not supported! use mingw instead)
endif

# mingw
ifeq ($(OSARCH), MINGW64_NT)
	OSARCH=MINGW64
else ifeq ($(OSARCH), MSYS_NT)
	OSARCH=MINGW64
else ifeq ($(OSARCH), MINGW32_NT)
	OSARCH=MINGW32
	$(error 32-bit mingw is not supported)
else
	OSARCH=LINUX64
endif

# project
PROJECT := $(notdir $(CURDIR))

VERSION_FILE := $(CURDIR)/VERSION
VERSION := $(shell cat $(VERSION_FILE))

STATICLIB := lib$(PROJECT).a
DYNAMICLIB_BASENAME := lib$(PROJECT).so
DYNAMICLIB_VERNAME := $(DYNAMICLIB_BASENAME).$(VERSION)

#--------------------------------------------------------------
# compiler
CC := gcc

# default build: RELEASE
# make BUILD=DEBUG
BUILD ?= RELEASE

# compile directives
#  -DCLOGGER_SHMGR_HANDLE
CFLAGS += -std=gnu11 -D_GNU_SOURCE -fPIC -Wall

NOWARNS ?= -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable

# load libs: -lpthread = libpthread.so
LDFLAGS += -lm -lpthread

ifeq ($(BUILD), DEBUG)
    # make BUILD=DEBUG
	CFLAGS += -D_DEBUG -g
else
    # default is release
	CFLAGS += -DNDEBUG -O3
endif

#--------------------------------------------------------------
INCDIRS := -I. -I..

#-I/usr/local/include -I/usr/include

# Get pathfiles for C source files like: 1.c 2.c
CSRCS := $(foreach cdir, $(CURDIR), $(wildcard $(cdir)/*.c))

CSRCS += $(foreach cdir, $(CURDIR)/../common, $(wildcard $(cdir)/*.c))

LIBDIRS :=

ifeq ($(OSARCH), MINGW64)
    CSRCS += $(foreach cdir, $(CURDIR)/../common/win32, $(wildcard $(cdir)/*.c))
	CFLAGS += -D__MINGW64__ -m64
	LDFLAGS += -lws2_32
else ifeq ($(OSARCH), LINUX64)
	CFLAGS += -D__LINUX__
    LDFLAGS += -lrt
else
	$(error $(OSARCH) is not supported)
endif

# Get names of object files: '1.o 2.o'
COBJS := $(patsubst %.c, %.o, $(notdir $(CSRCS)))

#--------------------------------------------------------------
.PHONY: all clean $(STATICLIB).$(OSARCH) $(DYNAMICLIB_VERNAME).$(OSARCH)

all: $(STATICLIB).$(OSARCH) $(DYNAMICLIB_VERNAME).$(OSARCH)

clean:
	@/usr/bin/find $(CURDIR) -type f -name '*.o' | xargs -I {} sh -c "rm -f {}"
	@rm -f lib$(PROJECT).a* lib$(PROJECT).so.*
	@echo "[clean] done: $(CURDIR)"

#--------------------------------------------------------------
# http://www.gnu.org/software/make/manual/make.html#Eval-Function
define COBJS_template =
$(basename $(notdir $(1))).o: $(1)
	$(CC) $(CFLAGS) $(NOWARNS) $(INCDIRS) -c $(1) -o $(basename $(notdir $(1))).o
endef

$(foreach src,$(CSRCS),$(eval $(call COBJS_template,$(src))))
#--------------------------------------------------------------

$(STATICLIB).$(OSARCH): $(COBJS)
	@echo ">>>> $(STATICLIB).$(OSARCH) >>>>"
	@rm -f $@
	@rm -f $(STATICLIB)
	ar cr $@ $^
	@ln -s $@ $(STATICLIB)


$(DYNAMICLIB_VERNAME).$(OSARCH): $(COBJS)
	@echo ">>>> $(DYNAMICLIB_VERNAME).$(OSARCH) >>>>"
	@rm -f $@
	@rm -f $(DYNAMICLIB_VERNAME)
	$(CC) $(CFLAGS) $(LIBDIRS) -shared \
		-Wl,--soname=$(DYNAMICLIB_VERNAME) \
		-Wl,--rpath='.:/usr/local/lib' \
		$^ -o $@ \
		$(LDFLAGS)
	@ln -s $@ $(DYNAMICLIB_VERNAME)