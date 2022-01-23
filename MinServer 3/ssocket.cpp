#include "ssocket.h"


void http_recv::release()
{
	this->path.release();
	this->content.release();
	this->process = "";
	this->proto_ver = "";
	this->attr.clear();
}

path_info http_recv::toPaths()
{
	// Please notice that '?' was included in path
	string tmp = "", tkey = "";
	int mode = 0; // 'true' to after '?'; '2' to get values
	vector<string> v;
	map<string, string> r;
	string wpath = this->path.toString();
	wpath.erase(wpath.begin()); // beginning '/'
	for (auto i = wpath.begin(); i != wpath.end(); i++) {
		switch (*i) {
		case '?':
			if (!mode && tmp.length()) {
				v.push_back(tmp);
				tmp = "";
			}
			if (!mode) {
				mode = 1;
				break;
			}
			// Actually continue to push
		case '/':
			if (!mode && tmp.length()) {
				v.push_back(tmp);
				tmp = "";
			}
			else {
				tmp += (*i);
			}
			break;
		case '=':
			if (mode) {
				tkey = tmp;
				tmp = "";
				mode = 2;
			}
			else {
				tmp += (*i);
			}
			break;
		case '&':
			if (mode == 2) {
				r[tkey] = tmp;
				tmp = "";
				mode = 1;
			}
			else {
				tmp += (*i);
			}
			break;
		default:
			tmp += (*i);
		}
	}
	if (tmp.length()) {
		if (mode) {
			r[tkey] = tmp;
		}
		else {
			v.push_back(tmp);
		}
	}
	wpath.insert(wpath.begin(), '/'); // add back, It's not necessary
	return { v, r };
}

content_info http_recv::toCType()
{
	if (!this->attr.count("Content-Type"))
		return content_info();
	vector<string> s = splitLines(this->attr["Content-Type"].c_str(), ';', false, ' ');
	// To get boundary...
	vector<string> bs;
	string bsz;
	switch (s.size()) {
	case 1:
		return { this->attr["Content-Type"], "" };
	case 2:
		//return { s[0], s[1] };
		bs = splitLines(s[1].c_str(), '=', true);
		bsz = bs.size() >= 2 ? bs[1] : "";
		while (bsz.length() && bsz[0] == '-') bsz.erase(bsz.begin());
		return { s[0], bsz };
	default:
		return content_info();
	}
}

vector<post_info> http_recv::toPost()
{
	// Files may contains special things...
	const char *c = this->content.toCharArray();
	size_t l = this->content.length();
	string ba = toCType().boundary;
	if (ba == "") {
		return vector<post_info>();
	}
	bytes tmp;
	int state = 0;	// 0 -- Normal data.
				   // 1 -- Args.

	post_info p;
	vector<post_info> t;
	bool flag = true;
	for (size_t i = 0; i < l; i++) {
		if (c[i] == '\n') {
			// debug
			printf_d("Debugger: state=%d, tmp: %s\n", state, tmp.toCharArray());
			// end
			bool wflag = false;
			string s = tmp.toString();
			//if (!s.empty()) s.pop_back();	// Here wasn't this kind of thing ('r').
			while (s.length() && (s[s.length() - 1] == '\n' || s[s.length() - 1] == '\r')) s.pop_back();
			while (s.length() && s[0] == '-') s.erase(s.begin());
			//			 while (s.length() && s[s.length() - 1] == '-') s.pop_back();
			if (s.length() > 2 && s.substr(s.length() - 2) == "--") {
				printf_d("EOB Checking...\n");
				s = s.substr(0, s.length() - 2);
				printf_d("EOB Info: \"%s\"\nEO Bound: \"%s\"", s.c_str(), ba.c_str());
				if (s == ba) break;	// End of processing already
			}
			//printf_d("Debugger: s=\"%s\"\nDebugger:ba=\"%s\"\n", s.c_str(),ba.c_str());
			if (s == ba && state == 0) {
				// Start boundary execution
				state = 1;
				// Clear too much end-lines
				//if (p.content.length()) p.content.pop_back();
				t.push_back(p);
				p = post_info();
				p.boundary = tmp.toString();
				tmp.clear();
				continue;
			}
			else if (state == 1) {
				if (s == "") {
					state = 0;
					tmp.clear();
					continue;
				}
				vector<string> s = splitLines(tmp.toCharArray(), ':', true, ' ');
				if (s.size() < 2)
					continue;				// Probably 'r'
				p.attr[s[0]] = s[1];
				tmp.clear();
				continue;
			}
			else {
				p.content += (tmp + '\n');
				tmp.clear();
				continue;
			}
		}
		else tmp += c[i];
	}
	t.erase(t.begin());		// Erase first unused information
	//p.content += tmp;
	t.push_back(p);
	return t;
}

map<string, string> contentTypes()
{
	static map<string, string> ret = { {".apk", "application/vnd.android"},  {".html","text/html"}, {".htm", "text/html"}, {".ico","image/ico"}, {".jpg", "image/jpg"}, {".jpeg", "image/jpeg"}, {".png", "image/apng"}, {".txt","text/plain"}, {".css", "text/css"}, {".js", "application/x-javascript"}, {".mp3", "audio/mpeg"}, {".wav", "audio/wav"}, {".mp4", "video/mpeg"} };
	return ret;
}

string searchTypes(string extension, string def)
{
	static map<string, string> ct = contentTypes();
	if (!ct.count(extension))
		return def;
	return ct[extension];
}
WSADATA initalize_socket()
{
	WSADATA ws;
	if (WSAStartup(MAKEWORD(2, 2), &ws)) {
		throw exception("Cannot start WSA");
	}
	return ws;
}

int hex2dec(string s) {
	int t = 1, u = 0;
	while (s.length()) {
		char c = s[s.length() - 1];
		s.pop_back();
		if (c >= 'a' && c <= 'z') c = toupper(c);
		if (c >= 'A' && c <= 'F') u += (10 + (c - 'A')) * t;
		else u += (c - '0') * t;
		t *= 16;
	}
	return u;
}

bytes resolveHTTPSymbols(string s)
{
	bytes b;
	for (int it = 0; it < s.length(); it++) {
		char &i = s[it];
		if (i == '%') {
			b += char(hex2dec(s.substr(it + 1, 2)));
			it += 2;
		}
		else {
			b += i;
		}
	}
	return move(b);
}


long getFileLength(FILE * f)
{
	if (f == NULL) return 0;
	long fb = ftell(f);
	fseek(f, 0, SEEK_END);
	long res = ftell(f);
	fseek(f, fb, SEEK_SET);	// Recover
	return res;
}

vector<string> splitLines(const char *data, char spl, bool firstonly, char filter) {
	size_t len = strlen(data);
	string tmp = "";
	vector<string> res;
	bool flag = false;
	for (size_t i = 0; i < len; i++) {
		if (data[i] == spl && !(firstonly && flag)) {
			flag = true;
			res.push_back(tmp);
			tmp = "";
		}
		else if (data[i] != filter) {
			tmp += data[i];
		}
	}
	if (tmp.length()) res.push_back(tmp);
	return move(res);
}


bool ssocket::binds(int port)
{
	sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.S_un.S_addr = INADDR_ANY;
	// As we included something special, we have to use this to call C socket binder
	auto be = ::bind(this->s, (LPSOCKADDR)&sa, sizeof(sa));
	this->errored = (be == SOCKET_ERROR);
	return !(be == SOCKET_ERROR);
}

bool ssocket::listens(int backlog)
{
	auto be = listen(this->s, backlog);
	this->errored = (be == SOCKET_ERROR);
	return !(be == SOCKET_ERROR);
}

void ssocket::accepts(function<void(ssocket::acceptor &s)> acceptor, function<void(void)> runner) {
	while (true) {
		runner();
		int acsz = sizeof(this->acc);
		SOCKET a = accept(this->s, (SOCKADDR*)&this->acc, &acsz);

	}
}