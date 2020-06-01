#include "evictor.hh"
#include <queue>


class Fifo : public Evictor {
    private:
        // private data structure
        std::queue<key_type> acess;                 // empty queue

    public:
        // Inform evictor that a certain key has been set or get:
        virtual void touch_key(const key_type& key);

        // Request evictor for the next key to evict, and remove it from evictor.
        // If evictor doesn't know what to evict, return an empty key ("").
        virtual const key_type evict();

        Fifo();
};
