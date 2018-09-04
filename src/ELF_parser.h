#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <cstdint>

class std::string;

namespace elf_parser
{

class ELF_parser
{
public:
    ELF_parser();
    ELF_parser(const std::string& file_path);
    ELF_parser(const ELF_parser&) = delete;
    ELF_parser(ELF_parser&& object);

    ELF_parser& operator=(const ELF_parser&) = delete;
    ELF_parser& operator=(ELF_parser&& object);
    ~ELF_parser();

    void load_file(const std::string& file_path);

private:
    void load_memory_map(const std::string& file_path);

    void initialize_members(int fd = -1, std::size_t program_length = 0, std::uint8_t *mmap_program = nullptr);
    
    //std::string m_file_name;
    int m_fd;
    std::size_t m_program_length;
    std::uint8_t *m_mmap_program;
};

}

#endif