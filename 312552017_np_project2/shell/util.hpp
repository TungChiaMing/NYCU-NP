#ifndef UTIL_HPP
#define UTIL_HPP
#define CIRCULAR_BUFFER_SIZE 1024

#include <vector>
#include <cstddef> // size_t
#include <sys/mman.h> // mmap
#include <string.h> // memset
namespace npshell {
  template <typename T>
  class circular_buffer {
   public:
     circular_buffer() : head{0}, buffer{} {}
    void set(size_t n, const T& item) { buffer[(head + n) % CIRCULAR_BUFFER_SIZE] = item; }
    T& get(size_t n) { return buffer[(head + n) % CIRCULAR_BUFFER_SIZE]; }
    T& front() { return buffer[head % CIRCULAR_BUFFER_SIZE]; }
    void next() {
      ++head;
      head %= CIRCULAR_BUFFER_SIZE;
    }

   private:
    size_t head;
    std::array<T, CIRCULAR_BUFFER_SIZE> buffer; 
    
  };

  template <typename T, int shmSize>
  class Memmap {
  public:
    Memmap() { }
    Memmap(int _shmfd) {  data = static_cast<T*>(mmap(nullptr, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, _shmfd, 0)); }
    ~Memmap() { munmap(data, shmSize); }

    void* getData() { return data; } 
    void setData(int _shmfd) { data = static_cast<T*>(mmap(nullptr, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, _shmfd, 0)); }
    // pthread_rwlock_t* getLock() { return &static_cast<T*>(data)->lock; }
  private:
    T* data;
  };
}
#endif // UTIL_HPP