#include "pipe.hpp"
#include "cmdType.hpp"

#include <unistd.h> // pipe
#include <fcntl.h> // open
#include <cstdlib> // exit
#include <cstdio> // perror
#include <sys/stat.h> // mkfifo
#include <iostream>

Pipe::Pipe() : fd{-1, -1} {}


void Pipe::create() {
  if (pipe2(fd.data(), O_CLOEXEC) == -1) {
      perror("pipe");
      exit(EXIT_FAILURE);
  }
}

void Pipe::openFile(const char* filename, short mode) {
  switch (mode)
  {
  case FileCommand::WRITE:
    fd[in] = -1;
    fd[out] = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    break;
  case FileCommand::READ:
    fd[in] = open(filename, O_RDONLY);
    fd[out] = -1;
    break;
  case FileCommand::APPEND:
    fd[in] = -1;
    fd[out] = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    break;
  default:
    break;
  }
}

bool Pipe::openFIFO(const char* filename, short mode) {
  bool flag = false;
  switch (mode)
  {
    case FileCommand::WRITE:
      if(access(filename, F_OK ) == -1) {
        mkfifo(filename, 0644);
        fd[out] = open(filename, O_RDWR );
        flag = true;
      } else {

        flag = false;
      }
      break;
    case FileCommand::READ:
      if(access(filename, F_OK ) != -1) {
        fd[in] = open(filename, O_RDONLY | O_NONBLOCK);
        flag = true;
      } else {

        flag = false;
      }
      break;
  }
  return flag;
}
void Pipe::clear() { fd[in] = fd[out] = -1; }

void Pipe::closePipe() { 
  if (fd[in] != -1) close(fd[in]);
  if (fd[out] != -1) close(fd[out]);
  clear();
}

void Pipe::read() {
  dup2(fd[in], STDIN_FILENO);
  closePipe();
}

void Pipe::write(bool writeErr) {
  dup2(fd[out], STDOUT_FILENO);
  if (writeErr) dup2(fd[out], STDERR_FILENO);
  closePipe();
}

bool Pipe::empty() {
  return fd[in] == -1 && fd[out] == -1;
}

void Pipe::set(int inFd, int outFd) {
  fd[in] = inFd;
  fd[out] = outFd;
}

void Pipe::readEOF() {  
  close(fd[out]);
  char buffer[1024];
  while (::read(fd[in], buffer, sizeof(buffer)) > 0);
  close(fd[in]);
} 