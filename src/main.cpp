#include "./app/Application.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

static std::string findIconPath(int argc, char* argv[]) {
  std::string fallback;
  if (argc > 0 && argv[0] && argv[0][0] != '\0') {
    try {
      namespace fs = std::filesystem;
      fs::path exe = fs::absolute(fs::path(argv[0]).lexically_normal());
      fs::path dir = exe.parent_path();
      fs::path icon = (dir / "../assets/ramterm-logo.png").lexically_normal();
      if (fs::is_regular_file(icon)) return icon.string();
      icon = (dir / "assets/ramterm-logo.png").lexically_normal();
      if (fs::is_regular_file(icon)) return icon.string();
    } catch (...) {}
  }
  return fallback;
}

int main(int argc, char* argv[]) {
  try {
    Application app(findIconPath(argc, argv));
    app.run();
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr << "Erro fatal: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}