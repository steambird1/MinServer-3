#include "../MinServer 3/ssocket.h"
#include "../MinServer 3/ssocket.cpp"
#include "../MinServer 3/bytes.h"
#include "../MinServer 3/bytes.cpp"
#include "../MinServer 3/utility.h"
#include "../MinServer 3/static_map.h"
#include "../MinServer 3/file_object.h"
#include "../MinServer 3/file_object.cpp"
#include "md5_3.h"
#include <cstdio>
using namespace std;

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
	bool ownerflag;

	int_fperm_key_obj() : ownerflag(false) {

	}

	virtual string toStore() {
		return username + "|" + filename + "|" + (ownerflag ? "1": "0");
	}
	virtual void fromStore(string data) {
		auto p = splitLines(data.c_str(), '|');
		if (p.size() < 3) {
			username = "";
			filename = "";
			ownerflag = false;
		}
		else {
			username = p[0];
			filename = p[1];
			ownerflag = (p[2] == "1");
		}
	}

	static int getPerm(string s) {
		static map<char, int> table = { {'r',4},{'w',2},{'a',1} };
		return s.length() ? table[s[0]] : 0;
	}

	static bool checkPerm(string s, int val) {
		return val & getPerm(s) == getPerm(s);
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

struct my_data {
	map<int, file_object> fo;
};

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

	// One of the advantages to use static_map is that you can update your system without clear users' login.
	auto user_table = static_map<int_string, int_string>("$users.txt");
	auto user_logged = static_map<int_string, int_string>("$users_logged.txt");	// Shows user-to-token 
	auto file_table = static_map<int_fperm_key_obj, int_string>("$files.txt");
	auto utoken_table = static_map<int_string, int_string>("$users_tokens.txt"); // Shows token-to-user
	auto group_table = static_map<int_string, int_vec<int_string> >("$groups.txt");

	if (op == "file_operate") {
		// Open only as we used static_map
		string &fn = p.exts["filename"];
		string &mod = p.exts["mod"];
		string &t = p.exts["token"];
		file_object f = file_object(fn, mod);

#pragma region Authority


		bool flag = false;
		int_fperm_key_obj query;

		string u;
		if (!p.exts.count("token") || !utoken_table.exist(t)) u = "0";
		else u = utoken_table.get(t);

		query.filename = fn;

		// 1. Single-user authority
		query.username = u;
		if (file_table.exist(query)) {
			int r = atoi(file_table.get(query).toString().c_str());
			if (int_fperm_key_obj::checkPerm(mod, r)) {
				flag = true;
			}
		}

		// 2. Group-based authority
		auto grs = group_table.exist(u) ? group_table.get(u) : int_vec<int_string>();
		for (auto &i : grs.data) {
			query.username = i;
			if (file_table.exist(query)) {
				int r = atoi(file_table.get(query).toString().c_str());
				if (int_fperm_key_obj::checkPerm(mod, r)) {
					flag = true;
				}
			}
		}

		// 3. Authority failure
		if (!flag) {
			se.codeid = 403;
			se.code_info = "Forbidden";
			goto sendup;
		}

#pragma endregion

		if (mod.length()) {
			switch (mod[0]) {
			case 'r':
				se.loadContent(f);
				f.close();
				break;
			case 'w': case 'a':
				fprintf(f, "%s", se.content.toCharArray());
				f.close();
				break;
			default:
				se.codeid = 400;
				se.code_info = "Bad request";
				break;
			}
		}
		else {
			se.codeid = 400;
			se.code_info = "Bad request";
		}
	}
	else if (op == "auth_workspace") {
		if (op2 == "check") {
			string &req = p.exts["request"];
			string &pwd = p.exts["passwd"];
			string mpwd = toMD5(pwd);
			if (user_table.exist(req) && user_table.get(req).toString() == mpwd) {
				if (user_logged.exist(req)) {
					utoken_table.erase(user_logged.get(req));
				}
				int_string t = AutoAllocateToken(utoken_table);
				utoken_table.append(t, req);
				user_logged.set(req, t);
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
				f.username = utoken_table.get(token);
				f.filename = fname;
				f.ownerflag = true;
				if (file_table.exist(f)) {
					file_table.erase(f);
					f.username = touid;
					// Here can only be ONE ownership
					// Also, the value set is not necessary

					// Also, ownership can only be a person,
					// not a group.
					file_table.append(f, to_string(-1));
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
				f.username = utoken_table.get(token);
				f.filename = fname;
				f.ownerflag = true;
				if (file_table.exist(f)) {
					int_fperm_key_obj g;
					g.username = touid;
					g.filename = fname;
					file_table.set(g, toperm);
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
	else {
		se.codeid = 400;
		se.code_info = "Bad Request";
	}
	// We don't support DLL execution now. It's not necessary.
sendup:;
	s.sends(se);
	se.release();
	r.release();	// It's just a copy
}