# library name
lib.name = websocketserver

cflags = -I ./include 
ldlibs += -lpthread 

WINARCH := $(shell $(CC) -dumpmachine)
ifneq ($(filter i686%, $(WINARCH)),)
	arch = m_i386
else
	arch = m_amd64
endif

websocketserver.class.sources = \
	./src/base64/base64.c \
	./src/handshake/handshake.c \
	./src/sha1/sha1.c \
	./src/utf8/utf8.c \
	./src/ws.c  \
	./src/pd_websocket.c \


datafiles = \
	websocketserver-help.pd \
	send_receive.html \
	LICENSE \
	README.md \
	CHANGELOG.txt \


define forWindows
  ldlibs = -lws2_32 
  datafiles += scripts/localdeps.win.sh 
endef


# include Makefile.pdlibbuilder
# (for real-world projects see the "Project Management" section
# in tips-tricks.md)

PDLIBBUILDER_DIR=./pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder

localdep_windows: install
	cd "${installpath}"; ./localdeps.win.sh websocketserver.dll; \
	mv websocketserver.dll websocketserver.$(arch); \
	rm ./localdeps.win.sh \
	
	