#include "./app/Application.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

int main() {
    try {
        Application app;
        app.run();
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Erro fatal: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}