#include "ELF_parser.h"

int main()
{
    using elf_parser::ELF_parser;

    ELF_parser s("./main");

    s.show_file_header();
    s.show_section_headers();

    return 0;
}