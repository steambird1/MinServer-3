#define _CRT_SECURE_NO_WARNINGS

#include "ssocket.h"
#include <iostream>
using namespace std;

// Notes: already, lots of things from older version.
// They can be used by your DLL.

int port = 80;
int bs = RCV_DEFAULT;

typedef void(*libcall)(ssocket::acceptor&,dlldata);
map<string, libcall> clibs;

string libs = "$library.txt";
string forbidden = "$forbids.htm";
string notfound = "$not_found.htm";

thread_local char buf3[MAX_PATH * 2];

// As thread_local we doesn't need any
#define USES_BEGIN(...)
#define USES_END(...)

int main(int argc, char* argv[]) {
	initalize_socket();
	ssocket s;
	for (int i = 1; i < argc; i++) {
		string p = argv[i];
		if (p == "--port") {
			port = atoi(argv[i + 1]);
			i++;
		}
		else if (p == "--lib") {
			libs = argv[i + 1];
			i++;
		}
		else if (p == "--forbidden") {
			forbidden = argv[i + 1];
			i++;
		}
		else if (p == "--not-found") {
			notfound = argv[i + 1];
			i++;
		}
		else if (p == "--buffer-size") {
			bs = atoi(argv[i + 1]);
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

	bool eflag = false;
	int aldr = 0;

	printf("Loading...\n");
	FILE *f = fopen(libs.c_str(), "r");
	if (f != NULL) {
		while (!feof(f)) {
			USES_BEGIN(buf3)

			fgets(buf3, sizeof(buf3), f);
			auto sp = splitLines(buf3, '|');
			// Format: [path]|[dll]
			if (sp.size() != 2) {
				printf("Error: Bad config in configruation: %s\n", buf3);
				eflag = true;
			}
			HINSTANCE l = LoadLibrary(sp[1].c_str());
			if (l == NULL) {
				FreeLibrary(l);
				printf("Error: Cannot load library %s in configruation\n", sp[1].c_str());
				eflag = true;
			}
			else {
				libcall addr = (libcall)GetProcAddress(l, "ServerMain");
				clibs[sp[0]] = addr;
				aldr++;
			}

			USES_END(buf3)
		}
		fclose(f);
	}

	if (eflag) system("pause");

	system("cls");
	printf("* Server running *\n%d Assiocation(s) loaded\n", aldr);
	s.accepts([&](ssocket::acceptor &s) {
		http_recv p;
		http_send se;
		// = clear
		p.release();
		se.release();

		s.receive(p);
		auto ph = resolveMinorPath(p.path);

		if (ph.first.find("$") != string::npos) {
			FILE *f = fopen(forbidden.c_str(), "rb");
			se.loadContent(f);
			fclose(f);
			goto sendup;
		}

		// Find for
		if (!clibs.count(ph.first)) {
			FILE *f = fopen(notfound.c_str(), "rb");
			se.loadContent(f);
			fclose(f);
			goto sendup;
		}
		else {
			dlldata d;
			d.forbidden = forbidden;
			d.notfound = notfound;
			clibs[ph.first](s, d);
			goto after_sent;
		}

	sendup: s.sends(se);
	after_sent: p.release();
		se.release();

	}, []() {}, bs);

	while (true) {
		this_thread::yield();
	}

	return 0;
}