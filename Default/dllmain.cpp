#include "../MinServer 3/bytes.h"
#include "../MinServer 3/ssocket.h"
#include <cstdio>
using namespace std;

// Let's go on, write the easiest first

extern "C" __declspec(dllexport) void ServerMain(ssocket::acceptor &s, dlldata d) {
	http_recv r;
	http_send sd;
	r.release();
	s.receive(r);
	auto p = resolveMinorPath(r.path);
	auto &fpath = p.first;

	FILE *f = fopen(fpath.c_str(), "rb");
	if (f == NULL) {
		FILE *g = fopen(d.notfound.c_str(), "rb");
		sd.loadContent(g);
		fclose(g);
	}
	else {
		sd.loadContent(f);
		fclose(f);
	}

	s.sends(sd);
	r.release();
}