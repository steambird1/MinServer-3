#pragma once
#include <Windows.h>
#include <string>
#include "file_object.h"
#include "bytes.h"
using namespace std;

// Most of these utilities are from MinServer 2.

BOOL FindFirstFileExists(LPCSTR lpPath, DWORD dwFilter)
{
	WIN32_FIND_DATAA fd;
	HANDLE hFind = FindFirstFileA(lpPath, &fd);
	BOOL bFilter = (FALSE == dwFilter) ? TRUE : fd.dwFileAttributes & dwFilter;
	BOOL RetValue = ((hFind != INVALID_HANDLE_VALUE) && bFilter) ? TRUE : FALSE;
	FindClose(hFind);
	return RetValue;
}

// Is file exists?
BOOL FilePathExists(LPCSTR lpPath)
{
	return FindFirstFileExists(lpPath, FALSE) && (!FindFirstFileExists(lpPath, FILE_ATTRIBUTE_DIRECTORY));
}

bool fileExists(string path) {
	return FilePathExists(path.c_str());
}

string getExt(string cwtemp) {
	string exte = cwtemp;
	for (size_t i = cwtemp.length() - 1; i >= 0; i--) {
		if (cwtemp[i] == '.') {
			exte = exte.substr(i);
			break;
		}
	}
	return exte;
}

int random_s(void) {
	static bool firstrun = true;
	if (firstrun) {
		srand(time(NULL));
		firstrun = false;
	}
	return rand();
}

int random(int bitm = 16) {
	int n = 0;
	for (int i = 0; i <= bitm; i++) {
		if (random_s() % 2) n |= (1 << i);
	}
	return n;
}

string makeTemp(void) {
	string s;
	do {
		s = string(getenv("temp")) + "\\" + to_string(random());
	} while (fileExists(s));
	return s;
}

int getSize(string filename) {
	file_object f = file_object(filename, "r");
	if (f == NULL) return 0;
	fseek(f, 0, SEEK_END);
	int res = ftell(f);
	f.close();
	return res;
}

bytes readAll(string cwtemp) {
	int cws = getSize(cwtemp);
	file_object rs = file_object(cwtemp, "rb");
	fseek(rs, 0, SEEK_SET); // to head
	char *sending = new char[cws + 10];
	memset(sending, 0, sizeof(sending));
	fread(sending, sizeof(char), cws, rs);
	sending[cws] = '\0';
	rs.close();
	bytes b;
	b.add(sending, cws);
	return move(b);
}

string dec2hex(int n) {
	string res = "";
	while (n != 0) {
		int x = n % 16;
		if (x < 10) {
			res = char(x + '0') + res;
		}
		else {
			res = char(x + 'A' - 10) + res;
		}
		n /= 16;
	}
	return res;
}

string dec2hexw(int n) {
	string s = dec2hex(n);
	char tmp[8];
	sprintf(tmp, "%02s", s.c_str());
	return tmp;
}

string encodeBytes(bytes b) {
	string res = "";
	for (size_t i = 0; i < b.length(); i++) {
		res += "\\x" + dec2hexw(b[i]);
	}
	return res;
}

string sRemovingEOL(string s) {
	string t = s;
	while (t.length() && t[t.length() - 1] == '\n') t.pop_back();
	return move(t);
}