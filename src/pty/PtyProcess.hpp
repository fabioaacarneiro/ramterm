#pragma once

#include <string>
#include <vector>
#include <sys/types.h>

class PtyProcess {
public:
  PtyProcess();
  ~PtyProcess();

  PtyProcess(const PtyProcess&) = delete;
  PtyProcess& operator=(const PtyProcess&) = delete;

  bool spawn(const std::string& shell, int rows, int cols);
  void resize(int rows, int cols);

  ssize_t read(char* buffer, size_t size);
  ssize_t write(const char* buffer, size_t size);

  int masterFd() const;
  bool isRunning() const;

private:
  int masterFd_ = -1;
  pid_t childPid_ = -1;
};