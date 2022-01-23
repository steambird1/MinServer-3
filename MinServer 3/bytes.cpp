#include "bytes.h"

// I love C++ ...
thread_local set<__t_safe*> __t_safe::tstable;

bytes::bytes()
{
	clear();
}

bytes::bytes(string b)
{
	clear();
	//(*this) += b;
	add(b.c_str(), b.length());
}

bytes::bytes(const char * b)
{
	clear();
	add(b, strlen(b));
}

bytes::bytes(char b)
{
	clear();
	const char t[1] = { b };
	add(t, 1);
}

bytes::bytes(const bytes & other)
{
	this->clear();
	this->add(other.byte_space, other.len);
}

int bytes::decst = 0;

bytes::~bytes()
{
	// Why do I still can't do that ...
	//release();
	decst++;
}

void bytes::release()
{
	if (this->byte_space != nullptr && this->len) {
		delete[] this->byte_space;
		this->byte_space = nullptr;
	}
	this->len = 0;
}

void bytes::clear()
{
	// Release may cause system releases unexpected pointer.
	this->byte_space = nullptr;
	this->len = 0;
	//release();
}

void bytes::fill(char c)
{
	if (!this->len)
		return;
	memset(this->byte_space, c, sizeof(char)*this->len);
}

void bytes::add(const char * bytes, size_t sz)
{
	size_t tl = this->len;
	this->realloc(tl + sz);
	memcpy(this->byte_space + tl, bytes, sizeof(char)*sz);
}

void bytes::erase(size_t pos, size_t count)
{
	printf("Erase called");	// debugger !!
	for (size_t i = pos; i < this->length(); i++) {
		size_t target = pos + count;
		this->byte_space[i] = byte_space[target];
	}
	realloc(length() - count);
}

void bytes::pop_back(size_t count)
{
	erase(length() - count, count);
}

char bytes::front()
{
	if (length())
		return this->byte_space[0];
	else
		return 0;
}

char bytes::rear()
{
	if (length())
		return byte_space[length() - 1];
	return 0;
}

const char * bytes::toCharArray()
{
	//return this->byte_space;
	if (this->len <= 0)
		return "";
	char *memspec = new char[this->len + 2];
	memset(memspec, 0, sizeof(char)*this->len);
	memspec[this->len] = char(0);
	memcpy(memspec, this->byte_space, sizeof(char)*this->len);
	return memspec;
}

string bytes::toString()
{
	if (this->len <= 0)
		return "";
	return move(string(toCharArray()));
}

size_t bytes::length()
{
	return this->len;
}

void bytes::operator+=(string v)
{
	add(v.c_str(), v.length());
	//return *this;
}

void bytes::operator+=(const bytes& b)
{
	add(b.byte_space, b.len);
	//return *this;
}

void bytes::operator+=(char b)
{
	const char c[1] = { b };
	add(c, 1);
	//return *this;
}

char & bytes::operator[](size_t pos)
{
	return this->byte_space[pos];
}

bytes::operator string()
{
	return toString();
}

void bytes::realloc(size_t sz)
{
	if (sz < this->len)
		return;
	char *bs_old = nullptr;
	if (this->len) {
		bs_old = new char[this->len + 1];
		memcpy(bs_old, this->byte_space, sizeof(char)*this->len);
		delete[] this->byte_space;		// Release old pointer, after copied
	}
	//release();
	this->byte_space = new char[sz + 2];
	memset(this->byte_space, 0, sizeof(char)*sz);
	if (this->len) {
		memcpy(byte_space, bs_old, sizeof(char)*this->len);
		delete[] bs_old;
	}
	this->byte_space[sz] = char(0);
	this->len = sz;
}

bytes operator+(const bytes& a, string v)
{
	bytes ax = bytes(a);
	ax.add(v.c_str(), v.length());
	return move(ax);
	//return w += v;
}

bytes operator+(const bytes& a, const bytes& b)
{
	bytes ax = bytes(a);
	ax.add(b.byte_space, b.len);
	return move(ax);
}

bytes operator+(const bytes& a, char b)
{
	const char c[1] = { b };
	bytes ax = bytes(a);
	ax.add(c, 1);
	return move(ax);
}

bytes operator+(const bytes& a, const char * b)
{
	return a + string(b);
}

bool operator==(const bytes& a, const bytes& b)
{
	if (a.len != b.len)
		return false;
	for (size_t i = 0; i < a.len; i++)
		if (a.byte_space[i] != b.byte_space[i])
			return false;
	return true;
}

bool operator==(const bytes &a, char b)
{
	if (a.len == 0)
		return false;
	return a.len == 1 && a.byte_space[0] == b;
}

bool operator==(const bytes &a, string b)
{
	if (a.len != b.length())
		return false;
	return b == a.byte_space;
}

void __t_safe::lock()
{
	m.lock();
	tstable.insert(this);
}

bool __t_safe::try_lock()
{
	bool b = m.try_lock();
	if (b) {
		tstable.insert(this);
	}
	return b;
}

void __t_safe::unlock()
{
	m.unlock();
	tstable.erase(this);
}

void __t_safe::auto_release_thread()
{
	for (auto &i : tstable) {
		i->unlock();
	}
}
