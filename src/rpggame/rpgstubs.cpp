#include "rpggame.h"

namespace game
{
	void parsepacketclient(int chan, packetbuf &p) {}
	int sendpacketclient(ucharbuf &p, bool &reliable, dynent *d) { return -1; }
	bool allowedittoggle() { return true; }
	void writeclientinfo(stream *f) {}
	void connectattempt(const char *name, const char *password, const ENetAddress &address) {}
	void connectfail() {}
	void parseoptions(vector<const char *> &args) {}
	bool allowmove (physent *d) {return true;}
	bool collidecamera() {return !editmode;}
	void setupcamera(){}
	void particletrack(physent *owner, vec &o, vec &d) {}
	void dynlighttrack(physent *owner, vec &o, vec &hud) {}
	void vartrigger(ident *id) {}
	void bounced(physent *d, const vec &surface) {}
	bool allowthirdperson(bool msg) { return true; }
}

namespace server
{
	void *newclientinfo() {return NULL;}
	void deleteclientinfo(void *ci) {}
	void serverinit() {}
	int reserveclients() { return 0; }
	void clientdisconnect(int n) {}
	int clientconnect(int n, uint ip) { return DISC_NONE; }
	void localdisconnect(int n) {}
	void localconnect(int n) {}
	int numchannels() {return 0;}
	bool allowbroadcast(int n) { return false; }
	void recordpacket(int chan, void *data, int len) {}
	void parsepacket(int sender, int chan, packetbuf &p) {}
	void sendservmsg(const char *s) {}
	bool sendpackets(bool force) { return false; }
	void serverinforeply(ucharbuf &q, ucharbuf &p) {}
	void serverupdate() {}
	int protocolversion() { return -1; }
	int laninfoport() {return 0;}
	int serverinfoport(int servport) {return 0;}
	int serverport(int infoport) {return 0;}
	const char *defaultmaster() {return "";}
	int masterport() {return 0;}
	void processmasterinput(const char *cmd, int cmdlen, const char *args) {}
	void masterconnected() {}
	void masterdisconnected() {}
	bool ispaused() {return game::ispaused();}
	int scaletime(int t) { return t; }
}
