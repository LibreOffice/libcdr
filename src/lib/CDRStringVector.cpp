/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <vector>
#include <libcdr/libcdr.h>

namespace libcdr
{
class CDRStringVectorImpl
{
public:
  CDRStringVectorImpl() : m_strings() {}
  ~CDRStringVectorImpl() {}
  std::vector<WPXString> m_strings;
};

} // namespace libcdr

libcdr::CDRStringVector::CDRStringVector()
  : m_pImpl(new CDRStringVectorImpl())
{
}

libcdr::CDRStringVector::CDRStringVector(const CDRStringVector &vec)
  : m_pImpl(new CDRStringVectorImpl(*(vec.m_pImpl)))
{
}

libcdr::CDRStringVector::~CDRStringVector()
{
  delete m_pImpl;
}

libcdr::CDRStringVector &libcdr::CDRStringVector::operator=(const CDRStringVector &vec)
{
  // Check for self-assignment
  if (this == &vec)
    return *this;
  if (m_pImpl)
    delete m_pImpl;
  m_pImpl = new CDRStringVectorImpl(*(vec.m_pImpl));
  return *this;
}

unsigned libcdr::CDRStringVector::size() const
{
  return (unsigned)(m_pImpl->m_strings.size());
}

bool libcdr::CDRStringVector::empty() const
{
  return m_pImpl->m_strings.empty();
}

const WPXString &libcdr::CDRStringVector::operator[](unsigned idx) const
{
  return m_pImpl->m_strings[idx];
}

void libcdr::CDRStringVector::append(const WPXString &str)
{
  m_pImpl->m_strings.push_back(str);
}

void libcdr::CDRStringVector::clear()
{
  m_pImpl->m_strings.clear();
}

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
