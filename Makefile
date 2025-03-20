# library name
lib.name = websocketserver

# todo: figure how to upgrade the code to not use the ugly -Wno-error=$ that is needed in modern compilers 

cflags = -I ./include -Wno-error=int-conversion -Wno-error=incompatible-pointer-types
ldlibs += -lpthread 


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
endef


# include Makefile.pdlibbuilder
# (for real-world projects see the "Project Management" section
# in tips-tricks.md)

PDLIBBUILDER_DIR=./pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder

localdep_windows: install
	scripts/localdeps.win.sh "${installpath}/websocketserver.${extension}"
