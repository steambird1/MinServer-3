#include "../MinServer 3/ssocket.h"
#include "../MinServer 3/ssocket.cpp"
#include "../MinServer 3/bytes.h"
#include "../MinServer 3/bytes.cpp"
#include <cstdio>
using namespace std;

// Let's go on, write the easiest first

extern "C" __declspec(dllexport) void ServerMain(ssocket::acceptor &s, dlldata d) {
	http_recv &r = d.rcv;
	http_send sd;
	//r.release();
	//s.receive(r);
	pair<string,string> p = resolveMinorPath(r.path);
	string fpath = p.first;

	sd.codeid = 200;
	sd.code_info = "OK";
	sd.proto_ver = r.proto_ver;
	sd.attr["Content-Type"] = "text/html";

	if (fpath.length() && (fpath[0] == '/' || fpath[0] == '\\')) fpath.erase(fpath.begin());

	FILE *f = fopen((d.currdir + fpath.c_str()).c_str(), "rb");
	if (f == NULL) {
		FILE *g = fopen(d.notfound.c_str(), "rb");
		sd.loadContent(g);
		sd.codeid = 404;
		sd.code_info = "Not Found";
		fclose(g);
	}
	else {
		sd.attr["Content-Type"] = ctypes.count(fpath) ? ctypes[fpath] : "text/plain";
		sd.loadContent(f);
		fclose(f);
	}

	s.sends(sd);
	r.release();
}