#include <sys/time.h>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <map>
#include <random>
#include <iostream>
#include <stdlib.h>
#include <MurmurHash3.h>

// test will enable main function and start multithreaded test
//#define TEST

#ifndef TEST
#include <boost/python.hpp>
#endif

#define uint128 unsigned __int128
typedef unsigned int uint;

using namespace std;

const std::chrono::milliseconds garbage_collector_sleep(1000);




/**
 * @brief The lockGIL class
 * 
 * RAII class used to lock and unlock the GIL - Global Interpreter Lock
 * 
 * because at any point in time, only one thread
 * is allowed to perform operations on Python objects
 * 
 * https://wiki.python.org/moin/GlobalInterpreterLock
 */
#ifndef TEST
class lockGIL {
public:
	lockGIL() {
		state = PyGILState_Ensure();
	}
	~lockGIL() {
		PyGILState_Release(state);
	}
	
private:
	PyGILState_STATE state;
};
#endif



/**
 * @brief getCurrentTS
 * @return timestamp
 */
inline uint getCurrentTS() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return uint(tv.tv_sec);
}


template <typename T>
/**
 * @brief The data struct
 */
struct data {
	/// the actual cached object
	shared_ptr<T> p;

	/// the insertion timestamp
	uint ts = 0;

	/// the given TTL
	uint TTL = 60;

	/// if this is true, delete it the next GC cycle
	bool deleted = false;
};






template <typename key, typename T>
/**
 * @brief The SuperCache class
 *
 * implements singleton instance() for direct access,
 * use functions set(), get(), remove() and erase() for manipulating with the cache
 */
class SuperCache {
	typedef data<T> data_t;
	typedef map<key,data_t> super_map;

public:
	SuperCache() {
		init();
	}

	/**
	 * @brief init
	 *
	 * start garbage collector in separated thread
	 */
	void init() {
		collector = new thread(&SuperCache::garbage_collector,this);
		collector->detach();
	}


	/**
	 * @brief instance - Singleton
	 * @return static object
	 */
	static SuperCache& instance() {
		static SuperCache instance;
		return instance;
	}
	// disable copy constructor
	SuperCache(SuperCache const&)  = delete;
	// disable operator of assignment
	void operator=(SuperCache const&) = delete;

	bool set(key const &k, const T &p, uint const &TTL);
	T get(const key &k, const T &default_value = T());
	void remove(key const &k);
	void erase();


private:
	void garbage_collector();
	void deleteFromMap();
	
	/**
	 * @brief deleteCondition - used in garbage collector
	 * @param iter
	 * @return true means item will be removed
	 */
	bool deleteCondition(typename super_map::iterator iter) {
		return (iter->second.deleted || ((iter->second.TTL)&&(iter->second.ts + iter->second.TTL < getCurrentTS())));
	}

	/// every access to our_map should be locked
	mutex our_mutex;
	/// main storage 
	super_map our_map;
	/// garbage collector thread
	thread* collector = nullptr;
};





template <typename key, typename T>
/**
 * @brief SuperCache<key, T>::set - set or update an item
 * @param k unique key of the item to be stored
 * @param p stored item
 * @param TTL Time To Live in seconds
 * @return true if the item was already present
 */
bool SuperCache<key, T>::set(key const &k, const T &p, uint const &TTL) {
	bool res = true;

	data_t temp;
	temp.p = make_shared<T>(p);
	temp.TTL = TTL;
	temp.ts = getCurrentTS();

	our_mutex.lock();

	//Search if the old item exists
	auto iter = our_map.find(k);
	if (iter == our_map.cend()) {
		res = false;
		our_map.insert(make_pair(k,temp));
	} else {
		iter->second = temp;
	}

	our_mutex.unlock();

	return res;
}

template <typename key, typename T>
/**
 * @brief SuperCache<key, T>::get
 * @param k unique key of stored item
 * @param default_value
 * @return stored item or default value if key is not found
 */
T SuperCache<key, T>::get(key const & k, const T &default_value) {

	T temp;

	our_mutex.lock();

	auto iter = our_map.find(k);
	if (iter != our_map.cend() && iter->second.deleted==false) {
		// item was found
		temp = *(iter->second.p);
	} else {
		// item was NOT found, return default
		temp = default_value;
	}
	
	our_mutex.unlock();

	return temp;
}

template <typename key, typename T>
/**
 * @brief SuperCache<key, T>::remove - delete an item
 * @param k key
 */
void SuperCache<key, T>::remove(key const &k) {

	our_mutex.lock();

	auto iter = our_map.find(k);
	if (iter != our_map.cend()) {
		// mark as deleted
		// garbage collector will take care of it
		iter->second.deleted = true;
	}

	our_mutex.unlock();
}

template <typename key, typename T>
/**
 * @brief SuperCache<key, T>::erase
 *
 * delete all items
 */
void SuperCache<key, T>::erase() {

	our_mutex.lock();
	our_map.clear();
	our_mutex.unlock();
}

template <typename key, typename T>
/**
 * @brief SuperCache<key, T>::garbage_collector
 *
 * infinite loop running in separated thread
 */
void SuperCache<key, T>::garbage_collector() {

	// infinite loop
	while (true) {
		deleteFromMap();
		this_thread::sleep_for(garbage_collector_sleep);
	}
}

template <typename key, typename T>
/**
 * @brief SuperCache<key, T>::deleteFromMap
 *
 * iterate all items and delete if old ts or remove() was called
 */
void SuperCache<key, T>::deleteFromMap() {
	typename super_map::iterator iter = our_map.begin();
	
	// check every item
	while (iter != our_map.end()) {
		if (deleteCondition(iter)) {

			our_mutex.lock();
			// shared pointer, no delete
			{
				// lock the GIL, so we can work from this thread with python objects
				#ifndef TEST
				lockGIL lock;
				#endif
				iter = our_map.erase(iter);
			}
			// lockGIL lock released
			our_mutex.unlock();
			
			continue;
		}
		
		++iter;
	}
}






#ifdef TEST
/**
 * @brief The stored_item struct
 */
struct stored_item {
	string key;
	string value;
};

/**
 * @brief loopSet
 */
void loopSet() {
	while(1) {
		uint r = rand() % 100000 +1;
		string kk;
		kk = to_string(r);

		uint128 hash = 0;
		MurmurHash3_x64_128(kk.c_str(),sizeof(kk.c_str())*sizeof(char),0,(void*)&hash);

		/////////////////////////////////////////////////////
		// way to print __int128 number
		
		// uint64_t low = (uint64_t)hash;
		// uint64_t high = (hash >> 64);
		// cout << low << high << endl;

		stored_item item;
		item.key = kk;
		item.value = kk;
		uint t = 10;

		SuperCache<uint128, stored_item>::instance().set(hash, item, t);

		this_thread::sleep_for (std::chrono::microseconds(10));
	}
}

/**
 * @brief loopGet
 */
void loopGet() {
	while(1) {
		uint r = rand() % 100000 +1;
		string kk;
		kk = to_string(r);

		uint128 hash = 0;
		MurmurHash3_x64_128(kk.c_str(),sizeof(kk.c_str())*sizeof(char),0,(void*)&hash);

		stored_item res = SuperCache<uint128, stored_item>::instance().get(hash);
		if (res.key != "" && res.key != kk) {
			cout << "real not ok " << res.key << ":" << kk << endl;
		}
		cout << "  GET  " << res.key << endl;

		this_thread::sleep_for (std::chrono::milliseconds(100));
	}
}




/**
 * @brief main
 * @return
 */
int main() {
	
	srand (time(NULL));

	thread * test_1 = new thread(&loopSet);
	thread * test_2 = new thread(&loopSet);
	thread * test_3 = new thread(&loopSet);
	thread * test_4 = new thread(&loopSet);

	thread * get_test = new thread(&loopGet);


	while(1){}
	
	return 0;
}
#endif






// for C++ binding in python
#ifndef TEST
using namespace boost;

// thread safe global variables
thread_local uint128 hash_key = 0;
thread_local uint TTL = 0;
thread_local string object_classname;
thread_local python::object o;

/**
 * @brief The stored_item struct
 */
struct stored_item {
	string key;
	python::object value;
};

/**
 * @brief python_set
 * @param key
 * @param value
 * @param timeout
 */
void python_set(char const * const key, python::object const &value, python::object const &timeout) {

	// get timeout
	TTL = 60;
	// is this object int or None?
	python::extract<python::object> objectExtractor(timeout);
	o = objectExtractor();
	object_classname = python::extract<string>(o.attr("__class__").attr("__name__"));
	if (object_classname == "int") {
		TTL = python::extract<long>(timeout);
		// Django: A timeout of 0 won’t cache the value.
		if (TTL == 0) {
			return;
		}
	}
	
	// if None cache the value forever
	if (object_classname == "object") {
		TTL = 0;
	}
	
	// generate hash
	// because the key generated by Django is small,
	// we can also use MurmurHash3_x86_32
	// performance test is suggested
	hash_key = 0;
	MurmurHash3_x64_128(key,strlen(key)*sizeof(char),0,(void*)&hash_key);

	// create item
	stored_item new_item;
	new_item.key = key;
	new_item.value = value;

	SuperCache<uint128, stored_item>::instance().set(hash_key, new_item, TTL);
	
}


/**
 * @brief python_get
 * @param key
 * @param default_value (default is None object)
 * @return python object
 */
python::object python_get(char const * const key, python::object const &default_value = python::object()) {

	// generate hash
	hash_key = 0;
	MurmurHash3_x64_128(key,strlen(key)*sizeof(char),0,(void*)&hash_key);

	stored_item default_item;
	default_item.key = key;
	default_item.value = default_value;

	stored_item res = SuperCache<uint128, stored_item>::instance().get(hash_key, default_item);
	if (res.key != key) {
		// returns the None object
		return python::object();
	}
	
	return res.value;
}

/**
 * @brief python_delete
 * @param key
 */
void python_delete(char const * const key) {

	// generate hash
	hash_key = 0;
	MurmurHash3_x64_128(key,strlen(key)*sizeof(char),0,(void*)&hash_key);

	SuperCache<uint128, stored_item>::instance().remove(hash_key);
}

/**
 * @brief python_clear
 */
void python_clear() {
	SuperCache<uint128, stored_item>::instance().erase();
}


BOOST_PYTHON_MODULE(supercache) {
	// python interface
	python::def("set", &python_set);
	python::def("get", &python_get);
	python::def("delete", &python_delete);
	python::def("clear", &python_clear);
}
#endif
