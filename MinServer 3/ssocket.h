#pragma once
#include <WinSock2.h>
#include <map>
#include <Windows.h>
#include "bytes.h"

#pragma comment(lib, "ws2_32.lib")

#define SEABIRD_NET_FRAMEWORK

// Change if code changed.
#define SEABIRD_NET_FRAMEWORK_VER 202201L

// Change if STRUCTURE or its PUBLIC BEHAIVOR changed.
// Not include BUG (NOT FEATURE) FIXES.
#define SEABIRD_NET_STRUCTURE_VER 3
#define SEABIRD_NET_STRUCTURE_SUBVER 0

#define SEABIRD_NET_DEBUG 0
#if SEABIRD_NET_DEBUG == 1
#define SEABIRD_NET_DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#if SEABIRD_NET_DEBUG == 2
#include "debugs.h"
#endif
#define SEABIRD_NET_DEBUG_PRINT(...) __noop
#endif
#define printf_d(...) SEABIRD_NET_DEBUG_PRINT(__VA_ARGS__)

#define DEPRECATED(text) __declspec(deprecated(text))

#ifndef SEABIRD_NET_NO_MEM_DEPRECATE
#define MEM_DEPRECATE(cons) DEPRECATED("Too much memory usage in this function. consider directly use: "#cons)
#endif

#ifndef SEABIRD_NET_NO_THREAD_DEPRECATE
#define THREAD_DEPRECATE(cons) DEPRECATED("This version doesn't support multithreading. consider using "#cons)
#endif

#pragma region(SSocket Extended Types)
struct path_info {
	vector<string> path;
	map<string, string> exts;
};

struct disp_info {
	string disp_sign;
	map<string, string> attr;
};

struct post_info {
	string boundary;
	map<string, string> attr;
	bytes content;

	void saveContent(FILE *hnd);
	disp_info toDispInfo();
};

struct content_info {
	string ctype;
	string boundary;
};

struct http_recv {
	string proto_ver;			// 1.0 or 1.1
	string process;				// methods, e.g. GET, POST, PUT
	bytes path;				// e.g. "/", "/index.html"
	map<string, string> attr;	// Attributes
	bytes content;				// Message body

	void release();
	path_info toPaths();		// To split path
	content_info toCType();		// To Content-Type information
	vector<post_info> toPost();	// To POST informations
};

struct http_send {
	bool raw_sending = false;
	bytes raw_send;				// Raw sending, not through toSender()-like

	string proto_ver;			// The same as http_recv
	int codeid;					// e.g. 200. HTTP codes.
	string code_info;			// e.g. 200 OK; 404 Not Found...
	map<string, string> attr;
	bytes content;

	void loadContent(FILE *hnd);
	// Load content from a file.
	// Deprecated
	MEM_DEPRECATE("ssocket::sends(sender&)")
		inline bytes toSender(bool autolen = true) {
		bytes b = proto_ver + " " + to_string(codeid) + " " + code_info + "\n";
		if (autolen) attr["Content-Length"] = to_string(this->content.length());
		//	 for (auto i = attr.begin(); i != attr.end(); i++)
		//		 b += (i->first + ": " + i->second) + "\n";
		for (auto &i : attr) {
			b += (i.first + ": " + i.second + "\n");
		}
		b += '\n';
		b += content;
		return move(b);
	} // Returns sendable info.
};
#pragma endregion

#define RCV_DEFAULT 4096
class ssocket {
public:
	
	class acceptor {
	public:
		acceptor(SOCKET ac);
		void receive(http_recv &h);
		bool sends(bytes &sender);
		bool sends(http_send& sender);
		void end_accept();
		bool accept_vaild();
		bytes& get_prev();
		void release_prev();
		const char* get_paddr();
	private:
		bytes raw_receive();
		SOCKET ace;
		sockaddr_in acc;
		bytes prev_recv;
		char *recv_buf;
		int rcbsz, last_receive;
		bool acc_errored;
	};

	ssocket();
	ssocket(SOCKET s);
	ssocket(int port);
	bool binds(int port);
	bool listens(int backlog = 5);
	bool accepts(function<void(acceptor &s)> acceptor, function<void(void)> runner = []() {});
private:
	void sock_init();
	SOCKET s;
	bool errored;
};