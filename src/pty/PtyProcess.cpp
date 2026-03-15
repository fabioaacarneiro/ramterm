#include "pty/PtyProcess.hpp"

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <fcntl.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

PtyProcess::PtyProcess() = default;

PtyProcess::~PtyProcess() {
  if (masterFd_ >= 0) {
    close(masterFd_);
    masterFd_ = -1;
  }
}

bool PtyProcess::spawn(const std::string& shell, int rows, int cols) {
  struct winsize ws{};
  ws.ws_row = static_cast<unsigned short>(rows);
  ws.ws_col = static_cast<unsigned short>(cols);
  ws.ws_xpixel = 0;
  ws.ws_ypixel = 0;

  childPid_ = forkpty(&masterFd_, nullptr, nullptr, &ws);
  if (childPid_ < 0) {
    return false;
  }

  if (childPid_ == 0) {
    setenv("TERM", "xterm-256color", 1);
    execlp(shell.c_str(), shell.c_str(), static_cast<char*>(nullptr));
    _exit(127);
  }

  const int flags = fcntl(masterFd_, F_GETFL, 0);
  if (flags >= 0) {
    fcntl(masterFd_, F_SETFL, flags | O_NONBLOCK);
  }

  return true;
}

void PtyProcess::resize(int rows, int cols) {
  if (masterFd_ < 0) {
    return;
  }

  struct winsize ws{};
  ws.ws_row = static_cast<unsigned short>(rows);
  ws.ws_col = static_cast<unsigned short>(cols);
  ws.ws_xpixel = 0;
  ws.ws_ypixel = 0;

  ioctl(masterFd_, TIOCSWINSZ, &ws);

  if (childPid_ > 0) {
    kill(childPid_, SIGWINCH);
  }
}

ssize_t PtyProcess::read(char* buffer, size_t size) {
  if (masterFd_ < 0) {
    return -1;
  }
  return ::read(masterFd_, buffer, size);
}

ssize_t PtyProcess::write(const char* buffer, size_t size) {
  if (masterFd_ < 0) {
    return -1;
  }
  return ::write(masterFd_, buffer, size);
}

int PtyProcess::masterFd() const {
  return masterFd_;
}

bool PtyProcess::isRunning() const {
  if (childPid_ <= 0) {
    return false;
  }

  int status = 0;
  const pid_t result = waitpid(childPid_, &status, WNOHANG);
  return result == 0;
}