#include "ssocket.h"

vector<string> defiles = { "", "index.html","index.htm" };
map<string, string> ctypes = { {".apk", "application/vnd.android"},  {".html","text/html"}, {".htm", "text/html"}, {".ico","image/ico"}, {".jpg", "image/jpg"}, {".jpeg", "image/jpeg"}, {".png", "image/apng"}, {".txt","text/plain"}, {".css", "text/css"}, {".js", "application/x-javascript"}, {".mp3", "audio/mpeg"}, {".wav", "audio/wav"}, {".mp4", "video/mpeg"} };

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


ssocket::ssocket()
{
	sock_init();
}

ssocket::ssocket(SOCKET s)
{
	sock_init();
	this->s = s;
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

void ssocket::accepts(function<void(ssocket::acceptor &s)> acceptor, function<void(void)> runner, int bufsz) {
	while (true) {
		runner();
		int acsz = sizeof(this->acc);
		SOCKET a = accept(this->s, (SOCKADDR*)&this->acc, &acsz);
		thread th = thread([a, bufsz, &acceptor]() {
			ssocket::acceptor ac = ssocket::acceptor(a, bufsz);
			try {
				acceptor(ac);
			}
			catch (exception ex) {
				printf("Error occured: %s!\n", ex.what());
			}
			catch (...) {
				printf("Critical error occured!\n");
			}
			ac.release_prev();
			ac.end_accept();
			__t_safe::auto_release_thread();
		});
		th.detach();
	}
}

void ssocket::sock_init()
{
	this->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	this->errored = (s == INVALID_SOCKET);
}

bytes ssocket::acceptor::raw_receive()
{
	int ret = recv(this->ace, this->recv_buf, sizeof(char)*this->rcbsz, 0);
	this->last_receive = ret;
	if (ret > 0) {
		bytes b;
		b.add(this->recv_buf, ret);
		return move(b);
	}
	else
		return bytes();
}

ssocket::acceptor::acceptor(SOCKET ac, int bufsz)
{
	this->recv_buf = new char[bufsz];
	this->rcbsz = bufsz;
	this->ace = ac;
	this->acc_errored = false;
}

void ssocket::acceptor::receive(http_recv & h)
{
	this->prev_recv.clear();	// May be unsafe ?
	bytes b = raw_receive();
	this->prev_recv += b;
	//b = bytes();				// Re-initalize

	// Getting 1st line (surely can use string
	// that will not contains '\0')
	string s = b.toString();
	vector<string> lf = splitLines(s.c_str());
	if (lf.size() < 1)
		return;
	// 1st line for informations
	vector<string> firstinf = splitLines(lf[0].c_str(), ' ');
	// [HTTP/?.?] [GET...] [/]
	if (firstinf.size() < 3)
		return;
	h.proto_ver = firstinf[2];
	h.process = firstinf[0];
	h.path = resolveHTTPSymbols(firstinf[1]);
	vector<string>::iterator i;
	size_t pos = lf[0].length();
	while (b[pos] == '\r' || b[pos] == '\n') pos++;
	printf_d("\n");
	for (i = lf.begin() + 1; i != lf.end(); i++) {
		if (i->empty()) {
			printf_d("Content receive started from line previous: %s\n", (i - 1)->c_str());
			break;
		}
		vector<string> af = splitLines(i->c_str(), ':', true);
		if (af.size() < 2)
			return;
		h.attr[af[0]] = af[1];
		pos += i->length();
		while (b[pos] == '\r' || b[pos] == '\n') pos++;
		/*printf("Previous 10 characters are: ");
		// debuggers
		for (size_t i = pos - 10; i <= pos; i++) printf("%c", b[i]);
		printf("\nNext 10 characters' ASCII are: ");
		for (size_t i = pos; i <= pos + 10; i++) printf("%c", b[i]);
		printf("\n");
		// end
		*/
	}
	if (!h.attr.count("Content-Length"))
	{
		b.release();
		return;
	}
	int l = atoi(h.attr["Content-Length"].c_str());
	//for (; i != lf.end(); i++) {
		// Oh! We can't do that.
		//h.content += ((*i) + '\n');
		//l -= (i->length() + 1);
	//}
	printf_d("Additional receiving:(ASCII=%d, pos=%zd, l=%d)", b[pos], pos, l);				// debuggin
	printf_d("Contented informations:\n");
	int lres = 0;
	for (size_t i = pos; i < min(l + pos, b.length()); i++) {
		h.content += b[i];
		lres++;
		//printf("{%d->%d}%d ",i,l+pos,b[i]);								// debugging
		printf_d("%c", b[i]);
	}

	printf_d("Another raw receive for contents remaining:\n");
	// BUT, FOR CONTENT
	// WE HAVE TO GET MORE
	printf_d("Less: %d\n", lres);
	// To save memory
	int r = 0;
	for (int i = 0; i < l - lres; i += r) {
		r = recv(this->ace, this->recv_buf, sizeof(char)*this->rcbsz, 0);
		if (r > 0) {
			h.content.add(this->recv_buf, r);
			this->prev_recv.add(this->recv_buf, r);
		}
	}
	b.release();
}

bool ssocket::acceptor::sends(bytes & sender)
{
	const char* dc = sender.toCharArray();
	bool t = (send(this->ace, dc, sender.length(), 0) != SOCKET_ERROR);
	delete[] dc; //?
	//data.release();	// It's a copy now
	return t;
}

bool ssocket::acceptor::sends(http_send & sender)
{
	bytes b = sender.proto_ver + " " + to_string(sender.codeid) + " " + sender.code_info + "\n";
	sender.attr["Content-Length"] = to_string(sender.content.length());
	//	 for (auto i = attr.begin(); i != attr.end(); i++)
	//		 b += (i->first + ": " + i->second) + "\n";
	for (auto &i : sender.attr) {
		b += (i.first + ": " + i.second + "\n");
	}
	b += '\n';
	b += sender.content;
	const char *d = b.toCharArray();
	bool t = (send(this->ace, d, b.length(), 0) != SOCKET_ERROR);
	delete[] d;
	b.release();
	return t;
}

void ssocket::acceptor::end_accept()
{
	closesocket(this->ace);
	this->acc_errored = true;
}

bool ssocket::acceptor::accept_vaild()
{
	return !this->acc_errored;
}

bytes & ssocket::acceptor::get_prev()
{
	return this->prev_recv;
}

void ssocket::acceptor::release_prev()
{
	this->prev_recv.release();
}

const char * ssocket::acceptor::get_paddr()
{
	return inet_ntoa(this->acc.sin_addr);
}

void http_send::loadContent(FILE * hnd)
{
	long len = getFileLength(hnd);
	char *c = new char[len + 1];
	fread(c, sizeof(char), len, hnd);
	this->content.clear();
	this->content.add(c, len);
	delete[] c;
}

void http_send::release()
{
	this->raw_send.release();
	this->raw_sending = false;
	this->proto_ver = "";
	this->code_info = "";
	this->attr.clear();
	this->content.release();
}

pair<string, string> resolveMinorPath(string full) {
	string f2 = full, f3 = full;
	for (size_t i = 0; i < full.length(); i++) {
		if (full[i] == '?') {
			f2 = f2.substr(0, i); // abc? (0,3) -> abc
			f3 = f3.substr(i + 1);
		}
	}
	return make_pair(f2, f3);
}

disp_info post_info::toDispInfo()
{
	// Content-Disposition
	 // processing
	if (!this->attr.count("Content-Disposition")) {
		return disp_info();
	}
	vector<string> s = splitLines(this->attr["Content-Disposition"].c_str(), ';', false, ' ');
	if (s.size() < 2) {
		if (s.size() < 1) {
			return disp_info();
		}
		return { s[0], {} };
	}
	map<string, string> atz;
	for (auto i = s.begin() + 1; i != s.end(); i++) {
		vector<string> sa = splitLines(i->c_str(), '=');
		if (sa.size() != 2)
			continue;
		atz[sa[0]] = sa[1];
	}
	return { s[0], atz };
}
