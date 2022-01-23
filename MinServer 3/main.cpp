#include "ssocket.h"
#include <iostream>
using namespace std;

// Notes: already, lots of things from older version.

int port = 80;

int main(int argc, char* argv[]) {
	initalize_socket();
	ssocket s;
	for (int i = 1; i < argc; i++) {
		string p = argv[i];
		if (p == "--port") {
			port = atoi(argv[i + 1]);
			i++;
		}
	}

	if (!s.binds(port)) {
		cout << "Cannot bind!" << endl;
		return 1;
	}
	if (!s.listens()) {
		cout << "Cannot listen!" << endl;
		return 2;
	}

	s.accepts([&](ssocket::acceptor &s) {
		http_recv rc;
		rc.release();
		s.receive(rc);
		http_send sd;
		sd.attr["Content-Type"] = "text/plain";
		sd.code_info = "OK";
		sd.codeid = 200;
		sd.content = "Hello for your " + rc.proto_ver + " " + rc.process + " at " + rc.path;
		sd.proto_ver = rc.proto_ver;
		s.sends(sd);
		
		rc.release();
	});

	while (true) {
		this_thread::yield();
	}

	return 0;
}