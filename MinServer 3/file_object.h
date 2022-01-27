#pragma once

// File object with mutex
// To make sure multithreading

#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <string>
#include <mutex>
#include <map>
using namespace std;

class file_object {
public:
	file_object(string filename, string operate);
	string get_operate();
	string get_filename();
	void close();
	bool state();
	operator FILE*&();
private:
	string my_fn, my_op;
	FILE *obj = nullptr;
	bool closed = false;
	static map<string, mutex> fm_map;
};