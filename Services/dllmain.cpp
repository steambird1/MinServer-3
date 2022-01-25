#include "../MinServer 3/ssocket.h"
#include "../MinServer 3/ssocket.cpp"
#include "../MinServer 3/bytes.h"
#include "../MinServer 3/bytes.cpp"
#include "../MinServer 3/utility.h"
#include "../MinServer 3/static_map.h"
#include "md5_3.h"
#include <cstdio>
using namespace std;

// struct-like thing
class int_file_token_obj : public int_static_map {
public:

	string filename;
	string perm;

	virtual string toStore() {
		return filename + "|" + perm;
	}
	virtual void fromStore(string data) {
		auto p = splitLines(data.c_str(), '|');
		if (p.size() < 2) {
			filename = "";
			perm = "";
		}
		else {
			filename = p[0];
			perm = p[1];
		}
	}
};

constexpr int decline_alloc = 16384;

class full_error : public exception {
public:
	virtual const char* what() {
		return "Space is too full to auto allocate";
	}
};

template <typename AutoTy>
int_string AutoAllocateToken(static_map<int_string, AutoTy> t) {
	int r;
	if (t.count() > decline_alloc) {
		throw full_error();
	}
	do {
		r = random_s();
	} while (t.exist(to_string(r)));
	return to_string(r);
}

extern "C" __declspec(dllexport) void ServerMain(ssocket::acceptor &s, dlldata d) {
	// With different operations...
	http_recv &r = d.rcv;

	http_send se;
	se.proto_ver = r.proto_ver;
	se.codeid = 200;
	se.code_info = "OK";

	auto p = r.toPaths();
	string &op = p.exts["method"];
	string &op2 = p.exts["operate"];

	auto user_table = static_map<int_string, int_string>("$users.txt");
	auto utoken_table = static_map<int_string, int_string>("$users_tokens.txt");
	auto ftoken_table = static_map<int_string, int_file_token_obj>("$files_tokens.txt");

	if (op == "file_operate") {

	}
	else if (op == "auth_workspace") {
		if (op2 == "check") {
			string &req = p.exts["request"];
			string &pwd = p.exts["passwd"];
			if (user_table.exist(req) && user_table.get(req).toString() == pwd) {
				int_string t = AutoAllocateToken(utoken_table);
				utoken_table.append(t, req);
				se.content = t.toString();
			}
			else {
				se.codeid = 400;
				se.code_info = "Bad request";
			}
		}
		else if (op2 == "register") {
			string &pwd = p.exts["passwd"];
			// 1. If you required,
			if (p.exts.count("request")) {
				string &req = p.exts["request"];
				if (user_table.exist(req)) {
					se.codeid = 400;
					se.code_info = "Bad request";
				}
				else {
					user_table.append(req, toMD5(pwd));
				}
			}
			else if (p.exts.count("token")) {
				int token = atoi(p.exts["token"].c_str());
				if (utoken_table.exist(to_string(token))) {
					int_string uname = utoken_table.get(to_string(token));
					user_table.set(uname, toMD5(pwd));
				}
				else {
					se.codeid = 400;
					se.code_info = "Bad request";
				}
			}
			else {
				// 2. If you not, auto alloc.
				int_string t = AutoAllocateToken(user_table);
				user_table.append(t, toMD5(pwd));
				se.content = t.toString();
			}
		}
	}
	else if (op == "upload") {

	}
	else {
		se.codeid = 400;
		se.code_info = "Bad Request";
	}
	// We don't support DLL execution now. It's not necessary.

	s.sends(se);
	se.release();
	r.release();	// It's just a copy
}