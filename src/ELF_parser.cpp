#include <string>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <elf.h>
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

ELF_parser::ELF_parser(const std::string& file_path)
    : m_file_path(file_path)
{
    load_memory_map();
}

ELF_parser::ELF_parser(std::string&& file_path)
    : m_file_path(std::move(file_path))
{
    load_memory_map();
}

ELF_parser::ELF_parser(const ELF_parser& object)
    : m_file_path(object.m_file_path)
{
    load_memory_map();
}

ELF_parser::ELF_parser(ELF_parser&& object)
    : m_file_path(std::move(object.m_file_path)), m_fd(object.m_fd),
    m_program_length(object.m_program_length), m_mmap_program(object.m_mmap_program)
{
    object.initialize_members();
}

ELF_parser& ELF_parser::operator=(const ELF_parser& object)
{
    initialize_members(object.m_file_path, object.m_fd, object.m_program_length,
                       object.m_mmap_program);
    return *this;
}

ELF_parser& ELF_parser::operator=(ELF_parser&& object)
{
    initialize_members(std::move(object.m_file_path), object.m_fd,
                       object.m_program_length, object.m_mmap_program);

    object.initialize_members();
    return *this;
}

ELF_parser::~ELF_parser()
{

    close_memory_map();
}

void ELF_parser::load_file(const std::string& path_name)
{
    m_file_path = path_name;
    close_memory_map();
    load_memory_map();
}

void ELF_parser::load_file(std::string&& file_path)
{
    m_file_path = std::move(file_path);
    close_memory_map();
    load_memory_map();
}

void ELF_parser::show_file_header() const
{
    auto file_header = reinterpret_cast<const Elf64_Ehdr *const>(m_mmap_program);

    if (file_header->e_ident[EI_MAG0] != ELFMAG0 || file_header->e_ident[EI_MAG1] != ELFMAG1 ||
        file_header->e_ident[EI_MAG2] != ELFMAG2 || file_header->e_ident[EI_MAG3] != ELFMAG3)
    {
        printf("It's not a ELF file.\n");
        return;
    }

    if (file_header->e_ident[EI_CLASS] != ELFCLASS64)
    {
        printf("It only support 64-bit architecture now!\n");
        return;
    }

    printf("ELF Header:\n");
    printf("  Magic:  ");
    for (int i = 0; i < EI_NIDENT; ++i)
    {
        printf(" %02x", file_header->e_ident[i]);
    }
    printf("\n");

    printf("  Class:                             ");
    switch (file_header->e_ident[EI_CLASS])
    {
    case ELFCLASSNONE:
        printf("INVALID\n");
        break;
    case ELFCLASS32:
        printf("ELF32\n");
        break;
    case ELFCLASS64:
        printf("Elf64\n");
        break;
    default:
        printf("Unknown class");
        break;
    }

    printf("  Data:                              ");
    switch (file_header->e_ident[EI_DATA])
    {
    case ELFDATANONE:
        printf("unknown data format\n");
        break;
    case ELFDATA2LSB:
        printf("2's complement, little-endian\n");
        break;
    case ELFDATA2MSB:
        printf("2's complement, big-endian\n");
        break;
    default:
        printf("error data format\n");
        break;
    }

    printf("  Version:                           ");
    printf("%d\n", (int)file_header->e_ident[EI_ABIVERSION]);

    printf("  OS/ABI:                            ");
    switch (file_header->e_ident[EI_OSABI])
    {
    case ELFOSABI_SYSV:
        printf("UNIX System V ABI\n");
        break;
    case ELFOSABI_HPUX:
        printf("HP-UX ABI\n");
        break;
    case ELFOSABI_NETBSD:
        printf("NetBSD ABI\n");
        break;
    case ELFOSABI_LINUX:
        printf("Linux ABI\n");
        break;
    case ELFOSABI_SOLARIS:
        printf("Solaris ABI\n");
        break;
    case ELFOSABI_IRIX:
        printf("IRIX ABI\n");
        break;
    case ELFOSABI_FREEBSD:
        printf("FreeBSD ABI\n");
        break;
    case ELFOSABI_TRU64:
        printf("TRU64 UNIX ABI\n");
        break;
    case ELFOSABI_ARM:
        printf("ARM architecture ABI\n");
        break;
    case ELFOSABI_STANDALONE:
        printf("Stand-alone (embedded) ABI\n");
        break;
    default:
        printf("error\n");
        break;
    }

    printf("  Type:                              ");
    switch (file_header->e_type)
    {
    case ET_NONE:
        printf("unknonw type\n");
        break;
    case ET_REL:
        printf("relocatable file\n");
        break;
    case ET_EXEC:
        printf("executable file\n");
        break;
    case ET_DYN:
        printf("shared object\n");
        break;
    case ET_CORE:
        printf("core file\n");
        break;
    default:
        printf("error\n");
        break;
    }

    printf("  Machine:                           ");
    switch (file_header->e_machine)
    {
    case EM_NONE:
        printf("unknown machine\n");
        break;
    case EM_M32:
        printf("AT&T WE 32100\n");
        break;
    case EM_SPARC:
        printf("Sun Microsystems SPARC\n");
        break;
    case EM_386:
        printf("Intel 80386\n");
        break;
    case EM_68K:
        printf("Motorola 68000\n");
        break;
    case EM_88K:
        printf("Motorola 88000\n");
        break;
    case EM_860:
        printf("Intel 80860\n");
        break;
    case EM_MIPS:
        printf("MIPS RS3000 (big-endian only)\n");
        break;
    case EM_PARISC:
        printf("HP/PA\n");
        break;
    case EM_SPARC32PLUS:
        printf("SPARC with enhanced instruction set\n");
        break;
    case EM_PPC:
        printf("PowerPC\n");
        break;
    case EM_PPC64:
        printf("PowerPC 64-bit\n");
        break;
    case EM_S390:
        printf("IBM S/390\n");
        break;
    case EM_ARM:
        printf("Advanced RISC Machines\n");
        break;
    case EM_SH:
        printf("Renesas SuperH\n");
        break;
    case EM_SPARCV9:
        printf("SPARC v9 64-bit\n");
        break;
    case EM_IA_64:
        printf("Intel Itanium\n");
        break;
    case EM_X86_64:
        printf("AMD x86-64\n");
        break;
    case EM_VAX:
        printf("DEC Vax\n");
        break;
    default:
        printf("error\n");
        break;
    }

    printf("  Version:                           ");
    switch (file_header->e_ident[EI_VERSION])
    {
    case EV_NONE:
        printf("invalid version\n");
        break;
    case EV_CURRENT:
        printf("current version\n");
        break;
    default:
        printf("error version\n");
        break;
    }

    printf("  Entry point address:               ");
    printf("0x%lx\n", file_header->e_entry);

    printf("  Start of program headers:          ");
    printf("%ld (bytes into file)\n", file_header->e_phoff);

    printf("  Start of section headers:          ");
    printf("%ld (bytes into file)\n", file_header->e_shoff);

    printf("  Flags:                             ");
    printf("0x%x\n", file_header->e_flags);

    printf("  Size of this header:               ");
    printf("%d (bytes)\n", file_header->e_ehsize);

    printf("  Size of program headers:           ");
    printf("%d (bytes)\n", file_header->e_phentsize);

    printf("  Number of program headers:         ");
    printf("%d\n", file_header->e_phnum < PN_XNUM ? file_header->e_phnum : 
                   ((Elf64_Shdr *)(&m_mmap_program[file_header->e_shoff]))->sh_info);

    printf("  Size of section headers:           ");
    printf("%d (bytes)\n", file_header->e_shentsize);

    printf("  Number of section headers:         ");
    auto shnum = reinterpret_cast<const Elf64_Shdr *const>(&m_mmap_program[file_header->e_shoff])->sh_size;
    if (shnum == 0)
    {
        printf("%d\n", file_header->e_shnum);
    }
    else
    {
        printf("%ld\n", shnum);
    }

    printf("  Section header string table index: ");
    switch (file_header->e_shstrndx)
    {
    case SHN_UNDEF:
        printf("undefined value\n");
        break;
    case SHN_XINDEX:
        printf("%d\n", reinterpret_cast<const Elf64_Shdr *const>(&m_mmap_program[file_header->e_shoff])->sh_link);
        break;
    default:
        printf("%d\n", file_header->e_shstrndx);
        break;
    }
}

void ELF_parser::show_section_headers() const
{
    const Elf64_Ehdr *const file_header = reinterpret_cast<const Elf64_Ehdr * const>(m_mmap_program);
    const Elf64_Shdr *const shdr = reinterpret_cast<const Elf64_Shdr *const>(m_mmap_program + file_header->e_shoff);;
    const char *const section_name_table = reinterpret_cast<const char *const>(&m_mmap_program[shdr[file_header->e_shstrndx].sh_offset]);;
    Elf64_Xword shnum;

    shnum = reinterpret_cast<const Elf64_Shdr *const>(&m_mmap_program[file_header->e_shoff])->sh_size;
    if (shnum == 0)
    {
        shnum = file_header->e_shnum;
    }

    printf("There are %ld section header%s, starting at offset 0x%lx:\n\n", shnum, 
           shnum > 1 ? "s" : "", file_header->e_shoff);
    printf("Section Headers:\n"
           "  [Nr] Name              Type             Address           Offset\n"
           "       Size              EntSize          Flags  Link  Info  Align\n");
    for (int i = 0; i < shnum; ++i)
    {
        printf("  [%2d] ", i);
        printf("%-16s  ", section_name_table+shdr[i].sh_name);
        switch (shdr[i].sh_type)
        {
        case SHT_NULL:
            printf("NULL             ");
            break;
        case SHT_PROGBITS:
            printf("PROGBITS         ");
            break;
        case SHT_SYMTAB:
            printf("SYMTAB           ");
            break;
        case SHT_STRTAB:
            printf("STRTAB           ");
            break;
        case SHT_RELA:
            printf("RELA             ");
            break;
        case SHT_HASH:
            printf("HASH             ");
            break;
        case SHT_NOTE:
            printf("NOTE             ");
            break;
        case SHT_REL:
            printf("REL              ");
            break;
        case SHT_SHLIB:
            printf("SHLIB            ");
            break;
        case SHT_DYNAMIC:
            printf("DYNSYM           ");
            break;
        default:
            printf("Unknown          ");
            break;
        }
        printf("%016lx  ", shdr[i].sh_addr);
        printf("%08lx\n", shdr[i].sh_offset);
        printf("       %016lx  ", shdr[i].sh_size);
        printf("%016lx ", shdr[i].sh_entsize);

        std::string flags;
        if (shdr[i].sh_flags & SHF_WRITE);
            flags.push_back('W');
        if (shdr[i].sh_flags & SHF_ALLOC);
            flags.push_back('A');
        if (shdr[i].sh_flags & SHF_EXECINSTR)
            flags.push_back('X');
        if (shdr[i].sh_flags & SHF_MERGE)
            flags.push_back('M');
        if (shdr[i].sh_flags & SHF_STRINGS)
            flags.push_back('S');
        if (shdr[i].sh_flags & SHF_INFO_LINK)
            flags.push_back('I');
        if (shdr[i].sh_flags & SHF_LINK_ORDER)
            flags.push_back('L');
        if (shdr[i].sh_flags &  SHF_INFO_LINK)
            flags.push_back('I');
        if (shdr[i].sh_flags & SHF_OS_NONCONFORMING)
            flags.push_back('O');
        if (shdr[i].sh_flags & SHF_GROUP)
            flags.push_back('G');
        if (shdr[i].sh_flags & SHF_TLS)
            flags.push_back('T');
        if (shdr[i].sh_flags & SHF_COMPRESSED)
            flags.push_back('l');
        if (shdr[i].sh_flags & SHF_MASKOS)
            flags.push_back('o');
        if (shdr[i].sh_flags & SHF_MASKPROC)
            flags.push_back('p');
        if (shdr[i].sh_flags & SHF_ORDERED)
            flags.push_back('x');
        if (shdr[i].sh_flags & SHF_EXCLUDE)
            flags.push_back('E');

        std::sort(flags.begin(), flags.end());
        printf("%5s  ", flags.c_str());
        printf("%4d  ", shdr[i].sh_link);
        printf("%4d  ", shdr[i].sh_info);
        printf("%4lu\n", shdr[i].sh_addralign);
    }
    printf("Key to Flags:\n"
           "  W (write), A (alloc), X (execute), M (merge), S (strings), l (large)\n"
           "  I (info), L (link order), G (group), T (TLS), E (exclude), x (unknown)\n"
           "  O (extra OS processing required) o (OS specific), p (processor specific)\n");

}

void ELF_parser::load_memory_map()
{
    void *mmap_res;
    struct stat st;

    if ((m_fd = open(m_file_path.c_str(), O_RDONLY)) == -1)
    {
        ERROR_EXIT("open");
    }

    if (fstat(m_fd, &st) == -1)
    {
        ERROR_EXIT("fstat");
    }

    m_program_length = static_cast<std::size_t>(st.st_size);

    mmap_res = mmap((void *)0, m_program_length, PROT_READ, MAP_PRIVATE, m_fd, 0);
    if (mmap_res == MAP_FAILED)
    {
        ERROR_EXIT("mmap");
    }

    m_mmap_program = static_cast<std::uint8_t *>(mmap_res);
}

void ELF_parser::close_memory_map()
{
    if (m_fd == -1)
    {
        return;
    }

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

void ELF_parser::initialize_members(std::string file_path, int fd, 
                                    std::size_t program_length, std::uint8_t *mmap_program)
{
    m_file_path = std::move(file_path);
    m_fd = fd;
    m_program_length = program_length;
    m_mmap_program = mmap_program;
}

} // namespace elf_parser