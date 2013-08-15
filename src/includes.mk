override CXXFLAGS+= -Wall -fsigned-char -fno-exceptions -fno-rtti
override ENET_CFLAGS+= -g -O2

BIN_SUFFIX=dbg

STRIP=
ifeq (,$(findstring -g,$(CXXFLAGS)))
ifeq (,$(findstring -pg,$(CXXFLAGS)))
	STRIP=strip
	BIN_SUFFIX=bin
endif
endif

MV=mv

INCLUDES= -Ishared -Iengine -Ienet/include
CLIENT_INCLUDES= $(INCLUDES) -I/usr/X11R6/include `sdl2-config --cflags`
CLIENT_LIBS= -L/usr/X11R6/lib `sdl2-config --libs` -lSDL2_image -lSDL2_mixer -lz -lGL -lX11


PLATFORM= $(shell uname -s)
PLATFORM_PATH="bin_unk"
PLATFORM_ARCH= $(shell uname -m)
PLATFORM_TYPE=windows

ifeq ($(PLATFORM),Linux)
	CLIENT_LIBS+= -lrt
	ifneq (,$(OVR))
		CLIENT_INCLUDES+= -ILibOVR/Include -DHAS_OVR=1
		ifneq (,$(findstring 64,$(PLATFORM_ARCH)))
			CLIENT_LIBS+= -LLibOVR/Lib/Linux/Release/x86_64 -lovr -ludev -lXinerama
		else
			CLIENT_LIBS+= -LLibOVR/Lib/Linux/Release/i386 -lovr -ludev -lXinerama
		endif
	endif
	PLATFORM_PATH=bin_unix
	PLATFORM_TYPE=unix
else
	ifneq (,$(findstring GNU,$(PLATFORM)))
		CLIENT_LIBS+= -lrt
	endif
endif

ifeq ($(PLATFORM),SunOS)
	CLIENT_LIBS+= -lsocket -lnsl -lX11
	PLATFORM_PATH=bin_sun
	PLATFORM_TYPE=unix
endif

ifeq ($(PLATFORM),FreeBSD)
	PLATFORM_PATH=bin_bsd
	PLATFORM_TYPE=unix
endif

ifeq ($(shell uname -m),x86_64)
	MACHINE?=64
endif

ifeq ($(shell uname -m),amd64)
	MACHINE?=64
endif

ifeq ($(shell uname -m),i686)
	MACHINE?=32
endif

ifeq ($(shell uname -m),i586)
	MACHINE?=32
endif

ifeq ($(shell uname -m),i486)
	MACHINE?=32
endif

# assume 64bit if in doubt
MACHINE?=64

ENET_OBJS= \
	enet/callbacks.o \
	enet/host.o \
	enet/list.o \
	enet/packet.o \
	enet/peer.o \
	enet/protocol.o \
	enet/unix.o \
	enet/win32.o

CLIENT_OBJS= \
	shared/crypto.o \
	shared/geom.o \
	shared/glemu.o \
	shared/stream.o \
	shared/tools.o \
	shared/zip.o \
	engine/aa.o \
	engine/bih.o \
	engine/blend.o \
	engine/client.o	\
	engine/command.o \
	engine/console.o \
	engine/decal.o \
	engine/dynlight.o \
	engine/grass.o \
	engine/light.o \
	engine/main.o \
	engine/material.o \
	engine/movie.o \
	engine/normal.o	\
	engine/octa.o \
	engine/octaedit.o \
	engine/octarender.o \
	engine/ovr.o \
	engine/pvs.o \
	engine/physics.o \
	engine/rendergl.o \
	engine/renderlights.o \
	engine/rendermodel.o \
	engine/renderparticles.o \
	engine/rendersky.o \
	engine/rendertext.o \
	engine/renderva.o \
	engine/server.o	\
	engine/serverbrowser.o \
	engine/shader.o \
	engine/sound.o \
	engine/texture.o \
	engine/ui.o \
	engine/water.o \
	engine/world.o \
	engine/worldio.o

RPGCLIENT_OBJS = \
	rpggame/rpg.o \
	rpggame/rpgaction.o \
	rpggame/rpgai.o \
	rpggame/rpgcamera.o \
	rpggame/rpgchar.o \
	rpggame/rpgconfig.o \
	rpggame/rpgcontainer.o \
	rpggame/rpgeffect.o \
	rpggame/rpgentities.o \
	rpggame/rpggui.o \
	rpggame/rpghud.o \
	rpggame/rpgio.o \
	rpggame/rpgitem.o \
	rpggame/rpgobstacle.o \
	rpggame/rpgplatform.o \
	rpggame/rpgproj.o \
	rpggame/rpgscript.o \
	rpggame/rpgstats.o \
	rpggame/rpgstatus.o \
	rpggame/rpgstubs.o \
	rpggame/rpgrender.o \
	rpggame/rpgreserved.o \
	rpggame/rpgtest.o \
	rpggame/rpgtrigger.o \
	rpggame/waypoint.o

CLIENT_PCH = \
	shared/cube.h.gch \
	engine/engine.h.gch \
	rpggame/rpggame.h.gch

ifeq (unix,$(PLATFORM_TYPE))
	ENET_XCFLAGS := $(shell ./check_enet.sh $(CC) $(CFLAGS))
endif

default: all

all: client

$(ENET_OBJS): CFLAGS += $(ENET_CFLAGS) $(ENET_XCFLAGS) -Ienet/include -Wno-error

clean:
	-$(RM) $(CLIENT_PCH) $(CLIENT_OBJS) $(RPGCLIENT_OBJS) $(ENET_OBJS) lamiae*.*

%.h.gch: %.h
	$(CXX) $(CXXFLAGS) -o $(subst .h.gch,.tmp.h.gch,$@) $(subst .h.gch,.h,$@)
	$(MV) $(subst .h.gch,.tmp.h.gch,$@) $@

%-standalone.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $(subst -standalone.o,.cpp,$@)

$(CLIENT_OBJS): CXXFLAGS += $(CLIENT_INCLUDES)

$(filter shared/%,$(CLIENT_OBJS)): $(filter shared/%,$(CLIENT_PCH))
$(filter engine/%,$(CLIENT_OBJS)): $(filter engine/%,$(CLIENT_PCH))

$(RPGCLIENT_OBJS): CXXFLAGS += $(CLIENT_INCLUDES) -Irpggame
$(RPGCLIENT_OBJS): $(filter rpggame/%,$(CLIENT_PCH))


client: $(ENET_OBJS) $(CLIENT_OBJS) $(RPGCLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o lamiae$(MACHINE).$(BIN_SUFFIX) $(ENET_OBJS) $(CLIENT_OBJS) $(RPGCLIENT_OBJS) $(CLIENT_LIBS)

install: all
ifneq (,$(STRIP))
	strip lamiae$(MACHINE).$(BIN_SUFFIX)
endif
	mkdir -p ../$(PLATFORM_PATH)
	cp lamiae$(MACHINE).$(BIN_SUFFIX) ../$(PLATFORM_PATH)/
	chmod o+x ../$(PLATFORM_PATH)/lamiae$(MACHINE).$(BIN_SUFFIX)

shared/tessfont.o: shared/tessfont.c
	$(CXX) $(CXXFLAGS) -c -o $@ $< `freetype-config --cflags`

tessfont: shared/tessfont.o
	$(CXX) $(CXXFLAGS) -o tessfont shared/tessfont.o `freetype-config --libs` -lz

fixspace:
	sed -i ':rep; s/^\([ ]*\)\t/\1    /g; trep'  shared/*.c shared/*.cpp shared/*.h engine/*.cpp engine/*.h
	sed -i 's/[ \t]*$$//;' shared/*.c shared/*.cpp shared/*.h engine/*.cpp engine/*.h rpggame/*.cpp rpggame/*.h

depend:
	makedepend -fincludes.mk -Y -Ienet/include $(subst .o,.c,$(ENET_OBJS))
	makedepend -fincludes.mk -a -Y -Ishared -Iengine $(subst .o,.cpp,$(CLIENT_OBJS))
	makedepend -fincludes.mk -a -Y -Ishared -Iengine -Irpggame $(subst .o,.cpp,$(RPGCLIENT_OBJS))
	makedepend -fincludes.mk -a -o.h.gch -Y -Ishared -Iengine -Irpggame $(subst .h.gch,.h,$(CLIENT_PCH))

engine/engine.h.gch: shared/cube.h.gch
rpggame/rpggame.h.gch: shared/cube.h.gch

# DO NOT DELETE

enet/callbacks.o: enet/include/enet/enet.h enet/include/enet/unix.h
enet/callbacks.o: enet/include/enet/types.h enet/include/enet/protocol.h
enet/callbacks.o: enet/include/enet/list.h enet/include/enet/callbacks.h
enet/host.o: enet/include/enet/enet.h enet/include/enet/unix.h
enet/host.o: enet/include/enet/types.h enet/include/enet/protocol.h
enet/host.o: enet/include/enet/list.h enet/include/enet/callbacks.h
enet/list.o: enet/include/enet/enet.h enet/include/enet/unix.h
enet/list.o: enet/include/enet/types.h enet/include/enet/protocol.h
enet/list.o: enet/include/enet/list.h enet/include/enet/callbacks.h
enet/packet.o: enet/include/enet/enet.h enet/include/enet/unix.h
enet/packet.o: enet/include/enet/types.h enet/include/enet/protocol.h
enet/packet.o: enet/include/enet/list.h enet/include/enet/callbacks.h
enet/peer.o: enet/include/enet/enet.h enet/include/enet/unix.h
enet/peer.o: enet/include/enet/types.h enet/include/enet/protocol.h
enet/peer.o: enet/include/enet/list.h enet/include/enet/callbacks.h
enet/protocol.o: enet/include/enet/utility.h enet/include/enet/time.h
enet/protocol.o: enet/include/enet/enet.h enet/include/enet/unix.h
enet/protocol.o: enet/include/enet/types.h enet/include/enet/protocol.h
enet/protocol.o: enet/include/enet/list.h enet/include/enet/callbacks.h
enet/unix.o: enet/include/enet/enet.h enet/include/enet/unix.h
enet/unix.o: enet/include/enet/types.h enet/include/enet/protocol.h
enet/unix.o: enet/include/enet/list.h enet/include/enet/callbacks.h

shared/crypto.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/crypto.o: shared/command.h shared/glexts.h shared/glemu.h
shared/crypto.o: shared/iengine.h shared/igame.h
shared/geom.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/geom.o: shared/command.h shared/glexts.h shared/glemu.h
shared/geom.o: shared/iengine.h shared/igame.h
shared/glemu.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/glemu.o: shared/command.h shared/glexts.h shared/glemu.h
shared/glemu.o: shared/iengine.h shared/igame.h
shared/stream.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/stream.o: shared/command.h shared/glexts.h shared/glemu.h
shared/stream.o: shared/iengine.h shared/igame.h
shared/tools.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/tools.o: shared/command.h shared/glexts.h shared/glemu.h
shared/tools.o: shared/iengine.h shared/igame.h
shared/zip.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/zip.o: shared/command.h shared/glexts.h shared/glemu.h
shared/zip.o: shared/iengine.h shared/igame.h
engine/aa.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/aa.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/aa.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/aa.o: engine/light.h engine/bih.h engine/texture.h engine/model.h
engine/bih.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/bih.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/bih.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/bih.o: engine/light.h engine/bih.h engine/texture.h engine/model.h
engine/blend.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/blend.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/blend.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/blend.o: engine/light.h engine/bih.h engine/texture.h engine/model.h
engine/client.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/client.o: shared/ents.h shared/command.h shared/glexts.h
engine/client.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/client.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/client.o: engine/texture.h engine/model.h
engine/command.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/command.o: shared/ents.h shared/command.h shared/glexts.h
engine/command.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/command.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/command.o: engine/texture.h engine/model.h
engine/console.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/console.o: shared/ents.h shared/command.h shared/glexts.h
engine/console.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/console.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/console.o: engine/texture.h engine/model.h
engine/decal.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/decal.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/decal.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/decal.o: engine/light.h engine/bih.h engine/texture.h engine/model.h
engine/dynlight.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/dynlight.o: shared/ents.h shared/command.h shared/glexts.h
engine/dynlight.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/dynlight.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/dynlight.o: engine/texture.h engine/model.h
engine/grass.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/grass.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/grass.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/grass.o: engine/light.h engine/bih.h engine/texture.h engine/model.h
engine/light.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/light.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/light.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/light.o: engine/light.h engine/bih.h engine/texture.h engine/model.h
engine/main.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/main.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/main.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/main.o: engine/light.h engine/bih.h engine/texture.h engine/model.h
engine/material.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/material.o: shared/ents.h shared/command.h shared/glexts.h
engine/material.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/material.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/material.o: engine/texture.h engine/model.h
engine/movie.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/movie.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/movie.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/movie.o: engine/light.h engine/bih.h engine/texture.h engine/model.h
engine/normal.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/normal.o: shared/ents.h shared/command.h shared/glexts.h
engine/normal.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/normal.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/normal.o: engine/texture.h engine/model.h
engine/octa.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/octa.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/octa.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/octa.o: engine/light.h engine/bih.h engine/texture.h engine/model.h
engine/octaedit.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/octaedit.o: shared/ents.h shared/command.h shared/glexts.h
engine/octaedit.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/octaedit.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/octaedit.o: engine/texture.h engine/model.h
engine/octarender.o: engine/engine.h shared/cube.h shared/tools.h
engine/octarender.o: shared/geom.h shared/ents.h shared/command.h
engine/octarender.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/octarender.o: shared/igame.h engine/world.h engine/octa.h
engine/octarender.o: engine/light.h engine/bih.h engine/texture.h
engine/octarender.o: engine/model.h
engine/ovr.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/ovr.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/ovr.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/ovr.o: engine/light.h engine/bih.h engine/texture.h engine/model.h
engine/pvs.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/pvs.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/pvs.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/pvs.o: engine/light.h engine/bih.h engine/texture.h engine/model.h
engine/physics.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/physics.o: shared/ents.h shared/command.h shared/glexts.h
engine/physics.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/physics.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/physics.o: engine/texture.h engine/model.h engine/mpr.h
engine/rendergl.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/rendergl.o: shared/ents.h shared/command.h shared/glexts.h
engine/rendergl.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/rendergl.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/rendergl.o: engine/texture.h engine/model.h
engine/renderlights.o: engine/engine.h shared/cube.h shared/tools.h
engine/renderlights.o: shared/geom.h shared/ents.h shared/command.h
engine/renderlights.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/renderlights.o: shared/igame.h engine/world.h engine/octa.h
engine/renderlights.o: engine/light.h engine/bih.h engine/texture.h
engine/renderlights.o: engine/model.h
engine/rendermodel.o: engine/engine.h shared/cube.h shared/tools.h
engine/rendermodel.o: shared/geom.h shared/ents.h shared/command.h
engine/rendermodel.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/rendermodel.o: shared/igame.h engine/world.h engine/octa.h
engine/rendermodel.o: engine/light.h engine/bih.h engine/texture.h
engine/rendermodel.o: engine/model.h engine/ragdoll.h engine/animmodel.h
engine/rendermodel.o: engine/vertmodel.h engine/skelmodel.h engine/hitzone.h
engine/rendermodel.o: engine/md3.h engine/md5.h engine/obj.h engine/smd.h
engine/rendermodel.o: engine/iqm.h
engine/renderparticles.o: engine/engine.h shared/cube.h shared/tools.h
engine/renderparticles.o: shared/geom.h shared/ents.h shared/command.h
engine/renderparticles.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/renderparticles.o: shared/igame.h engine/world.h engine/octa.h
engine/renderparticles.o: engine/light.h engine/bih.h engine/texture.h
engine/renderparticles.o: engine/model.h engine/explosion.h
engine/renderparticles.o: engine/lensflare.h engine/lightning.h
engine/rendersky.o: engine/engine.h shared/cube.h shared/tools.h
engine/rendersky.o: shared/geom.h shared/ents.h shared/command.h
engine/rendersky.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/rendersky.o: shared/igame.h engine/world.h engine/octa.h
engine/rendersky.o: engine/light.h engine/bih.h engine/texture.h
engine/rendersky.o: engine/model.h
engine/rendertext.o: engine/engine.h shared/cube.h shared/tools.h
engine/rendertext.o: shared/geom.h shared/ents.h shared/command.h
engine/rendertext.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/rendertext.o: shared/igame.h engine/world.h engine/octa.h
engine/rendertext.o: engine/light.h engine/bih.h engine/texture.h
engine/rendertext.o: engine/model.h
engine/renderva.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/renderva.o: shared/ents.h shared/command.h shared/glexts.h
engine/renderva.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/renderva.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/renderva.o: engine/texture.h engine/model.h
engine/server.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/server.o: shared/ents.h shared/command.h shared/glexts.h
engine/server.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/server.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/server.o: engine/texture.h engine/model.h
engine/serverbrowser.o: engine/engine.h shared/cube.h shared/tools.h
engine/serverbrowser.o: shared/geom.h shared/ents.h shared/command.h
engine/serverbrowser.o: shared/glexts.h shared/glemu.h shared/iengine.h
engine/serverbrowser.o: shared/igame.h engine/world.h engine/octa.h
engine/serverbrowser.o: engine/light.h engine/bih.h engine/texture.h
engine/serverbrowser.o: engine/model.h
engine/shader.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/shader.o: shared/ents.h shared/command.h shared/glexts.h
engine/shader.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/shader.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/shader.o: engine/texture.h engine/model.h
engine/sound.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/sound.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/sound.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/sound.o: engine/light.h engine/bih.h engine/texture.h engine/model.h
engine/texture.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/texture.o: shared/ents.h shared/command.h shared/glexts.h
engine/texture.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/texture.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/texture.o: engine/texture.h engine/model.h engine/scale.h
engine/ui.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/ui.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/ui.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/ui.o: engine/light.h engine/bih.h engine/texture.h engine/model.h
engine/ui.o: engine/textedit.h
engine/water.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/water.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/water.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/water.o: engine/light.h engine/bih.h engine/texture.h engine/model.h
engine/world.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/world.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
engine/world.o: shared/iengine.h shared/igame.h engine/world.h engine/octa.h
engine/world.o: engine/light.h engine/bih.h engine/texture.h engine/model.h
engine/worldio.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/worldio.o: shared/ents.h shared/command.h shared/glexts.h
engine/worldio.o: shared/glemu.h shared/iengine.h shared/igame.h
engine/worldio.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/worldio.o: engine/texture.h engine/model.h

rpggame/rpg.o: rpggame/rpggame.h shared/cube.h shared/tools.h shared/geom.h
rpggame/rpg.o: shared/ents.h shared/command.h shared/glexts.h shared/glemu.h
rpggame/rpg.o: shared/iengine.h shared/igame.h
rpggame/rpgaction.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgaction.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgaction.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgaction.o: shared/igame.h
rpggame/rpgai.o: rpggame/rpggame.h shared/cube.h shared/tools.h shared/geom.h
rpggame/rpgai.o: shared/ents.h shared/command.h shared/glexts.h
rpggame/rpgai.o: shared/glemu.h shared/iengine.h shared/igame.h
rpggame/rpgcamera.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgcamera.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgcamera.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgcamera.o: shared/igame.h
rpggame/rpgchar.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgchar.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgchar.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgchar.o: shared/igame.h
rpggame/rpgconfig.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgconfig.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgconfig.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgconfig.o: shared/igame.h
rpggame/rpgcontainer.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgcontainer.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgcontainer.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgcontainer.o: shared/igame.h
rpggame/rpgeffect.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgeffect.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgeffect.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgeffect.o: shared/igame.h
rpggame/rpgentities.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgentities.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgentities.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgentities.o: shared/igame.h
rpggame/rpggui.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpggui.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpggui.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpggui.o: shared/igame.h
rpggame/rpghud.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpghud.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpghud.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpghud.o: shared/igame.h
rpggame/rpgio.o: rpggame/rpggame.h shared/cube.h shared/tools.h shared/geom.h
rpggame/rpgio.o: shared/ents.h shared/command.h shared/glexts.h
rpggame/rpgio.o: shared/glemu.h shared/iengine.h shared/igame.h
rpggame/rpgitem.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgitem.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgitem.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgitem.o: shared/igame.h
rpggame/rpgobstacle.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgobstacle.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgobstacle.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgobstacle.o: shared/igame.h
rpggame/rpgplatform.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgplatform.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgplatform.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgplatform.o: shared/igame.h
rpggame/rpgproj.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgproj.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgproj.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgproj.o: shared/igame.h
rpggame/rpgscript.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgscript.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgscript.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgscript.o: shared/igame.h
rpggame/rpgstats.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgstats.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgstats.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgstats.o: shared/igame.h
rpggame/rpgstatus.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgstatus.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgstatus.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgstatus.o: shared/igame.h
rpggame/rpgstubs.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgstubs.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgstubs.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgstubs.o: shared/igame.h
rpggame/rpgrender.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgrender.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgrender.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgrender.o: shared/igame.h
rpggame/rpgreserved.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgreserved.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgreserved.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgreserved.o: shared/igame.h
rpggame/rpgtest.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgtest.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgtest.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgtest.o: shared/igame.h
rpggame/rpgtrigger.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgtrigger.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgtrigger.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/rpgtrigger.o: shared/igame.h
rpggame/waypoint.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/waypoint.o: shared/geom.h shared/ents.h shared/command.h
rpggame/waypoint.o: shared/glexts.h shared/glemu.h shared/iengine.h
rpggame/waypoint.o: shared/igame.h

shared/cube.h.gch: shared/tools.h shared/geom.h shared/ents.h
shared/cube.h.gch: shared/command.h shared/glexts.h shared/glemu.h
shared/cube.h.gch: shared/iengine.h shared/igame.h
engine/engine.h.gch: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
engine/engine.h.gch: shared/command.h shared/glexts.h shared/glemu.h
engine/engine.h.gch: shared/iengine.h shared/igame.h engine/world.h
engine/engine.h.gch: engine/octa.h engine/light.h engine/bih.h
engine/engine.h.gch: engine/texture.h engine/model.h
rpggame/rpggame.h.gch: shared/cube.h shared/tools.h shared/geom.h
rpggame/rpggame.h.gch: shared/ents.h shared/command.h shared/glexts.h
rpggame/rpggame.h.gch: shared/glemu.h shared/iengine.h shared/igame.h
