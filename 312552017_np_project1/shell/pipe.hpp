#ifndef PIPE_HPP
#define PIPE_HPP
#include <array>

class Pipe {
 public:
  Pipe() ; 
  void create();
  void openFile(const char* filename, short mode);
  void clear();
  void closePipe();
  void read();
  void write(bool writeErr = false);
  bool empty();
  int operator[](int i) { return fd[i]; }
  void set(int in, int out);
  void readEOF();
 private:
  std::array<int, 2> fd;
  static const int in = 0;
  static const int out = 1;
};

#endif // PIPE_HPP
