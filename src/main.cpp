#include "ELF_reader.h"

int main()
{
    using ELF::ELF_reader;

    ELF_reader s("./readelf");

    s.show_file_header();
    s.show_section_headers();
    s.show_symbols();
    return 0;
}