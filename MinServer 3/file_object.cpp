#include "file_object.h"

// Allocate memory for it
map<string, mutex> file_object::fm_map;

file_object::file_object(string filename, string operate)
{
	this->my_fn = filename;
	this->my_op = operate;
	fm_map.emplace(filename,mutex());
	while (true) {
		try {
			fm_map[filename].lock();
			return;
		}
		catch (system_error ex) {
			this_thread::yield();
		}
	}
}

string file_object::get_operate()
{
	return this->my_op;
}

string file_object::get_filename()
{
	return this->my_fn;
}

void file_object::close()
{
	fclose(obj);
	this->closed = true;
	fm_map[this->my_fn].unlock();
	fm_map.erase(this->my_fn);
}

bool file_object::state()
{
	return !this->closed;
}

file_object::operator FILE*&()
{
	return this->obj;
}
