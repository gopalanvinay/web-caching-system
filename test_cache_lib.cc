#include "cache.hh"
#include "fifo_evictor.hh"
#include <iostream>
#include <assert.h>

const Cache::size_type max_size = 20;
Fifo* evictor = new Fifo();
Cache obj(max_size, 0.75, evictor, std::hash<key_type>());

void test_set() {
    Cache::size_type val_size;
    for (int i=0; i < 20; i++) {
        ::obj.set(std::to_string(i), "1", 1);
    }
    std::string x = "1";
    assert(::obj.get("1", val_size) == x);
    assert(::obj.get("19", val_size) == x);
    assert(::obj.get("8", val_size) == x);
    assert(::obj.get("15", val_size) == x); // change key
}

void test_get() {
    Cache::size_type val_size;
    assert(::obj.get("3", val_size) == nullptr);
}

void test_valsize() {
    Cache::size_type val_size;
    ::obj.set("a","1",10);
    ::obj.get("a", val_size); // change key
    assert(val_size == 10);
}
void test_del() {
    Cache::size_type val_size;
    ::obj.set("testDel1","5", 1);
    ::obj.del("testDel1");
    assert(::obj.get("testDel1",val_size) == nullptr);
    ::obj.set("testDel2","7",1);
    ::obj.del("testDel2");
    assert(::obj.get("testDel2",val_size) == nullptr);
}

void test_space_used() {
    ::obj.set("testSpace","3",1);
    assert(::obj.space_used() == 1);
    ::obj.set("a","30",2);
    assert(::obj.space_used() == 3);
    ::obj.del("testSpace");
    assert(::obj.space_used() == 2);
}

void test_reset() {
    Cache::size_type val_size;
    ::obj.set("testReset1","2",1);
    ::obj.set("testReset2","4",1);
    ::obj.set("testReset3","8",1);
    assert(::obj.space_used() == 3);
    ::obj.reset();
    assert(::obj.get("testRest1",val_size)== nullptr);
    assert(::obj.get("testRest2",val_size)== nullptr);
    assert(::obj.get("testRest3",val_size)== nullptr);
    assert(::obj.space_used() == 0);
}

void test_evict() {
    Cache::size_type val_size;
    for (int i=0; i < 20; i++) {
        ::obj.set(std::to_string(i), "1", 1);
    }
    // maxmem exceeded
    ::obj.set("a","1",1);
    assert(::obj.get("a", val_size) == "1");
    assert(::obj.get("0", val_size) == nullptr);
}

void test_empty_evictor() {
    Fifo* evictor = new Fifo();
    assert(evictor->evict() == "");
}

void test_touchkey() {
    //Fifo* evictor = new Fifo();
}

// Main
int main() {
    test_set();
    obj.reset();
    test_del();
    obj.reset();
    test_reset();
    obj.reset();
    test_valsize();
    obj.reset();
    test_get();
    return 0;
}
