#pragma once
#include "md5.h"

string toMD5(string data) {
	MD5 m = MD5(data);
	return m.toString();
}