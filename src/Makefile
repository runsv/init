##
## default is to build for Linux using GNU make and gcc
##

#ar rcs $lib $@
#ar s $lib
# -lm -ldl -lz -ltermcap -ledit
# -ltecla -lcurses

#libtry.a:	$(LIB_OBJS)
#	ar cr $@ $(LIB_OBJS)
#	ranlib $@

#sinclude ../mk/*?.mk

PREFIX ?= /usr/local
DESTDIR ?=
LUA_MOD_DIR ?= /lib/lua/5.4
LUA_CMOD_DIR ?= /lib64/lua/5.4
CROSS ?=
CP ?= cp
RM ?= rm -f
MKDIR ?= mkdir -p
SHELL ?= /bin/sh
STRIP ?= strip -s
#CC ?= gcc -std=gnu99 -g -Os
#CC = gcc -std=c89
CC = gcc
CXX = g++
LD = gcc -s

# Skalibs libs/headers
SKALIBS_HOME = /usr
SKALIBS_INC_DIR = $(SKALIBS_HOME)/include
SKALIBS_LIB_DIR = /lib
SKALIBS_LIB = -L$(SKALIBS_LIB_DIR) -lskarnet
SKALIBS_STATIC_LIB = $(SKALIBS_LIB_DIR)/skalibs/libskarnet.a

# Lua libs/headers
LUA_HOME = /usr
TCL_HOME = /usr/local
LUA_INC_DIR = $(LUA_HOME)/include
LUA_LIB_DIR = $(LUA_HOME)/lib64
LUA_LIB = $(LUA_LIB_DIR)/liblua.a
LUA_LIBS = -lm -ldl -L$(LUA_LIB_DIR) -llua
#LUA_LDFLAGS = -lm -ldl -Wl,-rpath,$(LUA_LIB_DIR)
LUA_LDFLAGS = -lm -ldl -L$(LUA_LIB_DIR) $(LUA_LIB)

# Tcl libs/headers
TCL_INC_DIR = $(TCL_HOME)/include
TCL_LIB_DIR = $(TCL_HOME)/lib64
TCL_LIB = -ltcl8.6
TCL_LIBS = -L$(TCL_LIB_DIR) $(TCL_LIB)

# OpenRC einfo lib
EINFO_INC_DIR = /usr/include
EINFO_LIB_DIR = /lib64
EINFO_LIB = $(EINFO_LIB_DIR)/libeinfo.so

LIBS =
#INCLUDES = -I. -I$(LUA_HOME)/include
#INCS = -I$(LUA_HOME)/include -I$(TCL_HOME)/include
INCS = -I/usr/include -I/usr/local/include -I.
#CFLAGS = -Wall -Wpedantic -g -O2 $(INCS)
#CFLAGS += -Wall -Wpedantic -O2 $(INCS)
CFLAGS = -Wall -O2 $(INCS)
# -Wl,-rpath,/path/to/lib,/path/to/other/lib
#-rpath-link
#LDFLAGS = -Os -s -Wl,-rpath,/usr/local/lib64:/usr/lib64:/lib64
LDFLAGS = -s

inc = $(wildcard *?.h)
src = $(wildcard *?.c)
bin = delay fgrun lux pause pidfsup prcsup rcorder runas runlevel setutmpid
sbin = bbinit hardreboot hddown killall5 rmcgroup stage1 stage2 stage3 svinit tbinit testinit
bins = $(bin) $(sbin)
libs =
#inid_obj = main.o reboot.o respawn.o utils.o utmp.o
#obj = $(inid_obj) client.o hash.o rcorder.o runtcl.o
#objects = $(patsubst %.c,%.o,$(wildcard *.c))

.SUFFIXES : .c .h .cc .C .cpp .cxx .hh .H .o .so

#%.so :	%.c
#	$(CROSS)$(CC) $(CFLAGS) -fPIC -shared $<

%.o :	%.c
	@echo "  CC	$@"
	$(CROSS)$(CC) -c $(CFLAGS) $<

#% :	%.o
#	@echo "  LD	$@"
#	$(CROSS)$(LD) $(LDFLAGS) -o $@ $^

all :	libs exe

bbinit :	bbinit.c
	$(CROSS)$(CC) $(LDFLAGS) $(CFLAGS) -DBB=$(BUSYBOX) -o $@ $^

tbinit :	bbinit.c
	$(CROSS)$(CC) $(LDFLAGS) $(CFLAGS) -DBB=$(TOYBOX) -o $@ $^

svinit :	bbinit.c
	$(CROSS)$(CC) $(LDFLAGS) $(CFLAGS) -DBB=$(SYSVINIT) -o $@ $^

testinit :	reboot.o testinit.o
	@echo "  LD	$@"
	$(CROSS)$(LD) $(LDFLAGS) -o $@ $^

stage2 :	reboot.o stage2.o
	@echo "  LD	$@"
	$(CROSS)$(LD) $(LDFLAGS) -o $@ $^

stage3 :	reboot.o stage3.o
	@echo "  LD	$@"
	$(CROSS)$(LD) $(LDFLAGS) -o $@ $^

oinid :		$(inid_obj)
	@echo "  LD	$@"
	$(CROSS)$(LD) $(LDFLAGS) -o $@ $^

rcorder :	hash.o rcorder.o
	@echo "  LD	$@"
	$(CROSS)$(LD) $(LDFLAGS) -o $@ $^

runtcl.o :	runtcl.c
	@echo "  CC	$@"
	$(CROSS)$(CC) $(CFLAGS) $(INCS) -I$(TCL_INC_DIR) -c $<

runtcl :	runtcl.o
	@echo "  LD	$@"
	$(CROSS)$(LD) $(LDFLAGS) $(TCL_LIBS) -o $@ $^

rt :	runtcl

sys.o :		sys.c
	@echo "  CC	$@"
	$(CROSS)$(CC) $(CFLAGS) -I$(LUA_INC_DIR) -fPIC -O2 -c $^

sys.so :	sys.o
	@echo "  LD	$@"
	$(CROSS)$(LD) $(LDFLAGS) -fPIC -shared -o $@ $^

pesi.so :	sys.o
	@echo "  LD	$@"
	$(CROSS)$(LD) $(LDFLAGS) -fPIC -shared -o $@ $^

xinu.so :	sys.o
	@echo "  LD	$@"
	$(CROSS)$(LD) $(LDFLAGS) -fPIC -shared -o $@ $^

lua_einfo.o :		lua_einfo.c
	@echo "  CC	$@"
	$(CROSS)$(CC) $(CFLAGS) -I$(EINFO_INC_DIR) -I$(LUA_INC_DIR) -fPIC -O2 -c $^

einfo.so :		lua_einfo.o
	@echo "  LD	$@"
	$(CROSS)$(LD) $(LDFLAGS) -fPIC -shared -o $@ $^ $(EINFO_LIB)

einfo :		lua einfo.so

pesi :		pesi.so sys.so xinu.so

lux :	sys.c
	@echo "  CCLD	$@"
	$(CROSS)$(CC) $(LDFLAGS) $(CFLAGS) -I$(LUA_INC_DIR) $(LUA_LIBS) -DSTANDALONE -o $@ $^ $(LUA_LIB)

rl :	runlua

lua :	sys.so runlua

runsq.o :	runsq.c
	@echo "  CC	$@"

runsq :
	@echo "  LD	$@"

sq :	runsq

tcl :	runtcl

exe :		$(obj) $(bins)

strip :		exe
	$(CROSS)$(STRIP) $(bins) *?.so

clean :
	@$(RM) -f *?\~ *?.o *?.so *?.a a.out runtcl runlua $(bins)

install-conf :

install-doc :

install-man :

install-mod :

install-exe :		exe

install-strip-exe :		exe strip

install :		all install-conf install-exe

install-strip :		all install-conf install-strip-exe

overwrite :

check :		all

checks :	check

test :		check

tests :		check

full :		all lua tcl

install :	all strip
	$(MKDIR) -pm 755 $(DESTDIR)/bin $(DESTDIR)/sbin $(DESTDIR)/etc
	@echo "  INSTALL	$(bin)"
	$(INSTALL) $(bin) $(DESTDIR)/bin
	@echo "  INSTALL	$(sbin)"
	$(INSTALL) $(sbin) $(DESTDIR)/sbin
	ifeq ($(DESTDIR),)
	@echo
	@echo
	@echo cd /sbin
	@echo ln -s inid init
	@echo ln -s inidc reboot
	@echo ln -s inidc poweroff
	@echo ...
	@echo
	@echo If you want to use inid as new (sys)init.
	@echo Remember to backup the old tools before (sym)linking in case anything goes wrong !!
	@echo
	@echo
	endif

install-tcl :		all tcl install
	$(MKDIR) -pm 755 $(DESTDIR)/bin $(DESTDIR)/sbin $(DESTDIR)/etc
	if test -f runtcl ; then \
	@echo "  INSTALL	runtcl" ; \
	$(STRIP) runtcl ; $(INSTALL) runtcl $(DESTDIR)/bin ; fi

install-lua :		all lua install
	$(MKDIR) -pm 755 $(DESTDIR)/bin $(DESTDIR)/sbin $(DESTDIR)/etc \
	$(DESTDIR)$(LUA_CMOD_DIR) $(DESTDIR)$(LUA_MOD_DIR)
	if test -f runlua ; then \
	@echo "  INSTALL	runlua" ; \
	$(STRIP) runlua ; $(INSTALL) runlua $(DESTDIR)/bin ; fi
	if test -f sys.so ; then \
	@echo "  INSTALL	sys.so" ; \
	$(STRIP) sys.so ; \
	$(INSTALL) sys.so $(DESTDIR)$(LUA_MOD_DIR) ; \
	$(INSTALL) sys.so $(DESTDIR)$(LUA_MOD_DIR)/pesi.so ; \
	$(INSTALL) sys.so $(DESTDIR)$(LUA_MOD_DIR)/xinu.so ; \
	$(INSTALL) sys.so $(DESTDIR)$(LUA_MOD_DIR)/unx.so ; \
	$(INSTALL) sys.so $(DESTDIR)$(LUA_MOD_DIR)/osunix.so ; \
	fi

install-all :		all lua tcl install install-lua install-tcl

.PHONY :	help clean all install
