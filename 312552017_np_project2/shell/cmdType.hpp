#ifndef CMDTYPE_HPP
#define CMDTYPE_HPP
#include "pipe.hpp"
#include "util.hpp"

#include <string>
#include <vector>
#include <queue> 
#include <memory> // unique_ptr

enum class cmdType {
  EXE = 1,
  PIPE,
  FILE_CMD,
  NUMBERED_PIPE,
  USER_PIPE
};

// A basic commmand for all the derived commmands
class ICommand {
 public:
  virtual cmdType getType() const = 0;
  virtual void run() = 0; // perform functionality that will have some output
  virtual ~ICommand() {};
};

// A derived npCommand
class ExeCommand : public ICommand { 
 public:
  cmdType getType() const override { return cmdType::EXE; }
  void run() override;
  ExeCommand(std::vector<std::string>& argv) : m_argv(std::move(argv)) {}
  ~ExeCommand() override {};

 private:
  std::vector<std::string> m_argv;
};

// A basic pipe commmand
class PipeCommand : public ICommand {
 public:
  cmdType getType() const override { return cmdType::PIPE; }
  void run() override {}
  PipeCommand() {};
  virtual ~PipeCommand() {};
  
  virtual void createPipe(Pipe& currPipe);
  virtual bool pipeError() { return false; }
};

// A derived pipe commmand - Basic file redirection
class FileCommand : public PipeCommand {
 public:
  enum fileMode {
    WRITE = 1,
    READ,
    APPEND
  };
  cmdType getType() const override { return cmdType::FILE_CMD; }
  void run() override {}
  FileCommand(const std::string& filename) : m_filename(filename){}
  ~FileCommand() override {};

  void createPipe(Pipe& currPipe) override;
  static void createFileWithMode(Pipe& currPipe, const std::pair<std::string, fileMode>& file);
 private:
  std::string m_filename;
};

// 
class NumberedPipeCommand : public PipeCommand {
 public:
  cmdType getType() const override { return cmdType::NUMBERED_PIPE; }
  void run() override {}
  NumberedPipeCommand(int number, bool includeError = false) : m_number(number), m_includeError(includeError) {}
  ~NumberedPipeCommand() override {};
  
  void createPipe(Pipe& currPipe) override;
  bool pipeError() override { return m_includeError; } 

  static npshell::circular_buffer<Pipe>* numberPipeList;
 private:
  
  int m_number;
  bool m_includeError;
};

class UserPipeCommand : public PipeCommand {
 public:
  cmdType getType() const override { return cmdType::USER_PIPE; }
  void run() override {}
  UserPipeCommand() {};
  ~UserPipeCommand() override {};

};

struct NpCommandLine {
  NpCommandLine() : _readUid{0}, _writeUid{0}, file{} ,npCommands{} {}
  void addCommand(std::unique_ptr<ICommand> &&cmd) { npCommands.emplace_back(std::move(cmd)); }
  std::vector<std::unique_ptr<ICommand>>& get() { return npCommands; }
  size_t size() const { return npCommands.size(); }
  std::pair<std::string, FileCommand::fileMode> getFile() {
    auto& [fileName, fileMode] = file;
    return {fileName, fileMode};
  }
  void setFile(const std::pair<std::string, FileCommand::fileMode>& newFile) {
      const auto& [newFilename, newMode] = newFile;
      file.first = newFilename;
      file.second = newMode;
  }
  void clear() {
    _readUid = 0;
    _writeUid = 0;
    auto& [fileName, fileMode] = file;
    fileName.clear();
    fileMode = FileCommand::WRITE;
    npCommands.clear();
  }
  
  std::pair<std::string, FileCommand::fileMode> file;
  std::vector<std::unique_ptr<ICommand>> npCommands; // e.g., ls | number |1
  int _readUid;
  int _writeUid; 
};


#endif // CMDTYPE_HPP