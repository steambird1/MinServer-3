#include "ssocket.h"
#include <iostream>
using namespace std;

// Notes: already, lots of things from older version.
// They can be used by your DLL.

int port = 80;
int bs = RCV_DEFAULT;

typedef void(*libcall)(ssocket::acceptor&,dlldata);
map<string, libcall> clibs;

string sRemovingEOL(string s) {
	string t = s;
	while (t[t.length() - 1] == '\n') t.pop_back();
	return move(t);
}

string sCurrDir(string s = "") {
	char buf3[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, buf3);
	if (s.length()) {
		if (s[0] == '/' || s[0] == '\\') s.erase(s.begin());
	}
	return string(buf3) + "\\" + s;
}

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
			auto sp = splitLines(sRemovingEOL(buf3).c_str(), '|');
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

		s.receive(p);

		se.codeid = 200;
		se.code_info = "OK";
		se.proto_ver = p.proto_ver;
;
		auto ph = resolveMinorPath(p.path);

		if (ph.first.find("$") != string::npos) {
			FILE *f = fopen(forbidden.c_str(), "rb");
			se.codeid = 403;
			se.code_info = "Forbidden";
			se.loadContent(f);
			fclose(f);
			goto sendup;
		}

		// Find for
		if (!clibs.count(ph.first)) {
			FILE *f = fopen(notfound.c_str(), "rb");
			se.loadContent(f);
			se.codeid = 404;
			se.code_info = "Not Found";
			fclose(f);
			goto sendup;
		}
		else {
			dlldata d;
			d.forbidden = forbidden;
			d.notfound = notfound;
			d.currdir = sCurrDir();
			d.rcv = http_recv(p);
			clibs[ph.first](s, d);
			d.rcv.release();
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