#include "cache_client.hh"
#include <stdio.h>
#include <random>
#include <vector>
#include <chrono>
#include <fstream>
#include <cstdlib>
#include <pthread.h>

typedef std::ratio<1l, 1000000l> micro;
typedef std::chrono::duration<long long, micro> microseconds;
#define timeNow() std::chrono::high_resolution_clock::now()

struct tuple{
    double percentile_latency;
    double req_per_second;
    double hits;
};

struct thread_data {
   int thread_id;
   std::string host;
   std::string port;
   std::vector<std::string> pool;
   std::vector<double> time;
   int iters;
   int hits;
   int total;
};

std::vector<std::string> generate_pool(int num, int len) {
   std::vector<std::string> pool;
   std::random_device random_device;
   std::mt19937 generator(random_device());
   std::uniform_int_distribution<> distribution(0, 25);
   for (int j=0; j<num; j++) {
        std::string key = "/";
        for(int i = 0; i < len; i++)
            key += (char)(distribution(generator) + 97);
        pool.push_back(key);
   }
   return pool;
}

http::verb generate_verb(int a=63, int b=27, int c=5) {
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(0, a+b+c);
    int x = distribution(generator);
    if (x < a) {
        return http::verb::get;
    } else if (x < a+b) {
        return http::verb::put;
    } else {
        return http::verb::delete_;
    }
}

void warm_up(Client* x, std::vector<std::string> pool, double ratio) {
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> str_distribution(0, (pool.size() * ratio)-1);
    http::request<http::empty_body> req;
    req.target("/reset");
    req.method(http::verb::post);
    assert(x->process(req)); // clear current values
    req.method(http::verb::put);
    for (int i=0; i< (int) (pool.size() * ratio); i++) {
        req.target(pool[i] + pool[str_distribution(generator)]);
        x->process(req);
    }
}

void *threaded_latencies(void *threadarg) {
    struct thread_data *local_data;
    local_data = (struct thread_data *) threadarg;
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> str_distribution(0, ((int) local_data->pool.size()) -1);
    Client* x = new Client(local_data->host, local_data->port); // create a client for each thread
    for (int i=0; i< (int) local_data->iters; i++) {
        http::request<http::empty_body> req;
        req.method(generate_verb());
        switch (req.method()) {
            case http::verb::get: //        "/key"
                req.target(local_data->pool[str_distribution(generator)]);
                break;
            case http::verb::put: //        "/key/val"
                req.target(local_data->pool[str_distribution(generator)] + local_data->pool[str_distribution(generator)]);
                break;
            case http::verb::delete_://     "/key"
                req.target(local_data->pool[str_distribution(generator)]);
                break;
            default:
                std::exit(EXIT_FAILURE);
        }
        auto t1 = timeNow(); // Time a single cache request
        bool sucsess = x->process(req);
        auto t2 = timeNow();
        double duration = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();
        local_data->time.push_back(duration / 1000); // convert to milliseconds
        if (sucsess) // don't want this to be timed
            local_data->hits += 1;
        local_data->total += 1;
    }
    delete x;
    pthread_exit(NULL);
}

tuple threaded_performance(int nthreads, int iters, std::vector<std::string> pool, std::string host, std::string port) {
    pthread_t *threads = new pthread_t[nthreads];
    struct thread_data *td = new struct thread_data[nthreads];
    int rc; // error code
    for(int i=0; i<nthreads; i++) {
        td[i].thread_id = i;
        td[i].pool = pool; // a copy of pool
        td[i].host = host;
        td[i].port = port;
        td[i].iters = iters;
        td[i].hits = 0;
        td[i].total = 0;
        rc = pthread_create(&threads[i], NULL, threaded_latencies, (void *)&td[i]);
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }
    tuple result;
    std::vector<double> total_times;
    int hits = 0;
    int total = 0;
    for(int i=0; i<nthreads; i++){
        pthread_join(threads[i], NULL); // wait for thread to finish before copying data
        hits += td[i].hits;
        total += td[i].total;
        total_times.reserve(total_times.size() + td[i].time.size());
        total_times.insert( total_times.end(), td[i].time.begin(), td[i].time.end());
    }
    result.hits = (double) hits / (double) total;
    double total_time = accumulate(total_times.begin(),total_times.end(),0.0);
    result.req_per_second = total_times.size() / total_time;
    std::sort (total_times.begin(),total_times.end());
    result.percentile_latency = total_times[(int) (total_times.size() * 0.95)];
    return result;
}

void thread_plot(int iters, int pool_size, double pool_ratio, int key_length, std::string host, std::string port) {
    std::ofstream myfile;
    std::vector<std::string> pool = generate_pool(pool_size, key_length); // only generate one pool of values
    myfile.open ("thread_latencies.csv");
    for(int i=1; i<10; i++) {
        Client* x = new Client(host, port);
        warm_up(x, pool, pool_ratio); // reset cache and initialize pool of vals each time
        delete x;
        tuple vals = threaded_performance(i, (int) iters / i, pool, host, port);
        myfile << i << "," << vals.percentile_latency << "," << vals.req_per_second << "\n";
    }
    myfile.close();
}

void thread_test(int threads, int iters, int pool_size, double pool_ratio, int key_length, std::string host, std::string port){
    std::vector<std::string> pool = generate_pool(pool_size, key_length); // only generate one pool of values
    Client* x = new Client(host, port);
    warm_up(x, pool, pool_ratio); // only initialize pool of vals once
    delete x;
    tuple result = threaded_performance(threads, (int) iters / threads, pool, host, port);
    std::cout << iters * threads <<" requests | " <<threads << " threads \n" << result.req_per_second << " req/sec | " << result.percentile_latency << "ms latency | " << result.hits * 100 << "% hits\n";
}

void basic_test(std::string host, std::string port){
    Client x(host,port);
    http::request<http::empty_body> req;
    req.method(http::verb::post);
    req.target("/reset");
    assert(x.process(req));
    req.method(http::verb::get);
    req.target("/key");
    assert(x.process(req) == false);
    req.method(http::verb::get);
    req.target("/no");
    assert(x.process(req) == false);
    req.method(http::verb::put);
    req.target("/key/10");
    assert(x.process(req) == true); // put always true
    req.method(http::verb::get);
    req.target("/key");
    assert(x.process(req) == true);
    req.method(http::verb::delete_);
    req.target("/key");
    assert(x.process(req) == true);
    req.method(http::verb::post);
    req.target("/reset");
    assert(x.process(req));
}

int main(int argc, char* argv[]) {
    // default values
    std::string port = "4000";
    std::string host = "127.0.0.1";
    int pool_size = 1024;
    int key_length = 5;
    int iters = 1000;
    double pool_ratio = 0.8;
    uint8_t threads = 1;

    int opt;
    while((opt = getopt(argc, argv, "p:s:m:i:r:t:k:gh")) != -1) {
        switch(opt) {
            case 'p':
                port = optarg;
                break;
            case 's':
                host = optarg;
                break;
            case 'm':
                pool_size = atoi(optarg);
                break;
            case 'i':
                iters = atoi(optarg);
                break;
            case 'r':
                pool_ratio = strtod(optarg, nullptr);
                break;
            case 'k':
                key_length = atoi(optarg);
                break;
            case 't':
                threads = atoi(optarg);
                basic_test(host, port);
                thread_test(threads, iters, pool_size, pool_ratio, key_length, host, port);
                break;
            case 'g':
                thread_plot(iters, pool_size, pool_ratio, key_length, host, port);
                break;
            case 'h':
                std::cout << "-p [string] : port\n-s [string] : host\n-m [int] : pool size\n-r [decimal] : pool ratio\n-k [int] : key length\n-i [int] : iterations\n-t [int] : threads\n-g : graph\n-h : help\n";
                return 1;
        }
    }
    /* Last thing that main() should do */
    pthread_exit(NULL);
}
