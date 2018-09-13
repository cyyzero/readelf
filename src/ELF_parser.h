#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <cstdint>
#include <string>

namespace elf_parser
{

class ELF_parser
{
public:
    ELF_parser();
    ELF_parser(const std::string& file_path);
    ELF_parser(std::string&& file_path);
    ELF_parser(const ELF_parser& object);
    ELF_parser(ELF_parser&& object);
    ELF_parser& operator=(const ELF_parser& object);
    ELF_parser& operator=(ELF_parser&& object);
    ~ELF_parser();

    void load_file(const std::string& file_path);
    void load_file(std::string&& file_path);

    void show_file_header() const;
    void show_section_headers() const;
    void show_symbols() const;

private:
    void load_memory_map();
    void close_memory_map();
    void initialize_members(std::string file_path = std::string(),
                            int fd = -1,
                            std::size_t program_length = 0,
                            std::uint8_t *mmap_program = nullptr);

    std::string m_file_path;
    int m_fd;
    std::size_t m_program_length;
    std::uint8_t *m_mmap_program;
};

}

#endif