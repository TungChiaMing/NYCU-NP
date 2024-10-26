#ifndef UTIL_HPP
#define UTIL_HPP
#define CIRCULAR_BUFFER_SIZE 1024

#include <vector>
#include <cstddef> // size_t
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
}
#endif // UTIL_HPP