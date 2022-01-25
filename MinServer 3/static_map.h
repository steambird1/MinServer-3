#pragma once
// Static map stores map data in file.
// As size = n,
// Add: O(1), Modify: O(n), Query: O(n), Size: O(n).
// Your key-type and value-type have to give 'string' to store them or restore in an interface.

#include "utility.h"
#include <exception>
#include <string>
#include <Windows.h>
using namespace std;

class int_static_map {
public:
	virtual string toStore() = 0;
	virtual void fromStore(string data) = 0;
};

class int_string : public int_static_map {
public:
	virtual string toStore() {
		return this->origin;
	}
	virtual void fromStore(string data) {
		this->origin = data;
	}
	string& toString() {
		return origin;
	}
	operator string&() {
		return this->origin;
	}
	int_string(string o = "") : origin(o) {

	}
private:
	string origin;
};

// This kind of object CAN'T BE SHARED DURING MULTITHREAD.
template <typename TKey, typename TValue>
class static_map {
public:

	static_assert(is_base_of<int_static_map, TKey>, "Key and value should derive from int_static_map");
	static_assert(is_base_of<int_static_map, TValue>, "Key and value should derive from int_static_map");

	class bad_key : public exception {
	public:
		virtual const char* what() const {
			return "Bad key";
		}
	};

	constexpr int default_bufsz = 4096;

	static_map(string filename, int bufsz = default_bufsz) : fn(filename) {
		this->buf1 = new char[bufsz];
		this->buf2 = new char[bufsz];
	}

	TValue get(TKey key) {
		FILE *f = fopen(fn.c_str(), "r");
		string at = key.toStore();
		if (f == NULL)
			throw bad_key();
		while (!feof(f)) {
			// Key-first
			fgets(buf1, bufsz, f);
			fgets(buf2, bufsz, f);
			if (at == sRemovingEOL(buf1)) {
				TValue v;
				v.fromStore(buf2);
				return v;
			}
		}
		fclose(f);
	}

	bool exist(TKey key) {
		try {
			get(key);
		}
		catch (bad_key ex) {
			return false;
		}
		return true;
	}
	
	size_t count() {
		FILE *f = fopen(fn.c_str(), "r");
		if (f == NULL)
			return 0;
		size_t t = 0;
		while (!feof(f)) {
			fgets(buf1, bufsz, f);
			fgets(buf2, bufsz, f);
			t++;
		}
		return t;
	}

	void append(TKey key, TValue value) {
		FILE *f = fopen(fn.c_str(), "a");
		fprintf("%s\n%s\n", key.toStore().c_str(), value.toStore().c_str());
		fclose(f);
	}

	void set(TKey key, TValue value) {
		string s = makeTemp();
		FILE *f = fopen(fn.c_str(), "r"), *g = fopen(s.c_str(), "w");
		if (f == NULL)
		{
			append(key, value);
			return;
		}
		bool flag = false;
		while (!feof(f)) {
			fgets(buf1, bufsz, f);
			fgets(buf2, bufsz, f);
			fprintf(g, "%s\n", buf1);
			if (at == sRemovingEOL(buf1)) {
				fprintf(g, "%s\n", value.toStore().c_str());
				flag = true;
			}
			else {
				fprintf(g, "%s\n", buf2);
			}
		}
		if (!flag)
			append(key, value);
	}

private:
	char *buf1, *buf2;
	string fn;
};