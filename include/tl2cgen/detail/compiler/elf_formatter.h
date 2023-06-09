/*!
 * Copyright (c) 2023 by Contributors
 * \file elf_formatter.h
 * \author Hyunsu Cho
 * \brief Generate a relocatable object file containing a constant, read-only array
 */
#ifndef TL2CGEN_DETAIL_COMPILER_ELF_FORMATTER_H_
#define TL2CGEN_DETAIL_COMPILER_ELF_FORMATTER_H_

#include <vector>

namespace tl2cgen::compiler::detail::elf {

/*!
 * \brief Pre-allocate space in a buffer to fit an ELF header
 * \param elf Buffer in which space will be allocated
 */
void AllocateELFHeader(std::vector<char>* elf);
/*!
 * \brief Format a relocatable ELF object file containing a constant, read-only array
 * \param elf When the function is invoked, elf should contain 1) empty bytes that have been
 *            allocated with a call to AllocateELFHeader(); followed by 2) bytes representing the
 *            content of the array. The FormatArrayAsELF() function will then modify elf, so that
 *            in the end, elf will contain a valid ELF relocatable object. Two modification will
 *            take place: 1) The preceding empty bytes in elf will be overwritten with a valid ELF
 *            header; and 2) symbol table, section headers, and other various sections will be
 *            appended after the array bytes.
 */
void FormatArrayAsELF(std::vector<char>* elf);

}  // namespace tl2cgen::compiler::detail::elf

#endif  // TL2CGEN_DETAIL_COMPILER_ELF_FORMATTER_H_
