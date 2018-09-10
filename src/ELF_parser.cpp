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

    /*
    * Within this  array  everything  is  named  by  macros,  which start with the prefix 
    * EI_ and may contain values which start with the prefix ELF. 
    * 
    * EI_MAG0: The first byte of the magic number.  It must  be  filled  with  ELFMAG0.
    *          (0: 0x7f)
    * EI_MAG1: The  second  byte  of the magic number.  It must be filled with ELFMAG1.
    *          (1: 'E')
    * EI_MAG2: The third byte of the magic number.  It must  be  filled  with  ELFMAG2.
    *          (2: 'L')
    * EI_MAG3: The  fourth  byte  of the magic number.  It must be filled with ELFMAG3.
    *          (3: 'F')
    */
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

    /*
    * Magic number and other info
    */
    printf("  Magic:  ");
    for (int i = 0; i < EI_NIDENT; ++i)
    {
        printf(" %02x", file_header->e_ident[i]);
    }
    printf("\n");

    /*
    * EI_CLASS: The fifth byte identifies the architecture for this binary:
    * 
    *   ELFCLASSNONE: This class is invalid.
    *   ELFCLASS32  : This  defines  the  32-bit  architecture.    It   supports
    *                 machines  with  files  and  virtual address spaces up to 4
    *                 Gigabytes.
    *   ELFCLASS64  : This defines the 64-bit architecture.
    */
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

    /*
    * EI_DATA: The sixth byte specifies the data  encoding  of  the  processor-specific
    *         data in the file.  Currently these encodings are supported:
    *
    *     ELFDATANONE: Unknown data format.
    *     ELFDATA2LSB: Two's complement, little-endian.
    *     ELFDATA2MSB: Two's complement, big-endian.
    */
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

    /*
    * EI_ABIVERSION:
    *      The ninth byte identifies the version of the ABI to which the object  is
    *      targeted.  This field is used to distinguish among incompatible versions
    *      of an ABI.  The interpretation of this version number  is  dependent  on
    *      the  ABI  identified  by the EI_OSABI field.  Applications conforming to
    *      this specification use the value 0.
    */
    printf("  Version:                           ");
    printf("%d\n", (int)file_header->e_ident[EI_ABIVERSION]);

    /*
    * EI_OSABI: The  eighth  byte  identifies  the operating system and ABI to which the
    *           object is targeted.  Some fields in other ELF structures have flags  and
    *           values that have platform-specific meanings; the interpretation of those
    *           fields is determined by the value of this byte.  For example:
    * 
    *     ELFOSABI_NONE       Same as ELFOSABI_SYSV
    *     ELFOSABI_SYSV       UNIX System V ABI.
    *     ELFOSABI_HPUX       HP-UX ABI.
    *     ELFOSABI_NETBSD     NetBSD ABI.
    *     ELFOSABI_LINUX      Linux ABI.
    *     ELFOSABI_SOLARIS    Solaris ABI.
    *     ELFOSABI_IRIX       IRIX ABI.
    *     ELFOSABI_FREEBSD    FreeBSD ABI.
    *     ELFOSABI_TRU64      TRU64 UNIX ABI.
    *     ELFOSABI_ARM        ARM architecture ABI.
    *     ELFOSABI_STANDALONE Stand-alone (embedded) ABI.
    */
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
        printf("Unknown ABI\n");
        break;
    }

    /*
    * e_type: This member of the structure identifies the object file type:
    *
    *     ET_NONE: An unknown type.
    *     ET_REL : A relocatable file.
    *     ET_EXEC: An executable file.
    *     ET_DYN : A shared object.
    *     ET_CORE: A core file.
    */
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

    /*
    * e_machine: This member specifies the required architecture for an individual file.   
    *            For  example:
    *     EM_NONE     An unknown machine.
    *     EM_M32      AT&T WE 32100.
    *     EM_SPARC    Sun Microsystems SPARC.
    *     EM_386      Intel 80386.
    *     EM_68K      Motorola 68000.
    *     EM_88K      Motorola 88000.
    *     EM_860      Intel 80860.
    *     EM_MIPS     MIPS RS3000 (big-endian only).
    *     EM_PARISC   HP/PA.
    *     EM_SPARC32PLUS
    *                 SPARC with enhanced instruction set.
    *     EM_PPC      PowerPC.
    *     EM_PPC64    PowerPC 64-bit.
    *     EM_S390     IBM S/390
    *     EM_ARM      Advanced RISC Machines
    *     EM_SH       Renesas SuperH
    *     EM_SPARCV9  SPARC v9 64-bit.
    *     EM_IA_64    Intel Itanium
    *     EM_X86_64   AMD x86-64
    *     EM_VAX      DEC Vax.
    */
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

    /*
    * e_version: This member identifies the file version:
    * 
    *     EV_NONE   : Invalid version.
    *     EV_CURRENT: Current version.
    */
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

    /*
    * e_entry: This  member  gives the virtual address to which the system first transfers control,
    *          thus starting the process.  If the file has no associated entry point,  this  member
    *          holds zero.
    */
    printf("  Entry point address:               ");
    printf("0x%lx\n", file_header->e_entry);

    /*
    * e_phoff: This  member holds the program header table's file offset in bytes.  If the file has
    *          no program header table, this member holds zero.
    */
    printf("  Start of program headers:          ");
    printf("%ld (bytes into file)\n", file_header->e_phoff);

    /*
    * e_shoff: This member holds the section header table's file offset in bytes.  If the file  has
    *          no section header table, this member holds zero.
    */
    printf("  Start of section headers:          ");
    printf("%ld (bytes into file)\n", file_header->e_shoff);

    /*
    * e_flags: This  member  holds  processor-specific  flags associated with the file.  Flag names
    *          take the form EF_`machine_flag'.  Currently no flags have been defined.
    */
    printf("  Flags:                             ");
    printf("0x%x\n", file_header->e_flags);

    /*
    * e_ehsize: This member holds the ELF header's size in bytes.
    */
    printf("  Size of this header:               ");
    printf("%d (bytes)\n", file_header->e_ehsize);

    /*
    * e_phentsize: This member holds the size in bytes of one entry in the file's program header table;
    *              all entries are the same size.

    */
    printf("  Size of program headers:           ");
    printf("%d (bytes)\n", file_header->e_phentsize);

    /*
    * e_phnum: This member holds the number of entries in the program header table.  Thus the product
    *          of e_phentsize and e_phnum gives the table's size in bytes.  If a  file  has  no
    *          program header, e_phnum holds the value zero.
    *
    *          If  the  number  of  entries  in the program header table is larger than or equal to
    *          PN_XNUM (0xffff), this member holds PN_XNUM (0xffff) and the real number of  entries
    *          in  the  program  header table is held in the sh_info member of the initial entry in
    *          section header table.  Otherwise, the sh_info member of the initial  entry  contains
    *          the value zero.
    */
    printf("  Number of program headers:         ");
    printf("%d\n", file_header->e_phnum < PN_XNUM ? file_header->e_phnum : 
                   ((Elf64_Shdr *)(&m_mmap_program[file_header->e_shoff]))->sh_info);

    /*
    * e_shentsize: This member holds a sections header's size in bytes.  A section header is one  entry
    *              in the section header table; all entries are the same size.
    */
    printf("  Size of section headers:           ");
    printf("%d (bytes)\n", file_header->e_shentsize);

    /*
    * e_shnum: This member holds the number of entries in the section header table.  Thus the prod‐
    *          uct of e_shentsize and e_shnum gives the section header table's size in bytes.  If a
    *          file has no section header table, e_shnum holds the value of zero.
    *
    *          If  the  number  of  entries  in the section header table is larger than or equal to
    *          SHN_LORESERVE (0xff00), e_shnum holds the value zero and the real number of  entries
    *          in  the  section  header table is held in the sh_size member of the initial entry in
    *          section header table.  Otherwise, the sh_size member of the  initial  entry  in  the
    *          section header table holds the value zero.
    */
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

    /*
    * e_shstrndx: This  member  holds  the section header table index of the entry associated with the
    *             section name string table.  If the file has no section name string table, this  mem‐
    *             ber holds the value SHN_UNDEF.
    *
    *             If  the  index  of  section  name  string  table  section is larger than or equal to
    *             SHN_LORESERVE (0xff00), this member holds SHN_XINDEX (0xffff) and the real index  of
    *             the  section  name string table section is held in the sh_link member of the initial
    *             entry in section header table.  Otherwise, the sh_link member of the  initial  entry
    *             in section header table contains the value zero.
    */
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

        /*
        * sh_name: This member specifies the name of the section.  Its value is an index into  the
        * section header string table section, giving the location of a null-terminated string.
        */
        printf("%-16s  ", section_name_table+shdr[i].sh_name);

        /*
        * sh_type: This member categorizes the section's contents and semantics.
        * 
        */
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

        /*
        * sh_addr: If  this  section  appears  in  the  memory image of a process, this member holds the
        *          address at which the section's first byte should reside.  Otherwise, the member  con‐
        *          tains zero.
        */
        printf("%016lx  ", shdr[i].sh_addr);

        /*
        * sh_offset: This member's value holds the byte offset from the beginning of the file to the first
        *            byte in the section.  One section type, SHT_NOBITS, occupies no space  in  the  file,
        *            and its sh_offset member locates the conceptual placement in the file.
        */
        printf("%08lx\n", shdr[i].sh_offset);

        /*
        * sh_size: This  member  holds  the  section's  size  in  bytes.   Unless  the  section  type is
        *          SHT_NOBITS, the section occupies sh_size bytes  in  the  file.   A  section  of  type
        *          SHT_NOBITS may have a nonzero size, but it occupies no space in the file.
        */
        printf("       %016lx  ", shdr[i].sh_size);

        /*
        * sh_entsize:
                 Some  sections hold a table of fixed-sized entries, such as a symbol table.  For such
                 a section, this member gives the size in bytes for each entry.  This member  contains
                 zero if the section does not hold a table of fixed-size entries.

        */
        printf("%016lx ", shdr[i].sh_entsize);

        /*
        * sh_flags: Sections support one-bit flags that describe miscellaneous attributes.  If a flag bit
        *           is set in sh_flags, the attribute is "on" for the section.  Otherwise, the  attribute
        *           is "off" or does not apply.  Undefined attributes are set to zero.
        */
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