#include <string>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "ELF_reader.h"

#ifndef ERROR_EXIT
#define ERROR_EXIT(msg) do { \
    ::perror(msg);           \
    ::exit(EXIT_FAILURE);    \
} while (0)
#endif

namespace ELF
{

ELF_reader::ELF_reader()
    : fd_(-1), program_length_(0) , mmap_program_(nullptr) { }

ELF_reader::ELF_reader(const std::string& file_path)
    : file_path_(file_path)
{
    load_memory_map();
}

ELF_reader::ELF_reader(ELF_reader&& object) noexcept
    : file_path_(std::move(object.file_path_)), fd_(object.fd_),
    program_length_(object.program_length_), mmap_program_(object.mmap_program_)
{
    object.initialize_members();
}

ELF_reader& ELF_reader::operator=(ELF_reader&& object) noexcept
{
    initialize_members(std::move(object.file_path_), object.fd_,
                       object.program_length_, object.mmap_program_);

    object.initialize_members();
    return *this;
}

ELF_reader::~ELF_reader()
{
    close_memory_map();
}

void ELF_reader::load_file(const std::string& path_name)
{
    file_path_ = path_name;
    close_memory_map();
    load_memory_map();
}

void ELF_reader::show_file_header() const
{
    const Elf64_Ehdr *file_header;
    file_header = reinterpret_cast<Elf64_Ehdr *>(mmap_program_);

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
                   ((Elf64_Shdr *)(&mmap_program_[file_header->e_shoff]))->sh_info);

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
    auto shnum = reinterpret_cast<Elf64_Shdr *>(&mmap_program_[file_header->e_shoff])->sh_size;
    printf("%lu\n", shnum == 0 ? static_cast<decltype(shnum)>(file_header->e_shnum) : shnum);

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
        printf("%u\n", reinterpret_cast<Elf64_Shdr *>(&mmap_program_[file_header->e_shoff])->sh_link);
        break;
    default:
        printf("%u\n", file_header->e_shstrndx);
        break;
    }
}

void ELF_reader::show_section_headers() const
{
    const Elf64_Ehdr *file_header;
    const Elf64_Shdr *section_table;
    const char *section_string_table;
    size_t section_string_table_index;
    Elf64_Xword section_number;

    file_header = reinterpret_cast<Elf64_Ehdr *>(mmap_program_);
    section_table = reinterpret_cast<Elf64_Shdr *>(mmap_program_ + file_header->e_shoff);
    section_string_table_index = file_header->e_shstrndx == SHN_XINDEX ?
         reinterpret_cast<Elf64_Shdr *>(&mmap_program_[file_header->e_shoff])->sh_link :
         file_header->e_shstrndx;
    section_string_table = reinterpret_cast<char *>(&mmap_program_[section_table[section_string_table_index].sh_offset]);

    section_number = reinterpret_cast<Elf64_Shdr *>(&mmap_program_[file_header->e_shoff])->sh_size;
    if (section_number == 0)
    {
        section_number = file_header->e_shnum;
    }

    printf("There are %ld section header%s, starting at offset 0x%lx:\n\n", section_number, 
           section_number > 1 ? "s" : "", file_header->e_shoff);
    printf("Section Headers:\n"
           "  [Nr] Name              Type             Address           Offset\n"
           "       Size              EntSize          Flags  Link  Info  Align\n");
    for (decltype(section_number) i = 0; i < section_number; ++i)
    {
        printf("  [%2lu] ", i);

        /*
        * sh_name: This member specifies the name of the section.  Its value is an index into  the
        * section header string table section, giving the location of a null-terminated string.
        */
        printf("%-16.16s  ", section_string_table+section_table[i].sh_name);

        /*
        * sh_type: This member categorizes the section's contents and semantics.
        * 
        */
        switch (section_table[i].sh_type)
        {
        // Section header table entry unused
        case SHT_NULL:
            printf("NULL             ");
            break;

        // Program data
        case SHT_PROGBITS:
            printf("PROGBITS         ");
            break;

        // Symbol table
        case SHT_SYMTAB:
            printf("SYMTAB           ");
            break;

        // String table
        case SHT_STRTAB:
            printf("STRTAB           ");
            break;

        // Relocation entries with addends
        case SHT_RELA:
            printf("RELA             ");
            break;

        // Symbol hash table
        case SHT_HASH:
            printf("HASH             ");
            break;

        // Dynamic linking information
        case SHT_DYNAMIC:
            printf("DYNSYM           ");
            break;

        // Notes
        case SHT_NOTE:
            printf("NOTE             ");
            break;

        // Program space with no data (bss)
        case SHT_NOBITS:
            printf("NOBITS           ");
            break;

        // Relocation entries, no addends
        case SHT_REL:
            printf("REL              ");
            break;

        // Reserved
        case SHT_SHLIB:
            printf("SHLIB            ");
            break;

        // Dynamic linker symbol table
        case SHT_DYNSYM:
            printf("DYNSYM           ");
            break;

        // Array of constructors
        case SHT_INIT_ARRAY:
            printf("INIT_ARRAY       ");
            break;

        // Array of destructors
        case SHT_FINI_ARRAY:
            printf("FINIT_ARRAY       ");
            break;

        // Array of pre-constructors
        case SHT_PREINIT_ARRAY:
            printf("PREINIT_ARRAY    ");
            break;

        // Section group
        case SHT_GROUP:
            printf("GROUP            ");
            break;

        // Extended section indeces
        case SHT_SYMTAB_SHNDX:
            printf("SYMTAB_SHNDX     ");
            break;

        // Object attributes
        case SHT_GNU_ATTRIBUTES:
            printf("GNU_ATTRIBUTES   ");
            break;

        // GNU-style hash table
        case SHT_GNU_HASH:
            printf("GNU_HASH         ");
            break;

        // Prelink library list
        case SHT_GNU_LIBLIST:
            printf("GNU_LIBLIST      ");
            break;

        // Checksum for DSO content
        case SHT_CHECKSUM:
            printf("CHECKSUM         ");
            break;

        // Version definition section
        case SHT_GNU_verdef:
            printf("VERDEF           ");
            break;

        // Version needs section
        case SHT_GNU_verneed:
            printf("VERNEED          ");
            break;

        // Version symbol table
        case SHT_GNU_versym:
            printf("VERSYM           ");
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
        printf("%016lx  ", section_table[i].sh_addr);

        /*
        * sh_offset: This member's value holds the byte offset from the beginning of the file to the first
        *            byte in the section.  One section type, SHT_NOBITS, occupies no space  in  the  file,
        *            and its sh_offset member locates the conceptual placement in the file.
        */
        printf("%08lx\n", section_table[i].sh_offset);

        /*
        * sh_size: This  member  holds  the  section's  size  in  bytes.   Unless  the  section  type is
        *          SHT_NOBITS, the section occupies sh_size bytes  in  the  file.   A  section  of  type
        *          SHT_NOBITS may have a nonzero size, but it occupies no space in the file.
        */
        printf("       %016lx  ", section_table[i].sh_size);

        /*
        * sh_entsize:
                 Some  sections hold a table of fixed-sized entries, such as a symbol table.  For such
                 a section, this member gives the size in bytes for each entry.  This member  contains
                 zero if the section does not hold a table of fixed-size entries.

        */
        printf("%016lx ", section_table[i].sh_entsize);

        /*
        * sh_flags: Sections support one-bit flags that describe miscellaneous attributes.  If a flag bit
        *           is set in sh_flags, the attribute is "on" for the section.  Otherwise, the  attribute
        *           is "off" or does not apply.  Undefined attributes are set to zero.
        */
        std::string flags;
        if (section_table[i].sh_flags & SHF_WRITE)
            flags.push_back('W');
        if (section_table[i].sh_flags & SHF_ALLOC)
            flags.push_back('A');
        if (section_table[i].sh_flags & SHF_EXECINSTR)
            flags.push_back('X');
        if (section_table[i].sh_flags & SHF_MERGE)
            flags.push_back('M');
        if (section_table[i].sh_flags & SHF_STRINGS)
            flags.push_back('S');
        if (section_table[i].sh_flags & SHF_INFO_LINK)
            flags.push_back('I');
        if (section_table[i].sh_flags & SHF_LINK_ORDER)
            flags.push_back('L');
        if (section_table[i].sh_flags & SHF_OS_NONCONFORMING)
            flags.push_back('O');
        if (section_table[i].sh_flags & SHF_GROUP)
            flags.push_back('G');
        if (section_table[i].sh_flags & SHF_TLS)
            flags.push_back('T');
        if (section_table[i].sh_flags & SHF_COMPRESSED)
            flags.push_back('l');
        if (section_table[i].sh_flags & SHF_MASKOS)
            flags.push_back('o');
        if (section_table[i].sh_flags & SHF_MASKPROC)
            flags.push_back('p');
        if (section_table[i].sh_flags & SHF_ORDERED)
            flags.push_back('x');
        if (section_table[i].sh_flags & SHF_EXCLUDE)
            flags.push_back('E');

        std::sort(flags.begin(), flags.end());
        printf("%5s  ", flags.c_str());
        printf("%4d  ", section_table[i].sh_link);
        printf("%4d  ", section_table[i].sh_info);
        printf("%4lu\n", section_table[i].sh_addralign);
    }
    printf("Key to Flags:\n"
           "  W (write), A (alloc), X (execute), M (merge), S (strings), l (large)\n"
           "  I (info), L (link order), G (group), T (TLS), E (exclude), x (unknown)\n"
           "  O (extra OS processing required) o (OS specific), p (processor specific)\n");

}

void ELF_reader::show_symbols() const
{
    const Elf64_Ehdr *file_header;
    const Elf64_Shdr *section_table;
    const Elf64_Sym  *symbol_table;
    const char *symbol_string_table;
    const char *section_string_table;
    Elf64_Xword section_number;
    std::size_t symbol_entry_number;

    file_header = reinterpret_cast<Elf64_Ehdr *>(mmap_program_);
    section_table = reinterpret_cast<Elf64_Shdr *>(mmap_program_ + file_header->e_shoff);
    section_string_table = reinterpret_cast<char *>(mmap_program_ + section_table[file_header->e_shstrndx].sh_offset);
    section_number = reinterpret_cast<Elf64_Shdr *>(&mmap_program_[file_header->e_shoff])->sh_size;
    if (section_number == 0)
    {
        section_number = file_header->e_shnum;
    }

    for (decltype(section_number) i = 0; i < section_number; ++i)
    {
        if (section_table[i].sh_type == SHT_SYMTAB || section_table[i].sh_type == SHT_DYNSYM)
        {
            symbol_table = reinterpret_cast<Elf64_Sym *>(&mmap_program_[section_table[i].sh_offset]);
            symbol_entry_number = section_table[i].sh_size / section_table[i].sh_entsize;
            symbol_string_table = reinterpret_cast<char *>(&mmap_program_[section_table[i+1].sh_offset]);

            printf("\nSymbol table '%s' contain %lu %s:\n", &section_string_table[section_table[i].sh_name],
                   symbol_entry_number, symbol_entry_number == 0 ? "entry" : "entries");
            printf("   Num:    Value          Size Type    Bind   Vis      Ndx Name\n");
            for (decltype(symbol_entry_number) i = 0; i < symbol_entry_number; ++i)
            {
                printf("%6lu: %016lx %5lu ", i, symbol_table[i].st_value, symbol_table[i].st_size);
                switch (ELF64_ST_TYPE(symbol_table[i].st_info))
                {
                case STT_NOTYPE:
                    printf("NOTYPE  ");
                    break;
                case STT_OBJECT:
                    printf("OBJECT  ");
                    break;
                case STT_FUNC:
                    printf("FUNC    ");
                    break;
                case STT_SECTION:
                    printf("SECTION ");
                    break;
                case STT_FILE:
                    printf("FILE    ");
                    break;
                case STT_COMMON:
                    printf("COMMON  ");
                    break;
                case STT_TLS:
                    printf("TLS     ");
                    break;
                default:
                    printf("Unknown ");
                    break;
                }

                switch (ELF64_ST_BIND(symbol_table[i].st_info))
                {
                case STB_LOCAL:
                    printf("LOCAL  ");
                    break;
                case STB_GLOBAL:
                    printf("GLOBAL ");
                    break;
                case STB_WEAK:
                    printf("WEAK   ");
                    break;
                default:
                    printf("Unknown");
                }

                switch (ELF64_ST_VISIBILITY(symbol_table[i].st_other))
                {
                case STV_DEFAULT:
                    printf("DEFAULT  ");
                    break;
                case STV_INTERNAL:
                    printf("INTERNAL ");
                    break;
                case STV_HIDDEN:
                    printf("HIDDEN   ");
                    break;
                case STV_PROTECTED:
                    printf("PROTECTED");
                    break;
                default:
                    printf("Unknown  ");
                    break;
                }

                switch (symbol_table[i].st_shndx)
                {
                case SHN_ABS:
                    printf("ABS ");
                    break;
                case SHN_COMMON:
                    printf("COM ");
                    break;
                case SHN_UNDEF:
                    printf("UND ");
                    break;
                default:
                    printf("%3d ", symbol_table[i].st_shndx);
                    break;
                }

                printf("%.25s\n", &symbol_string_table[symbol_table[i].st_name]);
            }
        }
    }
}

void ELF_reader::load_memory_map()
{
    void *mmap_res;
    struct stat st;

    if ((fd_ = open(file_path_.c_str(), O_RDONLY)) == -1)
    {
        ERROR_EXIT("open");
    }

    if (fstat(fd_, &st) == -1)
    {
        ERROR_EXIT("fstat");
    }

    program_length_ = static_cast<std::size_t>(st.st_size);

    mmap_res = ::mmap(nullptr, program_length_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (mmap_res == MAP_FAILED)
    {
        ERROR_EXIT("mmap");
    }

    mmap_program_ = static_cast<std::uint8_t *>(mmap_res);
}

void ELF_reader::close_memory_map()
{
    if (fd_ == -1)
    {
        return;
    }

    if (::munmap(static_cast<void *>(mmap_program_), program_length_) == -1)
    {
        ERROR_EXIT("munmap");
    }

    if (::close(fd_) == -1)
    {
        ERROR_EXIT("close");
    }
}

void ELF_reader::initialize_members(std::string file_path, int fd,
                                    std::size_t program_length, std::uint8_t *mmap_program)
{
    file_path_ = std::move(file_path);
    fd_ = fd;
    program_length_ = program_length;
    mmap_program_ = mmap_program;
}

} // namespace elf_parser