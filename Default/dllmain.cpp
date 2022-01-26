#include "../MinServer 3/ssocket.h"
#include "../MinServer 3/ssocket.cpp"
#include "../MinServer 3/bytes.h"
#include "../MinServer 3/bytes.cpp"
#include "../MinServer 3/utility.h"
#include <cstdio>
using namespace std;

// Let's go on, write the easiest first

extern "C" __declspec(dllexport) void ServerMain(ssocket::acceptor &s, dlldata &d) {
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

	string rpath = (d.currdir + fpath.c_str());
	FILE *f = fopen(rpath.c_str(), "rb");
	if (f == NULL) {
		FILE *g = fopen(d.notfound.c_str(), "rb");
		sd.loadContent(g);
		sd.codeid = 404;
		sd.code_info = "Not Found";
		fclose(g);
	}
	else {
		string ge = getExt(fpath);
		string ctp = ctypes.count(ge) ? ctypes[ge] : "text/plain";
		sd.attr["Content-Type"] = ctp;
		if (ctp == "text/html") {
			string t = makeTemp(), label = "";
			bool label_f = false, inserted = false;
			const char* msp = readAll("mspara3.js").toCharArray();
			int sl = strlen(msp);
			FILE *g = fopen(t.c_str(), "w");
			fclose(f);
			f = fopen(rpath.c_str(), "r");
			while (!feof(f)) {
				char c = fgetc(f);
				if (feof(f)) break;//...
				if (c == '<') {
					label = "";
					label_f = true;
				}
				else if (c == '>') {
					label_f = false;
					if (label == "<head" || label == "<body") {
						if (!inserted) {
							inserted = true;
							// '>' for a ending
							fprintf(g, "><script>\n");
							// Insert our own thing
							int len = sl + r.proto_ver.length() + r.process.length() + p.first.length();
							string attr = "", post = "";

							// 1. Attr
							// It still necessary because somebody uses '"'
							for (auto &i : r.attr) {
								attr += "{key:\"" + encodeBytes(i.first) + "\",value:\"" + encodeBytes(i.second) + "\"},";
							}
							if (attr.length()) attr.pop_back();	//','

							// 2. Post
							auto po = r.toPost();
							for (auto &i : po) {
								post += "{attr:[";
								for (auto &j : i.attr) {
									post += "{key:\"" + encodeBytes(j.first) + "\",value:\"" + encodeBytes(j.second) + "\"},";
								}
								post.pop_back();	//',' and here's surely len > 0
								post += "],content:\"" + encodeBytes(i.content) + "\"},";
							}
							if (post.length()) post.pop_back(); //','

							char *dst = new char[len + attr.length() + post.length() + 2];
							sprintf(dst, msp, r.proto_ver.c_str(), r.process.c_str(), p.first.c_str(), attr.c_str(), post.c_str());
							fprintf(g, "%s\n", dst);

							// It'll help you to add '>'
							fprintf(g, "</script");
						}
					}
				}
				if (label_f) {
					label += c;
				}
				//else {
					fputc(c, g);
				//}
			}
			fclose(f);
			fclose(g);
			g = fopen(t.c_str(), "rb");
			sd.loadContent(g);
			fclose(g);
		}
		else {
			sd.loadContent(f);
			fclose(f);
		}
	}

	s.sends(sd);
	sd.release();
	r.release();
}