#######################################################################
# Makefile
#   Build libclogger
#
# Author: 350137278@qq.com
#
# Update: 2021-06-22
#
# Show all predefinitions of gcc:
#
#   https://blog.csdn.net/10km/article/details/49023471
#
#   $ gcc -posix -E -dM - < /dev/null
#
#######################################################################
# Linux, CYGWIN_NT, MSYS_NT, ...
shuname="$(shell uname)"
OSARCH=$(shell echo $(shuname)|awk -F '-' '{ print $$1 }')

# debug | release (default)
RELEASE = 1
BUILDCFG = release

# 32 | 64 (default)
BITS = 64

# Is MINGW(1) or not(0)
MINGW_FLAG = 0

###########################################################
# Compiler Specific Configuration

CC = gcc

# for gcc-8+
# -Wno-unused-const-variable
CFLAGS += -std=gnu99 -D_GNU_SOURCE -fPIC -Wall -Wno-unused-function -Wno-unused-variable
#......

LDFLAGS += -lpthread -lm
#......


###########################################################
# Architecture Configuration

ifeq ($(RELEASE), 0)
	# debug: make RELEASE=0
	CFLAGS += -D_DEBUG -g
	BUILDCFG = debug
else
	# release: make RELEASE=1
	CFLAGS += -DNDEBUG -O3
	BUILDCFG = release
endif

ifeq ($(BITS), 32)
	# 32bits: make BITS=32
	CFLAGS += -m32
	LDFLAGS += -m32
else
	ifeq ($(BITS), 64)
		# 64bits: make BITS=64
		CFLAGS += -m64
		LDFLAGS += -m64
	endif
endif


ifeq ($(OSARCH), MSYS_NT)
	MINGW_FLAG = 1
	ifeq ($(BITS), 64)
		CFLAGS += -D__MINGW64__
	else
		CFLAGS += -D__MINGW32__
	endif
else ifeq ($(OSARCH), MINGW64_NT)
	MINGW_FLAG = 1
	ifeq ($(BITS), 64)
		CFLAGS += -D__MINGW64__
	else
		CFLAGS += -D__MINGW32__
	endif
else ifeq ($(OSARCH), MINGW32_NT)
	MINGW_FLAG = 1
	ifeq ($(BITS), 64)
		CFLAGS += -D__MINGW64__
	else
		CFLAGS += -D__MINGW32__
	endif
endif


ifeq ($(OSARCH), CYGWIN_NT)
	ifeq ($(BITS), 64)
		CFLAGS += -D__CYGWIN64__ -D__CYGWIN__
	else
		CFLAGS += -D__CYGWIN32__ -D__CYGWIN__
	endif
endif


###########################################################
# Project Specific Configuration
PREFIX = .
DISTROOT = $(PREFIX)/dist

# Given dirs for all source (*.c) files
SRC_DIR = $(PREFIX)/src
COMMON_DIR = $(SRC_DIR)/common
APPS_DIR = $(SRC_DIR)/apps

CLOGGER_DIR = $(SRC_DIR)/clogger

APPS_DISTROOT = $(DISTROOT)/apps

CLOGGER_VERSION_FILE = $(CLOGGER_DIR)/VERSION
CLOGGER_VERSION = $(shell cat $(CLOGGER_VERSION_FILE))

CLOGGER_STATIC_LIB = libclogger.a
CLOGGER_DYNAMIC_LIB = libclogger.so.$(CLOGGER_VERSION)

CLOGGER_DISTROOT = $(DISTROOT)/libclogger-$(CLOGGER_VERSION)
CLOGGER_DIST_LIBDIR=$(CLOGGER_DISTROOT)/lib/$(OSARCH)/$(BITS)/$(BUILDCFG)

#......


# Set all dirs for C source: './src/a ./src/b'
ALLCDIRS += $(SRCDIR) $(COMMON_DIR) $(CLOGGER_DIR)

# Get pathfiles for C source files: './src/a/1.c ./src/b/2.c'
CSRCS := $(foreach cdir, $(ALLCDIRS), $(wildcard $(cdir)/*.c))

# Get names of object files: '1.o 2.o'
COBJS = $(patsubst %.c, %.o, $(notdir $(CSRCS)))


# Given dirs for all header (*.h) files
INCDIRS += -I$(PREFIX) \
	-I$(SRC_DIR) \
	-I$(COMMON_DIR) \
	-I$(CLOGGER_DIR)
#......


ifeq ($(MINGW_FLAG), 1)
	MINGW_CSRCS = $(COMMON_DIR)/win32/syslog-client.c
	MINGW_LINKS = -lws2_32
else
	MINGW_CSRCS =
	MINGW_LINKS =
endif

MINGW_COBJS = $(patsubst %.c, %.o, $(notdir $(MINGW_CSRCS)))

###########################################################
# Build Target Configuration
.PHONY: all apps clean cleanall dist


all: $(CLOGGER_DYNAMIC_LIB).$(OSARCH) $(CLOGGER_STATIC_LIB).$(OSARCH)



#----------------------------------------------------------
# http://www.gnu.org/software/make/manual/make.html#Eval-Function

define COBJS_template =
$(basename $(notdir $(1))).o: $(1)
	$(CC) $(CFLAGS) -c $(1) $(INCDIRS) -o $(basename $(notdir $(1))).o
endef
#----------------------------------------------------------


$(foreach src,$(CSRCS),$(eval $(call COBJS_template,$(src))))

$(foreach src,$(MINGW_CSRCS),$(eval $(call COBJS_template,$(src))))


help:
	@echo
	@echo Build libclogger-$(shell cat $(CLOGGER_VERSION_FILE)) for:
	@echo
	@echo "1) 64 bits release (default)"
	@echo "    $$ make"
	@echo
	@echo "2) 32 bits debug"
	@echo "    $$ make RELEASE=0 BITS=32"
	@echo
	@echo Dist target into default path: $(CLOGGER_DISTROOT)"
	@echo "    $$ make dist"
	@echo
	@echo Dist target into given path:
	@echo "    $$ make CLOGGER_DISTROOT=/path/to/YourInstallDir dist"
	@echo
	@echo "Show make options:"
	@echo "    $$ make help"


$(CLOGGER_STATIC_LIB).$(OSARCH): $(COBJS) $(MINGW_COBJS)
	rm -f $@
	rm -f $(CLOGGER_STATIC_LIB)
	ar cr $@ $^
	ln -s $@ $(CLOGGER_STATIC_LIB)


$(CLOGGER_DYNAMIC_LIB).$(OSARCH): $(COBJS) $(MINGW_COBJS)
	$(CC) $(CFLAGS) -shared \
		-Wl,--soname=$(CLOGGER_DYNAMIC_LIB) \
		-Wl,--rpath='$(PREFIX):$(PREFIX)/lib:$(PREFIX)/../lib:$(PREFIX)/../libs/lib' \
		-o $@ \
		$^ \
		$(LDFLAGS) \
		$(MINGW_LINKS)
	ln -s $@ $(CLOGGER_DYNAMIC_LIB)


apps: dist test_clogger.exe.$(OSARCH) test_cloggerdll.exe.$(OSARCH)


# -lrt for Linux

test_clogger.exe.$(OSARCH): $(APPS_DIR)/test_clogger/app_main.c
	@echo Building test_clogger.exe.$(OSARCH)
	$(CC) $(CFLAGS) $< $(INCDIRS) -o $@ \
	$(CLOGGER_STATIC_LIB) \
	$(LDFLAGS) \
	$(MINGW_LINKS) \
	-lrt


test_cloggerdll.exe.$(OSARCH): $(APPS_DIR)/test_clogger/app_main.c
	@echo Building test_cloggerdll.exe.$(OSARCH)
	$(CC) $(CFLAGS) $< $(INCDIRS) -o $@ \
	$(CLOGGER_DYNAMIC_LIB) \
	$(LDFLAGS) \
	$(MINGW_LINKS) \
	-lrt


dist: all
	@mkdir -p $(CLOGGER_DISTROOT)/include/clogger
	@mkdir -p $(CLOGGER_DISTROOT)/include/common
	@mkdir -p $(CLOGGER_DIST_LIBDIR)
	@cp $(CLOGGER_DIR)/logger_api.h $(CLOGGER_DISTROOT)/include/clogger/
	@cp $(CLOGGER_DIR)/clogger.h $(CLOGGER_DISTROOT)/include/clogger/
	@cp $(PREFIX)/$(CLOGGER_STATIC_LIB).$(OSARCH) $(CLOGGER_DIST_LIBDIR)/
	@cp $(PREFIX)/$(CLOGGER_DYNAMIC_LIB).$(OSARCH) $(CLOGGER_DIST_LIBDIR)/
	@cd $(CLOGGER_DIST_LIBDIR)/ && ln -sf $(PREFIX)/$(CLOGGER_STATIC_LIB).$(OSARCH) $(CLOGGER_STATIC_LIB)
	@cd $(CLOGGER_DIST_LIBDIR)/ && ln -sf $(PREFIX)/$(CLOGGER_DYNAMIC_LIB).$(OSARCH) $(CLOGGER_DYNAMIC_LIB)
	@cd $(CLOGGER_DIST_LIBDIR)/ && ln -sf $(CLOGGER_DYNAMIC_LIB) libclogger.so


clean:
	-rm -f ./*.stackdump
	-rm -f $(COBJS) $(MINGW_COBJS)
	-rm -f test_clogger.exe.$(OSARCH) test_cloggerdll.exe.$(OSARCH)
	-rm -f $(CLOGGER_STATIC_LIB)
	-rm -f $(CLOGGER_DYNAMIC_LIB)
	-rm -f $(CLOGGER_STATIC_LIB).$(OSARCH)
	-rm -f $(CLOGGER_DYNAMIC_LIB).$(OSARCH)
	-rm -f ./msvc/*.VC.db
	-rm -rf ./msvc/libclogger/build
	-rm -rf ./msvc/libclogger/target
	-rm -rf ./msvc/libclogger_dll/build
	-rm -rf ./msvc/libclogger_dll/target
	-rm -rf ./msvc/test_clogger/build
	-rm -rf ./msvc/test_clogger/target
	-rm -rf ./msvc/test_cloggerdll/build
	-rm -rf ./msvc/test_cloggerdll/target


cleanall: clean
	-rm -rf $(DISTROOT)
