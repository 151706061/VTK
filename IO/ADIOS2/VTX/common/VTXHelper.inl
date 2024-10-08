// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/*
 * VTXHelper.inl
 *
 *  Created on: May 3, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_VTX_COMMON_VTXHelper_inl
#define VTK_IO_ADIOS2_VTX_COMMON_VTXHelper_inl

#include <algorithm>
#include <iostream>
#include <sstream>

namespace vtx
{
namespace helper
{
VTK_ABI_NAMESPACE_BEGIN

template<class T>
std::vector<T> StringToVector(const std::string& input) noexcept
{
  std::vector<T> output;
  std::istringstream inputSS(input);

  T record;
  while (inputSS >> record)
  {
    output.push_back(record);
  }
  return output;
}

template<class T, class U>
std::vector<T> MapKeysToVector(const std::map<T, U>& input) noexcept
{
  std::vector<T> keys;
  keys.reserve(input.size());

  for (const auto& pair : input)
  {
    keys.push_back(pair.first);
  }
  return keys;
}

template<class T>
void Print(const std::vector<T>& input, const std::string& name)
{
  std::ostringstream oss;

  oss << name << " = { ";
  for (const T in : input)
  {
    oss << in << ", ";
  }
  oss << "}  rank : " << MPIGetRank();
  std::cout << oss.str() << "\n";
}

VTK_ABI_NAMESPACE_END
} // end namespace helper
} // end namespace vtx

#endif /* VTK_IO_ADIOS2_VTX_COMMON_VTXHelper_inl */
