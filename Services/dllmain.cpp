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
	int perm;

	int_file_token_obj() {

	}

	virtual string toStore() {
		return filename + "|" + to_string(perm);
	}
	virtual void fromStore(string data) {
		auto p = splitLines(data.c_str(), '|');
		if (p.size() < 2) {
			filename = "";
			perm = 0;
		}
		else {
			filename = p[0];
			perm = atoi(p[1].c_str());
		}
	}
};

class int_flag : public int_static_map {

public:
	int_flag() {

	}

	virtual string toStore() {
		return "1";
	}
	virtual void fromStore(string data) {
		// Do nothing
	}
};

class int_fperm_key_obj : public int_static_map {
public:

	string username;
	string filename;
	int perm;

	int_fperm_key_obj() {

	}

	virtual string toStore() {
		return username + "|" + to_string(perm) + "|" + filename;
	}
	virtual void fromStore(string data) {
		auto p = splitLines(data.c_str(), '|');
		if (p.size() < 3) {
			username = "";
			filename = "";
			perm = 0;
		}
		else {
			username = p[0];
			perm = atoi(p[1].c_str());
			filename = p[2];
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

extern "C" __declspec(dllexport) void ServerMain(ssocket::acceptor &s, dlldata &d) {
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
	auto file_table = static_map<int_fperm_key_obj, int_flag>("$files.txt");
	auto utoken_table = static_map<int_string, int_string>("$users_tokens.txt");
	auto ftoken_table = static_map<int_string, int_file_token_obj>("$files_tokens.txt");

	if (op == "file_operate") {

	}
	else if (op == "auth_workspace") {
		if (op2 == "check") {
			string &req = p.exts["request"];
			string &pwd = p.exts["passwd"];
			string mpwd = toMD5(pwd);
			if (user_table.exist(req) && user_table.get(req).toString() == mpwd) {
				int_string t = AutoAllocateToken(utoken_table);
				utoken_table.append(t, req);
				se.content = t.toString();
			}
			else {
				se.codeid = 400;
				se.code_info = "Bad request";
			}
		}
		else if (op2 == "create" || op2 == "register") {
			string &pwd = p.exts["passwd"];
			// 1. If you required,
			if (p.exts.count("token")) {
				int token = atoi(p.exts["token"].c_str());
				if (utoken_table.exist(to_string(token))) {
					int_string uname = utoken_table.get(to_string(token));
					user_table.set(uname, toMD5(pwd));
				}
				else {
					se.codeid = 400;
					se.code_info = "Bad request";
				}
			} else if (p.exts.count("id")) {
				string &req = p.exts["id"];
				if (user_table.exist(req)) {
					se.codeid = 400;
					se.code_info = "Bad request";
				}
				else {
					user_table.append(req, toMD5(pwd));
				}
			}
			else {
				// 2. If you not, auto alloc.
				int_string t = AutoAllocateToken(user_table);
				user_table.append(t, toMD5(pwd));
				se.content = t.toString();
			}
		}
		else if (op2 == "chown") {
			string &fname = p.exts["file"];
			string &token = p.exts["token"];
			string &touid = p.exts["touid"];

			if (utoken_table.exist(token)) {
				int_fperm_key_obj f;
				f.perm = -1;	// -1 for ownership
				f.username = utoken_table.get(token);
				f.filename = fname;
				if (file_table.exist(f)) {
					file_table.erase(f);
					f.username = touid;
					// Here can only be ONE ownership
					file_table.append(f, int_flag());
				}
				else {
					se.codeid = 400;
					se.code_info = "Bad request";
				}
			}
			else {
				se.codeid = 400;
				se.code_info = "Bad request";
			}
		}
		else if (op2 == "chfperm") {
			string &fname = p.exts["file"];
			string &token = p.exts["token"];
			string &touid = p.exts["touid"];
			string &toperm = p.exts["toperm"];

			if (utoken_table.exist(token)) {
				int_fperm_key_obj f;
				f.perm = -1;
				f.username = utoken_table.get(token);
				f.filename = fname;
				if (file_table.exist(f)) {
					int_fperm_key_obj g;
					g.perm = atoi(toperm.c_str());
					g.username = touid;
					g.filename = fname;
					file_table.set(g, int_flag());
				}
				else {
					se.codeid = 400;
					se.code_info = "Bad request";
				}
			}
			else {
				se.codeid = 400;
				se.code_info = "Bad request";
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