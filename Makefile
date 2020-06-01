CXX=clang++
CXXFLAGS=-Wall -Wextra -pedantic -Werror -std=c++17 -O3
LDFLAGS=$(CXXFLAGS)
LIBS=-pthread
OBJ=$(SRC:.cc=.o)

maxmem=5240
port=4000
ip=127.0.0.1
pool_ratio=0.8
pool_size=1000
key_length=5
sthreads=1
cthreads=1
iters=1000

all:  cache_server test_cache_lib generator

cache_server: cache_server.o cache_lib.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

generator: cache_client.o generator.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

threaded_generator: cache_client.o threaded_generator.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

test_threads: threaded_generator cache_server
	./cache_server -s $(ip) -p $(port) -m $(maxmem) -t $(sthreads) &
	-./threaded_generator -s $(ip) -p $(port) -m $(pool_size) -r $(pool_ratio) -k $(key_length) -i $(iters) -t $(cthreads)
	killall cache_server

test_server: generator cache_server
	./cache_server -s $(ip) -p $(port) -m $(maxmem) &
	-./generator -s $(ip) -p $(port) -m $(pool_size) -r $(pool_ratio) -k $(key_length) -b
	killall cache_server

plot: threaded_generator cache_server
	./cache_server -s $(ip) -p $(port) -m $(maxmem) -t $(sthreads) &
	-./threaded_generator  -s $(ip) -p $(port) -m $(pool_size) -r $(pool_ratio) -k $(key_length) -i $(iters) -g
	killall cache_server
	gnuplot -p "graph.gnuplot"

test_cache_lib: fifo_evictor.o test_cache_lib.o cache_lib.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.cc %.hh
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -g -c -o $@ $<

clean:
	rm -rf *.o test_cache_client generator cache_server threaded_generator

test: all
	./generator
	./cache_server -s 127.0.0.1 -p 4000 &
	./test_cache_client
	killall cache_server

valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all ./test_cache_lib
	valgrind --leak-check=full --show-leak-kinds=all ./test_evictors
