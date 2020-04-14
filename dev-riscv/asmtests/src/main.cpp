#include "assembler.hpp"

std::string CurrentTestName;
std::vector<std::string> AvailableTests;

int main(int argc, char **argv) {
    Assembler assembler;
    assembler.output = stdout;
    assembler.test();

    if (argc == 2 && std::string{argv[1]} == "-l") {
        for (const auto &test : AvailableTests) {
            std::printf("%s\n", test.c_str());
        }
        return 0;
    }

    if (argc != 3) {
        std::fprintf(stderr, "Usage: %s <test name> <assembly output>\n\n", argv[0]);
        std::fprintf(stderr, "Available tests:\n", argv[0]);
        for (const auto &test : AvailableTests) {
            std::fprintf(stderr, " - %s\n", test.c_str());
        }
        return 1;
    }

    CurrentTestName = argv[1];

    FILE *asm_output = fopen(argv[2], "w");
    assembler.output = asm_output;
    assembler.test();
    fclose(asm_output);

    return 0;
}
