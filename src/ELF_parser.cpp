#include <string>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "ELF_parser.h"

#ifndef ERROR_EXIT
#define ERROR_EXIT(msg) do { \
    perror(msg);             \
    exit(EXIT_FAILURE);      \
} while (0)
#endif

namespace elf_parser
{

ELF_parser::ELF_parser()
    : m_fd(-1), m_program_length(0) , m_mmap_program(nullptr) { }

ELF_parser::ELF_parser(const std::string &file_path)
{
    load_memory_map(file_path);
}

ELF_parser::ELF_parser(ELF_parser&& object)
    : m_fd(object.m_fd), m_program_length(object.m_program_length), m_mmap_program(object.m_mmap_program)
{
    object.initialize_members();
}

ELF_parser& ELF_parser::operator=(ELF_parser&& object)
{
    initialize_members(object.m_fd, object.m_program_length, object.m_mmap_program);

    object.initialize_members();
}

ELF_parser::~ELF_parser()
{
    if (m_fd != -1)
    {
        close_memory_map();
    }
}

void ELF_parser::load_file(const std::string& path_name)
{
    if (m_fd != -1)
    {
        close_memory_map();
    }
    load_memory_map(path_name);
}

void ELF_parser::load_memory_map(const std::string& path_name) const
{
    void *mmap_res;
    struct stat st;

    if ((m_fd = open(path_name.c_str(), O_RDONLY)) == -1)
    {
        ERROR_EXIT("open");
    }

    if (fstat(fd, &st) == -1)
    {
        ERROR_EXIT("fstat");
    }

    m_program_length = static_cast<std::size_t>(st.st_size);

    mmap_res = mmap((void *)0, m_program_length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mmap_res == MAP_FAILED)
    {
        ERROR_EXIT("mmap");
    }

    m_mmap_program = static_cast<std::uint8_t *>(mmap_res);
}

void ELF_parser::close_memory_map() const
{
    if (munmap(static_cast<void *>(m_mmap_program), m_program_length) == -1)
    {
        ERROR_EXIT("munmap");
    }

    if (close(m_fd) == -1)
    {
        ERROR_EXIT("close");
    }

    initialize_members();
}

void ELF_parser::initialize_members(int fd, std::size_t program_length, std::uint8_t *mmap_program)
{
    m_fd = fd;
    m_program_length = program_length;
    close_memory_map = mmap_program;
}

} // namespace elf_parser