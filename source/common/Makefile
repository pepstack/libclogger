# @file Makefile
#   Makefile both for mingw on windows
#
# @copyright Copyright(c) 2024, mapaware.top
# @since 2024-10-29 13:00:53
# @date 2024-10-29 16:54:39
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


# compiler
CC ?= gcc

# compile directives
CFLAGS ?= -std=gnu11 -D_GNU_SOURCE -fPIC -Wall

CFLAGSW := -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable

# load libs: -lpthread = libpthread.so
LDFLAGS ?= -lm -lpthread

# default build: RELEASE
# make BUILD=DEBUG
BUILD ?= RELEASE

VERSION_FILE := $(CURDIR)/VERSION
VERSION := $(shell cat $(VERSION_FILE))

#--------------------------------------------------------------
INCDIRS := -I. -I./uthash

# Get pathfiles for C source files like: 1.c 2.c
CSRCS := $(foreach cdir, $(CURDIR), $(wildcard $(cdir)/*.c))

# Get names of object files: '1.o 2.o'
COBJS := $(patsubst %.c, %.o, $(notdir $(CSRCS)))

#--------------------------------------------------------------
# source prefix dir
PREFIX := $(CURDIR)

# subdirs in prefix
SUBDIRS := uthash win32

#--------------------------------------------------------------
.PHONY: all clean $(COBJS)


all: $(COBJS)
	@for dir in $(SUBDIRS); do \
		$(MAKE) BUILD="$(BUILD)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" -C "$(PREFIX)/$$dir"; \
	done


clean:
	@/usr/bin/find . -type f -name '*.o' | xargs -I {} sh -c "rm -f {}"
	@echo "[clean] done: $(CURDIR)"
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C "$(PREFIX)/$$dir" clean; \
	done


#--------------------------------------------------------------
# http://www.gnu.org/software/make/manual/make.html#Eval-Function
define COBJS_template =
$(basename $(notdir $(1))).o: $(1)
	$(CC) $(CFLAGS) $(CFLAGSW) $(INCDIRS) -c $(1) -o $(basename $(notdir $(1))).o
endef

$(foreach src,$(CSRCS),$(eval $(call COBJS_template,$(src))))
#--------------------------------------------------------------