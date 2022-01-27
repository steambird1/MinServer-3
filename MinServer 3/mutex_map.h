#pragma once

#include <map>
#include <mutex>
using namespace std;

// The mutex points to KEY, not entire map
// Usually uses for 'static DLL memory' in main.cpp
template <typename TKey, typename TValue>
class mutex_map {
public:

	class bad_key : public exception {
	public:
		virtual const char* what() const {
			return "Bad key";
		}
	};

	mutex_map() {

	}

	void lock(TKey val) {
		if (!m_map.count(val)) m_map[val];	// Construct a mutex() through this
		while (true) {
			try {
				m_map[val].lock();
				return;
			}
			catch (system_error ex) {
				this_thread::yield();
			}
		}
	}

	TValue& operator [](TKey val) {
		return v_map[val];
	}

	void unlock(TKey val) {
		m_map[val].unlock();
	}

	bool count(TKey val) {
		return m_map.count(val);
	}

	void erase(TKey val) {
		wait_lock(val);
		v_map.erase(val);
		unlock(val);
		m_map.erase(val);
	}

private:

	

	map<TKey, mutex> m_map;
	map<TKey, TValue> v_map;
};