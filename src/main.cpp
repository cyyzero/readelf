#include "ELF_parser.h"
#include <string>
int main()
{
    using elf_parser::ELF_parser;

    ELF_parser s("./main");

    s.show_file_header();
    s.show_section_headers();
    s.show_symbols();
    return 0;
}