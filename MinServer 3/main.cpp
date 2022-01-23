#include "ssocket.h"
#include <iostream>
using namespace std;

// Notes: already, lots of things from older version.

int port = 80;

int main(int argc, char* argv[]) {
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
		bytes b = "Hello!";
		s.sends(b);
	});

	while (true) {
		this_thread::yield();
	}

	return 0;
}