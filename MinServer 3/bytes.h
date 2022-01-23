#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <set>
#include <map>
using namespace std;

//extern "C" {
	template <typename TKey, typename TValue>
	class t_safe_table;

	class __t_safe {
	public:
		virtual void lock();
#ifdef _WIN32
		__declspec(deprecated("try_lock() unsupported under Win32"))
#endif
			virtual bool try_lock();
		virtual void unlock();
		static void auto_release_thread();
	private:
		static thread_local set<__t_safe*> tstable;
		mutex m;
	};

	// Prepare for threading.
	template <typename Ty>
	class t_safe : public __t_safe {
	public:
		t_safe() {

		}
		Ty& get() {
			while (true) {
				try {
					m.lock();
					return this->data;
				}
				catch (...) {
					// A fail lock causes exception
				}
			}
		}
	private:
		Ty data;
	};

	template <typename TKey, typename TValue>
	class t_safe_table {
	public:
		using mapdata = map<TKey, TValue>;
		t_safe_table() {

		}
		size_t size() {
			return v.get().size();
		}
		bool count(TKey member) {
			return v.get().count(member);
		}
		TValue& operator [](TKey value) {
			return v.get()[value];
		}
		operator __t_safe&() {
			return this->v;
		}
		// Don't think of this ... Windows does not support
		/*mapdata& unsafe_get() {
			return v.data;
		}*/
		bool try_lock() {
			return v.try_lock();
		}
		void unlock() {
			v.unlock();
		}
	private:
		t_safe<mapdata> v;
	};


	class bytes {
	public:
		friend bytes operator + (const bytes& a, string v);
		friend bytes operator + (const bytes& a, const bytes& b);
		friend bytes operator + (const bytes& a, char b);
		friend bytes operator + (const bytes& a, const char* b);
		friend bool operator == (const bytes &a, const bytes &b);
		friend bool operator == (const bytes &a, char b);
		friend bool operator == (const bytes &a, string b);
		//	friend bool ssocket::sends(http_send& sender);
		bytes();
		bytes(string b);
		bytes(const char* b);
		bytes(char b);
		bytes(const bytes &other);
		~bytes();
		void release();
		void clear();
		void fill(char c);
		void add(const char * bytes, size_t sz);
		void erase(size_t pos, size_t count = 1);	// Erase char in specified place
		void pop_back(size_t count = 1);			// Remove from back
		char front();
		char rear();
		const char* toCharArray();
		string toString();							// Please notices that string
													// Will search '\0' and ignore informations after it automaticly.
		size_t length();
		void operator += (string v);
		void operator += (const bytes& b);
		void operator += (char b);
		char& operator [] (size_t pos);
		operator string();
		static int decst;
		// Fuck optimize
		//~bytes() = delete;
	private:
		void realloc(size_t sz);
		char *byte_space;
		size_t len;
	};

	bytes operator + (const bytes& a, string v);
	bytes operator + (const bytes& a, const bytes& b);
	bytes operator + (const bytes& a, char b);
	bytes operator + (const bytes& a, const char* b);

	bool operator == (const bytes& a, const bytes& b);
	bool operator == (const bytes& a, char b);
	bool operator == (const bytes& a, string b);

	// Used for sharing to DLL
	struct dlldata {
		string forbidden, notfound, currdir;
	};
//}

