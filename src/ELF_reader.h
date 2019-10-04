#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <cstdint>
#include <string>

namespace ELF
{

class ELF_reader
{
public:
    ELF_reader();
    explicit ELF_reader(const std::string& file_path);
    ELF_reader(const ELF_reader& object) = delete;
    ELF_reader(ELF_reader&& object) noexcept;
    ELF_reader& operator=(const ELF_reader& object) = delete;
    ELF_reader& operator=(ELF_reader&& object) noexcept;
    ~ELF_reader();

    void load_file(const std::string& file_path);

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

    std::string file_path_;
    int fd_;
    std::size_t program_length_;
    std::uint8_t *mmap_program_;
};

} // namespace ELF

#endif // ELF_PARSER_H