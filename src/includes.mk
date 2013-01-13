override CXXFLAGS+= -Wall -fsigned-char -fno-exceptions -fno-rtti
override ENET_CFLAGS+= -g -O2

STRIP=
ifeq (,$(findstring -g,$(CXXFLAGS)))
ifeq (,$(findstring -pg,$(CXXFLAGS)))
	STRIP=strip
endif
endif

MV=mv

INCLUDES= -Ishared -Iengine -Ienet/include
CLIENT_INCLUDES= $(INCLUDES) -I/usr/X11R6/include `sdl-config --cflags`
CLIENT_LIBS= -Lenet/.libs -lenet -L/usr/X11R6/lib `sdl-config --libs` -lSDL_image -lSDL_mixer -lz -lGL -lX11


PLATFORM= $(shell uname -s)
ifeq ($(PLATFORM),Linux)
	CLIENT_LIBS+= -lrt
	PLATFORM_PATH=bin_unix
endif

ifeq ($(PLATFORM),SunOS)
	CLIENT_LIBS+= -lsocket -lnsl -lX11
	PLATFORM_PATH=bin_sun
endif

ifeq ($(PLATFORM),FreeBSD)
	#TODO does BSD build?
	PLATFORM_PATH=bin_bsd
endif

ifeq ($(shell uname -m),x86_64)
	MACHINE?=64
else
	MACHINE?=32
endif


CLIENT_OBJS= \
	shared/crypto.o \
	shared/geom.o \
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
	rpggame/rpgai.o \
	rpggame/rpgaction.o \
	rpggame/rpgcamera.o \
	rpggame/rpgchar.o \
	rpggame/rpgconfig.o \
	rpggame/rpgcontainer.o \
	rpggame/rpgeffect.o \
	rpggame/rpgentities.o \
	rpggame/rpghud.o \
	rpggame/rpggui.o \
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
	rpggame/rpgtest.o \
	rpggame/rpgtrigger.o \
	rpggame/waypoint.o

CLIENT_PCH = \
	shared/cube.h.gch \
	engine/engine.h.gch \
	rpggame/rpggame.h.gch


default: all

all: client

enet/Makefile:
	cd enet; CFLAGS="$(ENET_CFLAGS)" ./configure --enable-shared=no --enable-static=yes

libenet: enet/Makefile
	$(MAKE)	-C enet/ all

clean-enet: enet/Makefile
	$(MAKE) -C enet/ distclean

clean:
	-$(RM) $(CLIENT_PCH) $(CLIENT_OBJS) $(RPGCLIENT_OBJS) lamiae*.*

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

client: libenet $(CLIENT_OBJS) $(RPGCLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o lamiae$(MACHINE).$(BIN_SUFFIX) $(CLIENT_OBJS) $(RPGCLIENT_OBJS) $(CLIENT_LIBS)

install: all
ifneq (,$(STRIP))
	strip lamiae$(MACHINE).$(BIN_SUFFIX)
endif
	cp lamiae$(MACHINE).$(BIN_SUFFIX) ../$(PLATFORM_PATH)/
	chmod o+x ../$(PLATFORM_PATH)/lamiae$(MACHINE).$(BIN_SUFFIX)

shared/cube2font.o: shared/cube2font.c
	$(CXX) $(CXXFLAGS) -c -o $@ $< `freetype-config --cflags`

cube2font: shared/cube2font.o
	$(CXX) $(CXXFLAGS) -o cube2font shared/cube2font.o `freetype-config --libs` -lz

depend:
	makedepend -fincludes.mk -Y -Ishared -Iengine $(subst .o,.cpp,$(CLIENT_OBJS))
	makedepend -fincludes.mk -a -Y -Ishared -Iengine -Irpggame $(subst .o,.cpp,$(RPGCLIENT_OBJS))
	makedepend -fincludes.mk -a -o.h.gch -Y -Ishared -Iengine -Irpggame $(subst .h.gch,.h,$(CLIENT_PCH))

engine/engine.h.gch: shared/cube.h.gch
rpggame/game.h.gch: shared/cube.h.gch

# DO NOT DELETE

shared/crypto.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/crypto.o: shared/command.h shared/iengine.h shared/igame.h
shared/geom.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/geom.o: shared/command.h shared/iengine.h shared/igame.h
shared/stream.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/stream.o: shared/command.h shared/iengine.h shared/igame.h
shared/tools.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/tools.o: shared/command.h shared/iengine.h shared/igame.h
shared/zip.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/zip.o: shared/command.h shared/iengine.h shared/igame.h
engine/aa.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/aa.o: shared/ents.h shared/command.h shared/iengine.h shared/igame.h
engine/aa.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/aa.o: engine/texture.h engine/model.h engine/varray.h engine/AreaTex.h
engine/aa.o: engine/SearchTex.h
engine/bih.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/bih.o: shared/ents.h shared/command.h shared/iengine.h shared/igame.h
engine/bih.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/bih.o: engine/texture.h engine/model.h engine/varray.h
engine/blend.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/blend.o: shared/ents.h shared/command.h shared/iengine.h
engine/blend.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/blend.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/client.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/client.o: shared/ents.h shared/command.h shared/iengine.h
engine/client.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/client.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/command.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/command.o: shared/ents.h shared/command.h shared/iengine.h
engine/command.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/command.o: engine/bih.h engine/texture.h engine/model.h
engine/command.o: engine/varray.h
engine/console.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/console.o: shared/ents.h shared/command.h shared/iengine.h
engine/console.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/console.o: engine/bih.h engine/texture.h engine/model.h
engine/console.o: engine/varray.h
engine/decal.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/decal.o: shared/ents.h shared/command.h shared/iengine.h
engine/decal.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/decal.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/dynlight.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/dynlight.o: shared/ents.h shared/command.h shared/iengine.h
engine/dynlight.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/dynlight.o: engine/bih.h engine/texture.h engine/model.h
engine/dynlight.o: engine/varray.h
engine/grass.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/grass.o: shared/ents.h shared/command.h shared/iengine.h
engine/grass.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/grass.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/light.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/light.o: shared/ents.h shared/command.h shared/iengine.h
engine/light.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/light.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/main.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/main.o: shared/ents.h shared/command.h shared/iengine.h shared/igame.h
engine/main.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/main.o: engine/texture.h engine/model.h engine/varray.h
engine/material.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/material.o: shared/ents.h shared/command.h shared/iengine.h
engine/material.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/material.o: engine/bih.h engine/texture.h engine/model.h
engine/material.o: engine/varray.h
engine/movie.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/movie.o: shared/ents.h shared/command.h shared/iengine.h
engine/movie.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/movie.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/normal.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/normal.o: shared/ents.h shared/command.h shared/iengine.h
engine/normal.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/normal.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/octa.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/octa.o: shared/ents.h shared/command.h shared/iengine.h shared/igame.h
engine/octa.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/octa.o: engine/texture.h engine/model.h engine/varray.h
engine/octaedit.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/octaedit.o: shared/ents.h shared/command.h shared/iengine.h
engine/octaedit.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/octaedit.o: engine/bih.h engine/texture.h engine/model.h
engine/octaedit.o: engine/varray.h
engine/octarender.o: engine/engine.h shared/cube.h shared/tools.h
engine/octarender.o: shared/geom.h shared/ents.h shared/command.h
engine/octarender.o: shared/iengine.h shared/igame.h engine/world.h
engine/octarender.o: engine/octa.h engine/light.h engine/bih.h
engine/octarender.o: engine/texture.h engine/model.h engine/varray.h
engine/pvs.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/pvs.o: shared/ents.h shared/command.h shared/iengine.h shared/igame.h
engine/pvs.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/pvs.o: engine/texture.h engine/model.h engine/varray.h
engine/physics.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/physics.o: shared/ents.h shared/command.h shared/iengine.h
engine/physics.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/physics.o: engine/bih.h engine/texture.h engine/model.h
engine/physics.o: engine/varray.h engine/mpr.h
engine/rendergl.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/rendergl.o: shared/ents.h shared/command.h shared/iengine.h
engine/rendergl.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/rendergl.o: engine/bih.h engine/texture.h engine/model.h
engine/rendergl.o: engine/varray.h
engine/renderlights.o: engine/engine.h shared/cube.h shared/tools.h
engine/renderlights.o: shared/geom.h shared/ents.h shared/command.h
engine/renderlights.o: shared/iengine.h shared/igame.h engine/world.h
engine/renderlights.o: engine/octa.h engine/light.h engine/bih.h
engine/renderlights.o: engine/texture.h engine/model.h engine/varray.h
engine/rendermodel.o: engine/engine.h shared/cube.h shared/tools.h
engine/rendermodel.o: shared/geom.h shared/ents.h shared/command.h
engine/rendermodel.o: shared/iengine.h shared/igame.h engine/world.h
engine/rendermodel.o: engine/octa.h engine/light.h engine/bih.h
engine/rendermodel.o: engine/texture.h engine/model.h engine/varray.h
engine/rendermodel.o: engine/ragdoll.h engine/animmodel.h engine/vertmodel.h
engine/rendermodel.o: engine/skelmodel.h engine/hitzone.h engine/md2.h
engine/rendermodel.o: engine/md3.h engine/md5.h engine/obj.h engine/smd.h
engine/rendermodel.o: engine/iqm.h
engine/renderparticles.o: engine/engine.h shared/cube.h shared/tools.h
engine/renderparticles.o: shared/geom.h shared/ents.h shared/command.h
engine/renderparticles.o: shared/iengine.h shared/igame.h engine/world.h
engine/renderparticles.o: engine/octa.h engine/light.h engine/bih.h
engine/renderparticles.o: engine/texture.h engine/model.h engine/varray.h
engine/renderparticles.o: engine/explosion.h engine/lensflare.h
engine/renderparticles.o: engine/lightning.h
engine/rendersky.o: engine/engine.h shared/cube.h shared/tools.h
engine/rendersky.o: shared/geom.h shared/ents.h shared/command.h
engine/rendersky.o: shared/iengine.h shared/igame.h engine/world.h
engine/rendersky.o: engine/octa.h engine/light.h engine/bih.h
engine/rendersky.o: engine/texture.h engine/model.h engine/varray.h
engine/rendertext.o: engine/engine.h shared/cube.h shared/tools.h
engine/rendertext.o: shared/geom.h shared/ents.h shared/command.h
engine/rendertext.o: shared/iengine.h shared/igame.h engine/world.h
engine/rendertext.o: engine/octa.h engine/light.h engine/bih.h
engine/rendertext.o: engine/texture.h engine/model.h engine/varray.h
engine/renderva.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/renderva.o: shared/ents.h shared/command.h shared/iengine.h
engine/renderva.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/renderva.o: engine/bih.h engine/texture.h engine/model.h
engine/renderva.o: engine/varray.h
engine/server.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/server.o: shared/ents.h shared/command.h shared/iengine.h
engine/server.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/server.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/serverbrowser.o: engine/engine.h shared/cube.h shared/tools.h
engine/serverbrowser.o: shared/geom.h shared/ents.h shared/command.h
engine/serverbrowser.o: shared/iengine.h shared/igame.h engine/world.h
engine/serverbrowser.o: engine/octa.h engine/light.h engine/bih.h
engine/serverbrowser.o: engine/texture.h engine/model.h engine/varray.h
engine/shader.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/shader.o: shared/ents.h shared/command.h shared/iengine.h
engine/shader.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/shader.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/sound.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/sound.o: shared/ents.h shared/command.h shared/iengine.h
engine/sound.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/sound.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/texture.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/texture.o: shared/ents.h shared/command.h shared/iengine.h
engine/texture.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/texture.o: engine/bih.h engine/texture.h engine/model.h
engine/texture.o: engine/varray.h engine/scale.h
engine/ui.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/ui.o: shared/ents.h shared/command.h shared/iengine.h shared/igame.h
engine/ui.o: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/ui.o: engine/texture.h engine/model.h engine/varray.h
engine/ui.o: engine/textedit.h
engine/water.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/water.o: shared/ents.h shared/command.h shared/iengine.h
engine/water.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/water.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/world.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/world.o: shared/ents.h shared/command.h shared/iengine.h
engine/world.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/world.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/worldio.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/worldio.o: shared/ents.h shared/command.h shared/iengine.h
engine/worldio.o: shared/igame.h engine/world.h engine/octa.h engine/light.h
engine/worldio.o: engine/bih.h engine/texture.h engine/model.h
engine/worldio.o: engine/varray.h

rpggame/rpg.o: rpggame/rpggame.h shared/cube.h shared/tools.h shared/geom.h
rpggame/rpg.o: shared/ents.h shared/command.h shared/iengine.h shared/igame.h
rpggame/rpgai.o: rpggame/rpggame.h shared/cube.h shared/tools.h shared/geom.h
rpggame/rpgai.o: shared/ents.h shared/command.h shared/iengine.h
rpggame/rpgai.o: shared/igame.h
rpggame/rpgaction.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgaction.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgaction.o: shared/iengine.h shared/igame.h
rpggame/rpgcamera.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgcamera.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgcamera.o: shared/iengine.h shared/igame.h
rpggame/rpgchar.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgchar.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgchar.o: shared/iengine.h shared/igame.h
rpggame/rpgconfig.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgconfig.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgconfig.o: shared/iengine.h shared/igame.h
rpggame/rpgcontainer.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgcontainer.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgcontainer.o: shared/iengine.h shared/igame.h
rpggame/rpgeffect.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgeffect.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgeffect.o: shared/iengine.h shared/igame.h
rpggame/rpgentities.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgentities.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgentities.o: shared/iengine.h shared/igame.h
rpggame/rpghud.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpghud.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpghud.o: shared/iengine.h shared/igame.h
rpggame/rpggui.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpggui.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpggui.o: shared/iengine.h shared/igame.h
rpggame/rpgio.o: rpggame/rpggame.h shared/cube.h shared/tools.h shared/geom.h
rpggame/rpgio.o: shared/ents.h shared/command.h shared/iengine.h
rpggame/rpgio.o: shared/igame.h
rpggame/rpgitem.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgitem.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgitem.o: shared/iengine.h shared/igame.h
rpggame/rpgobstacle.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgobstacle.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgobstacle.o: shared/iengine.h shared/igame.h
rpggame/rpgplatform.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgplatform.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgplatform.o: shared/iengine.h shared/igame.h
rpggame/rpgproj.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgproj.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgproj.o: shared/iengine.h shared/igame.h
rpggame/rpgscript.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgscript.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgscript.o: shared/iengine.h shared/igame.h
rpggame/rpgstats.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgstats.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgstats.o: shared/iengine.h shared/igame.h
rpggame/rpgstatus.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgstatus.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgstatus.o: shared/iengine.h shared/igame.h
rpggame/rpgstubs.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgstubs.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgstubs.o: shared/iengine.h shared/igame.h
rpggame/rpgrender.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgrender.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgrender.o: shared/iengine.h shared/igame.h
rpggame/rpgtest.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgtest.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgtest.o: shared/iengine.h shared/igame.h
rpggame/rpgtrigger.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/rpgtrigger.o: shared/geom.h shared/ents.h shared/command.h
rpggame/rpgtrigger.o: shared/iengine.h shared/igame.h
rpggame/waypoint.o: rpggame/rpggame.h shared/cube.h shared/tools.h
rpggame/waypoint.o: shared/geom.h shared/ents.h shared/command.h
rpggame/waypoint.o: shared/iengine.h shared/igame.h

shared/cube.h.gch: shared/tools.h shared/geom.h shared/ents.h
shared/cube.h.gch: shared/command.h shared/iengine.h shared/igame.h
engine/engine.h.gch: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
engine/engine.h.gch: shared/command.h shared/iengine.h shared/igame.h
engine/engine.h.gch: engine/world.h engine/octa.h engine/light.h engine/bih.h
engine/engine.h.gch: engine/texture.h engine/model.h engine/varray.h
rpggame/rpggame.h.gch: shared/cube.h shared/tools.h shared/geom.h
rpggame/rpggame.h.gch: shared/ents.h shared/command.h shared/iengine.h
rpggame/rpggame.h.gch: shared/igame.h
