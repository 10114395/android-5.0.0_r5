// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(simonb): Extend for 64-bit target libraries.

#include "elf_file.h"

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <vector>

#include "debug.h"
#include "libelf.h"
#include "packer.h"

namespace relocation_packer {

// Stub identifier written to 'null out' packed data, "NULL".
static const Elf32_Word kStubIdentifier = 0x4c4c554eu;

// Out-of-band dynamic tags used to indicate the offset and size of the
// .android.rel.dyn section.
static const Elf32_Sword DT_ANDROID_ARM_REL_OFFSET = DT_LOPROC;
static const Elf32_Sword DT_ANDROID_ARM_REL_SIZE = DT_LOPROC + 1;

// Alignment to preserve, in bytes.  This must be at least as large as the
// largest d_align and sh_addralign values found in the loaded file.
static const size_t kPreserveAlignment = 256;

namespace {

// Get section data.  Checks that the section has exactly one data entry,
// so that the section size and the data size are the same.  True in
// practice for all sections we resize when packing or unpacking.  Done
// by ensuring that a call to elf_getdata(section, data) returns NULL as
// the next data entry.
Elf_Data* GetSectionData(Elf_Scn* section) {
  Elf_Data* data = elf_getdata(section, NULL);
  CHECK(data && elf_getdata(section, data) == NULL);
  return data;
}

// Rewrite section data.  Allocates new data and makes it the data element's
// buffer.  Relies on program exit to free allocated data.
void RewriteSectionData(Elf_Data* data,
                        const void* section_data,
                        size_t size) {
  CHECK(size == data->d_size);
  uint8_t* area = new uint8_t[size];
  memcpy(area, section_data, size);
  data->d_buf = area;
}

// Verbose ELF header logging.
void VerboseLogElfHeader(const Elf32_Ehdr* elf_header) {
  VLOG("e_phoff = %u\n", elf_header->e_phoff);
  VLOG("e_shoff = %u\n", elf_header->e_shoff);
  VLOG("e_ehsize = %u\n", elf_header->e_ehsize);
  VLOG("e_phentsize = %u\n", elf_header->e_phentsize);
  VLOG("e_phnum = %u\n", elf_header->e_phnum);
  VLOG("e_shnum = %u\n", elf_header->e_shnum);
  VLOG("e_shstrndx = %u\n", elf_header->e_shstrndx);
}

// Verbose ELF program header logging.
void VerboseLogProgramHeader(size_t program_header_index,
                             const Elf32_Phdr* program_header) {
  std::string type;
  switch (program_header->p_type) {
    case PT_NULL: type = "NULL"; break;
    case PT_LOAD: type = "LOAD"; break;
    case PT_DYNAMIC: type = "DYNAMIC"; break;
    case PT_INTERP: type = "INTERP"; break;
    case PT_NOTE: type = "NOTE"; break;
    case PT_SHLIB: type = "SHLIB"; break;
    case PT_PHDR: type = "PHDR"; break;
    case PT_TLS: type = "TLS"; break;
    default: type = "(OTHER)"; break;
  }
  VLOG("phdr %lu : %s\n", program_header_index, type.c_str());
  VLOG("  p_offset = %u\n", program_header->p_offset);
  VLOG("  p_vaddr = %u\n", program_header->p_vaddr);
  VLOG("  p_paddr = %u\n", program_header->p_paddr);
  VLOG("  p_filesz = %u\n", program_header->p_filesz);
  VLOG("  p_memsz = %u\n", program_header->p_memsz);
}

// Verbose ELF section header logging.
void VerboseLogSectionHeader(const std::string& section_name,
                             const Elf32_Shdr* section_header) {
  VLOG("section %s\n", section_name.c_str());
  VLOG("  sh_addr = %u\n", section_header->sh_addr);
  VLOG("  sh_offset = %u\n", section_header->sh_offset);
  VLOG("  sh_size = %u\n", section_header->sh_size);
  VLOG("  sh_addralign = %u\n", section_header->sh_addralign);
}

// Verbose ELF section data logging.
void VerboseLogSectionData(const Elf_Data* data) {
  VLOG("  data\n");
  VLOG("    d_buf = %p\n", data->d_buf);
  VLOG("    d_off = %lu\n", data->d_off);
  VLOG("    d_size = %lu\n", data->d_size);
  VLOG("    d_align = %lu\n", data->d_align);
}

}  // namespace

// Load the complete ELF file into a memory image in libelf, and identify
// the .rel.dyn, .dynamic, and .android.rel.dyn sections.  No-op if the
// ELF file has already been loaded.
bool ElfFile::Load() {
  if (elf_)
    return true;

  elf_ = elf_begin(fd_, ELF_C_RDWR, NULL);
  CHECK(elf_);

  if (elf_kind(elf_) != ELF_K_ELF) {
    LOG("ERROR: File not in ELF format\n");
    return false;
  }

  Elf32_Ehdr* elf_header = elf32_getehdr(elf_);
  if (!elf_header) {
    LOG("ERROR: Failed to load ELF header\n");
    return false;
  }
  if (elf_header->e_machine != EM_ARM) {
    LOG("ERROR: File is not an arm32 ELF file\n");
    return false;
  }

  // Require that our endianness matches that of the target, and that both
  // are little-endian.  Safe for all current build/target combinations.
  const int endian = static_cast<int>(elf_header->e_ident[5]);
  CHECK(endian == ELFDATA2LSB);
  CHECK(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);

  VLOG("endian = %u\n", endian);
  VerboseLogElfHeader(elf_header);

  const Elf32_Phdr* elf_program_header = elf32_getphdr(elf_);
  CHECK(elf_program_header);

  const Elf32_Phdr* dynamic_program_header = NULL;
  for (size_t i = 0; i < elf_header->e_phnum; ++i) {
    const Elf32_Phdr* program_header = &elf_program_header[i];
    VerboseLogProgramHeader(i, program_header);

    if (program_header->p_type == PT_DYNAMIC) {
      CHECK(dynamic_program_header == NULL);
      dynamic_program_header = program_header;
    }
  }
  CHECK(dynamic_program_header != NULL);

  size_t string_index;
  elf_getshdrstrndx(elf_, &string_index);

  // Notes of the .rel.dyn, .android.rel.dyn, and .dynamic sections.  Found
  // while iterating sections, and later stored in class attributes.
  Elf_Scn* found_rel_dyn_section = NULL;
  Elf_Scn* found_android_rel_dyn_section = NULL;
  Elf_Scn* found_dynamic_section = NULL;

  // Flag set if we encounter any .debug* section.  We do not adjust any
  // offsets or addresses of any debug data, so if we find one of these then
  // the resulting output shared object should still run, but might not be
  // usable for debugging, disassembly, and so on.  Provides a warning if
  // this occurs.
  bool has_debug_section = false;

  Elf_Scn* section = NULL;
  while ((section = elf_nextscn(elf_, section)) != NULL) {
    const Elf32_Shdr* section_header = elf32_getshdr(section);
    std::string name = elf_strptr(elf_, string_index, section_header->sh_name);
    VerboseLogSectionHeader(name, section_header);

    // Note special sections as we encounter them.
    if (name == ".rel.dyn") {
      found_rel_dyn_section = section;
    }
    if (name == ".android.rel.dyn") {
      found_android_rel_dyn_section = section;
    }
    if (section_header->sh_offset == dynamic_program_header->p_offset) {
      found_dynamic_section = section;
    }

    // If we find a section named .debug*, set the debug warning flag.
    if (std::string(name).find(".debug") == 0) {
      has_debug_section = true;
    }

    // Ensure we preserve alignment, repeated later for the data block(s).
    CHECK(section_header->sh_addralign <= kPreserveAlignment);

    Elf_Data* data = NULL;
    while ((data = elf_getdata(section, data)) != NULL) {
      CHECK(data->d_align <= kPreserveAlignment);
      VerboseLogSectionData(data);
    }
  }

  // Loading failed if we did not find the required special sections.
  if (!found_rel_dyn_section) {
    LOG("ERROR: Missing .rel.dyn section\n");
    return false;
  }
  if (!found_dynamic_section) {
    LOG("ERROR: Missing .dynamic section\n");
    return false;
  }
  if (!found_android_rel_dyn_section) {
    LOG("ERROR: Missing .android.rel.dyn section "
        "(to fix, run with --help and follow the pre-packing instructions)\n");
    return false;
  }

  if (has_debug_section) {
    LOG("WARNING: found .debug section(s), and ignored them\n");
  }

  rel_dyn_section_ = found_rel_dyn_section;
  dynamic_section_ = found_dynamic_section;
  android_rel_dyn_section_ = found_android_rel_dyn_section;
  return true;
}

namespace {

// Helper for ResizeSection().  Adjust the main ELF header for the hole.
void AdjustElfHeaderForHole(Elf32_Ehdr* elf_header,
                            Elf32_Off hole_start,
                            int32_t hole_size) {
  if (elf_header->e_phoff > hole_start) {
    elf_header->e_phoff += hole_size;
    VLOG("e_phoff adjusted to %u\n", elf_header->e_phoff);
  }
  if (elf_header->e_shoff > hole_start) {
    elf_header->e_shoff += hole_size;
    VLOG("e_shoff adjusted to %u\n", elf_header->e_shoff);
  }
}

// Helper for ResizeSection().  Adjust all program headers for the hole.
void AdjustProgramHeadersForHole(Elf32_Phdr* elf_program_header,
                                 size_t program_header_count,
                                 Elf32_Off hole_start,
                                 int32_t hole_size) {
  for (size_t i = 0; i < program_header_count; ++i) {
    Elf32_Phdr* program_header = &elf_program_header[i];

    if (program_header->p_offset > hole_start) {
      // The hole start is past this segment, so adjust offsets and addrs.
      program_header->p_offset += hole_size;
      VLOG("phdr %lu p_offset adjusted to %u\n", i, program_header->p_offset);

      // Only adjust vaddr and paddr if this program header has them.
      if (program_header->p_vaddr != 0) {
        program_header->p_vaddr += hole_size;
        VLOG("phdr %lu p_vaddr adjusted to %u\n", i, program_header->p_vaddr);
      }
      if (program_header->p_paddr != 0) {
        program_header->p_paddr += hole_size;
        VLOG("phdr %lu p_paddr adjusted to %u\n", i, program_header->p_paddr);
      }
    } else if (program_header->p_offset +
               program_header->p_filesz > hole_start) {
      // The hole start is within this segment, so adjust file and in-memory
      // sizes, but leave offsets and addrs unchanged.
      program_header->p_filesz += hole_size;
      VLOG("phdr %lu p_filesz adjusted to %u\n", i, program_header->p_filesz);
      program_header->p_memsz += hole_size;
      VLOG("phdr %lu p_memsz adjusted to %u\n", i, program_header->p_memsz);
    }
  }
}

// Helper for ResizeSection().  Adjust all section headers for the hole.
void AdjustSectionHeadersForHole(Elf* elf,
                                 Elf32_Off hole_start,
                                 int32_t hole_size) {
  size_t string_index;
  elf_getshdrstrndx(elf, &string_index);

  Elf_Scn* section = NULL;
  while ((section = elf_nextscn(elf, section)) != NULL) {
    Elf32_Shdr* section_header = elf32_getshdr(section);
    std::string name = elf_strptr(elf, string_index, section_header->sh_name);

    if (section_header->sh_offset > hole_start) {
      section_header->sh_offset += hole_size;
      VLOG("section %s sh_offset"
           " adjusted to %u\n", name.c_str(), section_header->sh_offset);
      // Only adjust section addr if this section has one.
      if (section_header->sh_addr != 0) {
        section_header->sh_addr += hole_size;
        VLOG("section %s sh_addr"
             " adjusted to %u\n", name.c_str(), section_header->sh_addr);
      }
    }
  }
}

// Helper for ResizeSection().  Adjust the .dynamic section for the hole.
void AdjustDynamicSectionForHole(Elf_Scn* dynamic_section,
                                 bool is_rel_dyn_resize,
                                 Elf32_Off hole_start,
                                 int32_t hole_size) {
  Elf_Data* data = GetSectionData(dynamic_section);

  const Elf32_Dyn* dynamic_base = reinterpret_cast<Elf32_Dyn*>(data->d_buf);
  std::vector<Elf32_Dyn> dynamics(
      dynamic_base,
      dynamic_base + data->d_size / sizeof(dynamics[0]));

  for (size_t i = 0; i < dynamics.size(); ++i) {
    Elf32_Dyn* dynamic = &dynamics[i];
    const Elf32_Sword tag = dynamic->d_tag;
    // Any tags that hold offsets are adjustment candidates.
    const bool is_adjustable = (tag == DT_PLTGOT ||
                                tag == DT_HASH ||
                                tag == DT_STRTAB ||
                                tag == DT_SYMTAB ||
                                tag == DT_RELA ||
                                tag == DT_INIT ||
                                tag == DT_FINI ||
                                tag == DT_REL ||
                                tag == DT_JMPREL ||
                                tag == DT_INIT_ARRAY ||
                                tag == DT_FINI_ARRAY ||
                                tag == DT_ANDROID_ARM_REL_OFFSET);
    if (is_adjustable && dynamic->d_un.d_ptr > hole_start) {
      dynamic->d_un.d_ptr += hole_size;
      VLOG("dynamic[%lu] %u"
           " d_ptr adjusted to %u\n", i, dynamic->d_tag, dynamic->d_un.d_ptr);
    }

    // If we are specifically resizing .rel.dyn, we need to make some added
    // adjustments to tags that indicate the counts of R_ARM_RELATIVE
    // relocations in the shared object.
    if (is_rel_dyn_resize) {
      // DT_RELSZ is the overall size of relocations.  Adjust by hole size.
      if (tag == DT_RELSZ) {
        dynamic->d_un.d_val += hole_size;
        VLOG("dynamic[%lu] %u"
             " d_val adjusted to %u\n", i, dynamic->d_tag, dynamic->d_un.d_val);
      }

      // The crazy linker does not use DT_RELCOUNT, but we keep it updated
      // anyway.  In practice the section hole is always equal to the size
      // of R_ARM_RELATIVE relocations, and DT_RELCOUNT is the count of
      // relative relocations.  So closing a hole on packing reduces
      // DT_RELCOUNT to zero, and opening a hole on unpacking restores it to
      // its pre-packed value.
      if (tag == DT_RELCOUNT) {
        dynamic->d_un.d_val += hole_size / sizeof(Elf32_Rel);
        VLOG("dynamic[%lu] %u"
             " d_val adjusted to %u\n", i, dynamic->d_tag, dynamic->d_un.d_val);
      }

      // DT_RELENT doesn't change, but make sure it is what we expect.
      if (tag == DT_RELENT) {
        CHECK(dynamic->d_un.d_val == sizeof(Elf32_Rel));
      }
    }
  }

  void* section_data = &dynamics[0];
  size_t bytes = dynamics.size() * sizeof(dynamics[0]);
  RewriteSectionData(data, section_data, bytes);
}

// Helper for ResizeSection().  Adjust the .dynsym section for the hole.
// We need to adjust the values for the symbols represented in it.
void AdjustDynSymSectionForHole(Elf_Scn* dynsym_section,
                                Elf32_Off hole_start,
                                int32_t hole_size) {
  Elf_Data* data = GetSectionData(dynsym_section);

  const Elf32_Sym* dynsym_base = reinterpret_cast<Elf32_Sym*>(data->d_buf);
  std::vector<Elf32_Sym> dynsyms
      (dynsym_base,
       dynsym_base + data->d_size / sizeof(dynsyms[0]));

  for (size_t i = 0; i < dynsyms.size(); ++i) {
    Elf32_Sym* dynsym = &dynsyms[i];
    const int type = static_cast<int>(ELF32_ST_TYPE(dynsym->st_info));
    const bool is_adjustable = (type == STT_OBJECT ||
                                type == STT_FUNC ||
                                type == STT_SECTION ||
                                type == STT_FILE ||
                                type == STT_COMMON ||
                                type == STT_TLS);
    if (is_adjustable && dynsym->st_value > hole_start) {
      dynsym->st_value += hole_size;
      VLOG("dynsym[%lu] type=%u"
           " st_value adjusted to %u\n", i, type, dynsym->st_value);
    }
  }

  void* section_data = &dynsyms[0];
  size_t bytes = dynsyms.size() * sizeof(dynsyms[0]);
  RewriteSectionData(data, section_data, bytes);
}

// Helper for ResizeSection().  Adjust the .rel.plt section for the hole.
// We need to adjust the offset of every relocation inside it that falls
// beyond the hole start.
void AdjustRelPltSectionForHole(Elf_Scn* relplt_section,
                                Elf32_Off hole_start,
                                int32_t hole_size) {
  Elf_Data* data = GetSectionData(relplt_section);

  const Elf32_Rel* relplt_base = reinterpret_cast<Elf32_Rel*>(data->d_buf);
  std::vector<Elf32_Rel> relplts(
      relplt_base,
      relplt_base + data->d_size / sizeof(relplts[0]));

  for (size_t i = 0; i < relplts.size(); ++i) {
    Elf32_Rel* relplt = &relplts[i];
    if (relplt->r_offset > hole_start) {
      relplt->r_offset += hole_size;
      VLOG("relplt[%lu] r_offset adjusted to %u\n", i, relplt->r_offset);
    }
  }

  void* section_data = &relplts[0];
  size_t bytes = relplts.size() * sizeof(relplts[0]);
  RewriteSectionData(data, section_data, bytes);
}

// Helper for ResizeSection().  Adjust the .symtab section for the hole.
// We want to adjust the value of every symbol in it that falls beyond
// the hole start.
void AdjustSymTabSectionForHole(Elf_Scn* symtab_section,
                                Elf32_Off hole_start,
                                int32_t hole_size) {
  Elf_Data* data = GetSectionData(symtab_section);

  const Elf32_Sym* symtab_base = reinterpret_cast<Elf32_Sym*>(data->d_buf);
  std::vector<Elf32_Sym> symtab(
      symtab_base,
      symtab_base + data->d_size / sizeof(symtab[0]));

  for (size_t i = 0; i < symtab.size(); ++i) {
    Elf32_Sym* sym = &symtab[i];
    if (sym->st_value > hole_start) {
      sym->st_value += hole_size;
      VLOG("symtab[%lu] value adjusted to %u\n", i, sym->st_value);
    }
  }

  void* section_data = &symtab[0];
  size_t bytes = symtab.size() * sizeof(symtab[0]);
  RewriteSectionData(data, section_data, bytes);
}

// Resize a section.  If the new size is larger than the current size, open
// up a hole by increasing file offsets that come after the hole.  If smaller
// than the current size, remove the hole by decreasing those offsets.
void ResizeSection(Elf* elf, Elf_Scn* section, size_t new_size) {
  Elf32_Shdr* section_header = elf32_getshdr(section);
  if (section_header->sh_size == new_size)
    return;

  // Note if we are resizing the real .rel.dyn.  If yes, then we have to
  // massage d_un.d_val in the dynamic section where d_tag is DT_RELSZ and
  // DT_RELCOUNT.
  size_t string_index;
  elf_getshdrstrndx(elf, &string_index);
  const std::string section_name =
      elf_strptr(elf, string_index, section_header->sh_name);
  const bool is_rel_dyn_resize = section_name == ".rel.dyn";

  // Require that the section size and the data size are the same.  True
  // in practice for all sections we resize when packing or unpacking.
  Elf_Data* data = GetSectionData(section);
  CHECK(data->d_off == 0 && data->d_size == section_header->sh_size);

  // Require that the section is not zero-length (that is, has allocated
  // data that we can validly expand).
  CHECK(data->d_size && data->d_buf);

  const Elf32_Off hole_start = section_header->sh_offset;
  const int32_t hole_size = new_size - data->d_size;

  VLOG_IF(hole_size > 0, "expand section size = %lu\n", data->d_size);
  VLOG_IF(hole_size < 0, "shrink section size = %lu\n", data->d_size);

  // Resize the data and the section header.
  data->d_size += hole_size;
  section_header->sh_size += hole_size;

  Elf32_Ehdr* elf_header = elf32_getehdr(elf);
  Elf32_Phdr* elf_program_header = elf32_getphdr(elf);

  // Add the hole size to all offsets in the ELF file that are after the
  // start of the hole.  If the hole size is positive we are expanding the
  // section to create a new hole; if negative, we are closing up a hole.

  // Start with the main ELF header.
  AdjustElfHeaderForHole(elf_header, hole_start, hole_size);

  // Adjust all program headers.
  AdjustProgramHeadersForHole(elf_program_header,
                              elf_header->e_phnum,
                              hole_start,
                              hole_size);

  // Adjust all section headers.
  AdjustSectionHeadersForHole(elf, hole_start, hole_size);

  // We use the dynamic program header entry to locate the dynamic section.
  const Elf32_Phdr* dynamic_program_header = NULL;

  // Find the dynamic program header entry.
  for (size_t i = 0; i < elf_header->e_phnum; ++i) {
    Elf32_Phdr* program_header = &elf_program_header[i];

    if (program_header->p_type == PT_DYNAMIC) {
      dynamic_program_header = program_header;
    }
  }
  CHECK(dynamic_program_header);

  // Sections requiring special attention, and the .android.rel.dyn offset.
  Elf_Scn* dynamic_section = NULL;
  Elf_Scn* dynsym_section = NULL;
  Elf_Scn* relplt_section = NULL;
  Elf_Scn* symtab_section = NULL;
  Elf32_Off android_rel_dyn_offset = 0;

  // Find these sections, and the .android.rel.dyn offset.
  section = NULL;
  while ((section = elf_nextscn(elf, section)) != NULL) {
    Elf32_Shdr* section_header = elf32_getshdr(section);
    std::string name = elf_strptr(elf, string_index, section_header->sh_name);

    if (section_header->sh_offset == dynamic_program_header->p_offset) {
      dynamic_section = section;
    }
    if (name == ".dynsym") {
      dynsym_section = section;
    }
    if (name == ".rel.plt") {
      relplt_section = section;
    }
    if (name == ".symtab") {
      symtab_section = section;
    }

    // Note .android.rel.dyn offset.
    if (name == ".android.rel.dyn") {
      android_rel_dyn_offset = section_header->sh_offset;
    }
  }
  CHECK(dynamic_section != NULL);
  CHECK(dynsym_section != NULL);
  CHECK(relplt_section != NULL);
  CHECK(android_rel_dyn_offset != 0);

  // Adjust the .dynamic section for the hole.  Because we have to edit the
  // current contents of .dynamic we disallow resizing it.
  CHECK(section != dynamic_section);
  AdjustDynamicSectionForHole(dynamic_section,
                              is_rel_dyn_resize,
                              hole_start,
                              hole_size);

  // Adjust the .dynsym section for the hole.
  AdjustDynSymSectionForHole(dynsym_section, hole_start, hole_size);

  // Adjust the .rel.plt section for the hole.
  AdjustRelPltSectionForHole(relplt_section, hole_start, hole_size);

  // If present, adjust the .symtab section for the hole.  If the shared
  // library was stripped then .symtab will be absent.
  if (symtab_section)
    AdjustSymTabSectionForHole(symtab_section, hole_start, hole_size);
}

// Replace the first free (unused) slot in a dynamics vector with the given
// value.  The vector always ends with a free (unused) element, so the slot
// found cannot be the last one in the vector.
void AddDynamicEntry(Elf32_Dyn dyn,
                     std::vector<Elf32_Dyn>* dynamics) {
  // Loop until the penultimate entry.  We cannot replace the end sentinel.
  for (size_t i = 0; i < dynamics->size() - 1; ++i) {
    Elf32_Dyn &slot = dynamics->at(i);
    if (slot.d_tag == DT_NULL) {
      slot = dyn;
      VLOG("dynamic[%lu] overwritten with %u\n", i, dyn.d_tag);
      return;
    }
  }

  // No free dynamics vector slot was found.
  LOG("FATAL: No spare dynamic vector slots found "
      "(to fix, increase gold's --spare-dynamic-tags value)\n");
  NOTREACHED();
}

// Remove the element in the dynamics vector that matches the given tag with
// unused slot data.  Shuffle the following elements up, and ensure that the
// last is the null sentinel.
void RemoveDynamicEntry(Elf32_Sword tag,
                        std::vector<Elf32_Dyn>* dynamics) {
  // Loop until the penultimate entry, and never match the end sentinel.
  for (size_t i = 0; i < dynamics->size() - 1; ++i) {
    Elf32_Dyn &slot = dynamics->at(i);
    if (slot.d_tag == tag) {
      for ( ; i < dynamics->size() - 1; ++i) {
        dynamics->at(i) = dynamics->at(i + 1);
        VLOG("dynamic[%lu] overwritten with dynamic[%lu]\n", i, i + 1);
      }
      CHECK(dynamics->at(i).d_tag == DT_NULL);
      return;
    }
  }

  // No matching dynamics vector entry was found.
  NOTREACHED();
}

// Apply R_ARM_RELATIVE relocations to the file data to which they refer.
// This relocates data into the area it will occupy after the hole in
// .rel.dyn is added or removed.
void AdjustRelocationTargets(Elf* elf,
                             Elf32_Off hole_start,
                             size_t hole_size,
                             const std::vector<Elf32_Rel>& relocations) {
  Elf_Scn* section = NULL;
  while ((section = elf_nextscn(elf, section)) != NULL) {
    const Elf32_Shdr* section_header = elf32_getshdr(section);

    // Identify this section's start and end addresses.
    const Elf32_Addr section_start = section_header->sh_addr;
    const Elf32_Addr section_end = section_start + section_header->sh_size;

    Elf_Data* data = GetSectionData(section);

    // Ignore sections with no effective data.
    if (data->d_buf == NULL)
      continue;

    // Create a copy-on-write pointer to the section's data.
    uint8_t* area = reinterpret_cast<uint8_t*>(data->d_buf);

    for (size_t i = 0; i < relocations.size(); ++i) {
      const Elf32_Rel* relocation = &relocations[i];
      CHECK(ELF32_R_TYPE(relocation->r_info) == R_ARM_RELATIVE);

      // See if this relocation points into the current section.
      if (relocation->r_offset >= section_start &&
          relocation->r_offset < section_end) {
        Elf32_Addr byte_offset = relocation->r_offset - section_start;
        Elf32_Off* target = reinterpret_cast<Elf32_Off*>(area + byte_offset);

        // Is the relocation's target after the hole's start?
        if (*target > hole_start) {
          // Copy on first write.  Recompute target to point into the newly
          // allocated buffer.
          if (area == data->d_buf) {
            area = new uint8_t[data->d_size];
            memcpy(area, data->d_buf, data->d_size);
            target = reinterpret_cast<Elf32_Off*>(area + byte_offset);
          }

          *target += hole_size;
          VLOG("relocation[%lu] target adjusted to %u\n", i, *target);
        }
      }
    }

    // If we applied any relocation to this section, write it back.
    if (area != data->d_buf) {
      RewriteSectionData(data, area, data->d_size);
      delete [] area;
    }
  }
}

// Pad relocations with a given number of R_ARM_NONE relocations.
void PadRelocations(size_t count,
                    std::vector<Elf32_Rel>* relocations) {
  const Elf32_Rel r_arm_none = {R_ARM_NONE, 0};
  std::vector<Elf32_Rel> padding(count, r_arm_none);
  relocations->insert(relocations->end(), padding.begin(), padding.end());
}

// Adjust relocations so that the offset that they indicate will be correct
// after the hole in .rel.dyn is added or removed (in effect, relocate the
// relocations).
void AdjustRelocations(Elf32_Off hole_start,
                       size_t hole_size,
                       std::vector<Elf32_Rel>* relocations) {
  for (size_t i = 0; i < relocations->size(); ++i) {
    Elf32_Rel* relocation = &relocations->at(i);
    if (relocation->r_offset > hole_start) {
      relocation->r_offset += hole_size;
      VLOG("relocation[%lu] offset adjusted to %u\n", i, relocation->r_offset);
    }
  }
}

}  // namespace

// Remove R_ARM_RELATIVE entries from .rel.dyn and write as packed data
// into .android.rel.dyn.
bool ElfFile::PackRelocations() {
  // Load the ELF file into libelf.
  if (!Load()) {
    LOG("ERROR: Failed to load as ELF (elf_error=%d)\n", elf_errno());
    return false;
  }

  // Retrieve the current .rel.dyn section data.
  Elf_Data* data = GetSectionData(rel_dyn_section_);

  // Convert data to a vector of Elf32 relocations.
  const Elf32_Rel* relocations_base = reinterpret_cast<Elf32_Rel*>(data->d_buf);
  std::vector<Elf32_Rel> relocations(
      relocations_base,
      relocations_base + data->d_size / sizeof(relocations[0]));

  std::vector<Elf32_Rel> relative_relocations;
  std::vector<Elf32_Rel> other_relocations;

  // Filter relocations into those that are R_ARM_RELATIVE and others.
  for (size_t i = 0; i < relocations.size(); ++i) {
    const Elf32_Rel& relocation = relocations[i];
    if (ELF32_R_TYPE(relocation.r_info) == R_ARM_RELATIVE) {
      CHECK(ELF32_R_SYM(relocation.r_info) == 0);
      relative_relocations.push_back(relocation);
    } else {
      other_relocations.push_back(relocation);
    }
  }
  LOG("R_ARM_RELATIVE: %lu entries\n", relative_relocations.size());
  LOG("Other         : %lu entries\n", other_relocations.size());
  LOG("Total         : %lu entries\n", relocations.size());

  // If no relative relocations then we have nothing packable.  Perhaps
  // the shared object has already been packed?
  if (relative_relocations.empty()) {
    LOG("ERROR: No R_ARM_RELATIVE relocations found (already packed?)\n");
    return false;
  }

  // Unless padding, pre-apply R_ARM_RELATIVE relocations to account for the
  // hole, and pre-adjust all relocation offsets accordingly.
  if (!is_padding_rel_dyn_) {
    // Pre-calculate the size of the hole we will close up when we rewrite
    // .rel.dyn.  We have to adjust relocation addresses to account for this.
    Elf32_Shdr* section_header = elf32_getshdr(rel_dyn_section_);
    const Elf32_Off hole_start = section_header->sh_offset;
    size_t hole_size =
        relative_relocations.size() * sizeof(relative_relocations[0]);
    const size_t unaligned_hole_size = hole_size;

    // Adjust the actual hole size to preserve alignment.
    hole_size -= hole_size % kPreserveAlignment;
    LOG("Compaction    : %lu bytes\n", hole_size);

    // Adjusting for alignment may have removed any packing benefit.
    if (hole_size == 0) {
      LOG("Too few R_ARM_RELATIVE relocations to pack after alignment\n");
      return false;
    }

    // Add R_ARM_NONE relocations to other_relocations to preserve alignment.
    const size_t padding_bytes = unaligned_hole_size - hole_size;
    CHECK(padding_bytes % sizeof(other_relocations[0]) == 0);
    const size_t required = padding_bytes / sizeof(other_relocations[0]);
    PadRelocations(required, &other_relocations);
    LOG("Alignment pad : %lu relocations\n", required);

    // Apply relocations to all R_ARM_RELATIVE data to relocate it into the
    // area it will occupy once the hole in .rel.dyn is removed.
    AdjustRelocationTargets(elf_, hole_start, -hole_size, relative_relocations);
    // Relocate the relocations.
    AdjustRelocations(hole_start, -hole_size, &relative_relocations);
    AdjustRelocations(hole_start, -hole_size, &other_relocations);
  } else {
    // If padding, add R_ARM_NONE relocations to other_relocations to make it
    // the same size as the the original relocations we read in.  This makes
    // the ResizeSection() below a no-op.
    const size_t required = relocations.size() - other_relocations.size();
    PadRelocations(required, &other_relocations);
  }


  // Pack R_ARM_RELATIVE relocations.
  const size_t initial_bytes =
      relative_relocations.size() * sizeof(relative_relocations[0]);
  LOG("Unpacked R_ARM_RELATIVE: %lu bytes\n", initial_bytes);
  std::vector<uint8_t> packed;
  RelocationPacker packer;
  packer.PackRelativeRelocations(relative_relocations, &packed);
  const void* packed_data = &packed[0];
  const size_t packed_bytes = packed.size() * sizeof(packed[0]);
  LOG("Packed   R_ARM_RELATIVE: %lu bytes\n", packed_bytes);

  // If we have insufficient R_ARM_RELATIVE relocations to form a run then
  // packing fails.
  if (packed.empty()) {
    LOG("Too few R_ARM_RELATIVE relocations to pack\n");
    return false;
  }

  // Run a loopback self-test as a check that packing is lossless.
  std::vector<Elf32_Rel> unpacked;
  packer.UnpackRelativeRelocations(packed, &unpacked);
  CHECK(unpacked.size() == relative_relocations.size());
  for (size_t i = 0; i < unpacked.size(); ++i) {
    CHECK(unpacked[i].r_offset == relative_relocations[i].r_offset);
    CHECK(unpacked[i].r_info == relative_relocations[i].r_info);
  }

  // Make sure packing saved some space.
  if (packed_bytes >= initial_bytes) {
    LOG("Packing R_ARM_RELATIVE relocations saves no space\n");
    return false;
  }

  // Rewrite the current .rel.dyn section to be only the non-R_ARM_RELATIVE
  // relocations, then shrink it to size.
  const void* section_data = &other_relocations[0];
  const size_t bytes = other_relocations.size() * sizeof(other_relocations[0]);
  ResizeSection(elf_, rel_dyn_section_, bytes);
  RewriteSectionData(data, section_data, bytes);

  // Rewrite the current .android.rel.dyn section to hold the packed
  // R_ARM_RELATIVE relocations.
  data = GetSectionData(android_rel_dyn_section_);
  ResizeSection(elf_, android_rel_dyn_section_, packed_bytes);
  RewriteSectionData(data, packed_data, packed_bytes);

  // Rewrite .dynamic to include two new tags describing .android.rel.dyn.
  data = GetSectionData(dynamic_section_);
  const Elf32_Dyn* dynamic_base = reinterpret_cast<Elf32_Dyn*>(data->d_buf);
  std::vector<Elf32_Dyn> dynamics(
      dynamic_base,
      dynamic_base + data->d_size / sizeof(dynamics[0]));
  Elf32_Shdr* section_header = elf32_getshdr(android_rel_dyn_section_);
  // Use two of the spare slots to describe the .android.rel.dyn section.
  const Elf32_Dyn offset_dyn
      = {DT_ANDROID_ARM_REL_OFFSET, {section_header->sh_offset}};
  AddDynamicEntry(offset_dyn, &dynamics);
  const Elf32_Dyn size_dyn
      = {DT_ANDROID_ARM_REL_SIZE, {section_header->sh_size}};
  AddDynamicEntry(size_dyn, &dynamics);
  const void* dynamics_data = &dynamics[0];
  const size_t dynamics_bytes = dynamics.size() * sizeof(dynamics[0]);
  RewriteSectionData(data, dynamics_data, dynamics_bytes);

  Flush();
  return true;
}

// Find packed R_ARM_RELATIVE relocations in .android.rel.dyn, unpack them,
// and rewrite the .rel.dyn section in so_file to contain unpacked data.
bool ElfFile::UnpackRelocations() {
  // Load the ELF file into libelf.
  if (!Load()) {
    LOG("ERROR: Failed to load as ELF (elf_error=%d)\n", elf_errno());
    return false;
  }

  // Retrieve the current .android.rel.dyn section data.
  Elf_Data* data = GetSectionData(android_rel_dyn_section_);

  // Convert data to a vector of bytes.
  const uint8_t* packed_base = reinterpret_cast<uint8_t*>(data->d_buf);
  std::vector<uint8_t> packed(
      packed_base,
      packed_base + data->d_size / sizeof(packed[0]));

  // Properly packed data must begin with "APR1".
  if (packed.empty() ||
      packed[0] != 'A' || packed[1] != 'P' ||
      packed[2] != 'R' || packed[3] != '1') {
    LOG("ERROR: Packed R_ARM_RELATIVE relocations not found (not packed?)\n");
    return false;
  }

  // Unpack the data to re-materialize the R_ARM_RELATIVE relocations.
  const size_t packed_bytes = packed.size() * sizeof(packed[0]);
  LOG("Packed   R_ARM_RELATIVE: %lu bytes\n", packed_bytes);
  std::vector<Elf32_Rel> relative_relocations;
  RelocationPacker packer;
  packer.UnpackRelativeRelocations(packed, &relative_relocations);
  const size_t unpacked_bytes =
      relative_relocations.size() * sizeof(relative_relocations[0]);
  LOG("Unpacked R_ARM_RELATIVE: %lu bytes\n", unpacked_bytes);

  // Retrieve the current .rel.dyn section data.
  data = GetSectionData(rel_dyn_section_);

  // Interpret data as Elf32 relocations.
  const Elf32_Rel* relocations_base = reinterpret_cast<Elf32_Rel*>(data->d_buf);
  std::vector<Elf32_Rel> relocations(
      relocations_base,
      relocations_base + data->d_size / sizeof(relocations[0]));

  std::vector<Elf32_Rel> other_relocations;
  size_t padding = 0;

  // Filter relocations to locate any that are R_ARM_NONE.  These will occur
  // if padding was turned on for packing.
  for (size_t i = 0; i < relocations.size(); ++i) {
    const Elf32_Rel& relocation = relocations[i];
    if (ELF32_R_TYPE(relocation.r_info) != R_ARM_NONE) {
      other_relocations.push_back(relocation);
    } else {
      ++padding;
    }
  }
  LOG("R_ARM_RELATIVE: %lu entries\n", relative_relocations.size());
  LOG("Other         : %lu entries\n", other_relocations.size());

  // If we found the same number of R_ARM_NONE entries in .rel.dyn as we
  // hold as unpacked relative relocations, then this is a padded file.
  const bool is_padded = padding == relative_relocations.size();

  // Unless padded, pre-apply R_ARM_RELATIVE relocations to account for the
  // hole, and pre-adjust all relocation offsets accordingly.
  if (!is_padded) {
    // Pre-calculate the size of the hole we will open up when we rewrite
    // .rel.dyn.  We have to adjust relocation addresses to account for this.
    Elf32_Shdr* section_header = elf32_getshdr(rel_dyn_section_);
    const Elf32_Off hole_start = section_header->sh_offset;
    size_t hole_size =
        relative_relocations.size() * sizeof(relative_relocations[0]);

    // Adjust the hole size for the padding added to preserve alignment.
    hole_size -= padding * sizeof(other_relocations[0]);
    LOG("Expansion     : %lu bytes\n", hole_size);

    // Apply relocations to all R_ARM_RELATIVE data to relocate it into the
    // area it will occupy once the hole in .rel.dyn is opened.
    AdjustRelocationTargets(elf_, hole_start, hole_size, relative_relocations);
    // Relocate the relocations.
    AdjustRelocations(hole_start, hole_size, &relative_relocations);
    AdjustRelocations(hole_start, hole_size, &other_relocations);
  }

  // Rewrite the current .rel.dyn section to be the R_ARM_RELATIVE relocations
  // followed by other relocations.  This is the usual order in which we find
  // them after linking, so this action will normally put the entire .rel.dyn
  // section back to its pre-split-and-packed state.
  relocations.assign(relative_relocations.begin(), relative_relocations.end());
  relocations.insert(relocations.end(),
                     other_relocations.begin(), other_relocations.end());
  const void* section_data = &relocations[0];
  const size_t bytes = relocations.size() * sizeof(relocations[0]);
  LOG("Total         : %lu entries\n", relocations.size());
  ResizeSection(elf_, rel_dyn_section_, bytes);
  RewriteSectionData(data, section_data, bytes);

  // Nearly empty the current .android.rel.dyn section.  Leaves a four-byte
  // stub so that some data remains allocated to the section.  This is a
  // convenience which allows us to re-pack this file again without
  // having to remove the section and then add a new small one with objcopy.
  // The way we resize sections relies on there being some data in a section.
  data = GetSectionData(android_rel_dyn_section_);
  ResizeSection(elf_, android_rel_dyn_section_, sizeof(kStubIdentifier));
  RewriteSectionData(data, &kStubIdentifier, sizeof(kStubIdentifier));

  // Rewrite .dynamic to remove two tags describing .android.rel.dyn.
  data = GetSectionData(dynamic_section_);
  const Elf32_Dyn* dynamic_base = reinterpret_cast<Elf32_Dyn*>(data->d_buf);
  std::vector<Elf32_Dyn> dynamics(
      dynamic_base,
      dynamic_base + data->d_size / sizeof(dynamics[0]));
  RemoveDynamicEntry(DT_ANDROID_ARM_REL_SIZE, &dynamics);
  RemoveDynamicEntry(DT_ANDROID_ARM_REL_OFFSET, &dynamics);
  const void* dynamics_data = &dynamics[0];
  const size_t dynamics_bytes = dynamics.size() * sizeof(dynamics[0]);
  RewriteSectionData(data, dynamics_data, dynamics_bytes);

  Flush();
  return true;
}

// Flush rewritten shared object file data.
void ElfFile::Flush() {
  // Flag all ELF data held in memory as needing to be written back to the
  // file, and tell libelf that we have controlled the file layout.
  elf_flagelf(elf_, ELF_C_SET, ELF_F_DIRTY);
  elf_flagelf(elf_, ELF_C_SET, ELF_F_LAYOUT);

  // Write ELF data back to disk.
  const off_t file_bytes = elf_update(elf_, ELF_C_WRITE);
  CHECK(file_bytes > 0);
  VLOG("elf_update returned: %lu\n", file_bytes);

  // Clean up libelf, and truncate the output file to the number of bytes
  // written by elf_update().
  elf_end(elf_);
  elf_ = NULL;
  const int truncate = ftruncate(fd_, file_bytes);
  CHECK(truncate == 0);
}

}  // namespace relocation_packer
