/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* libcdr
 * Version: MPL 1.1 / GPLv2+ / LGPLv2+
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License or as specified alternatively below. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * Major Contributor(s):
 * Copyright (C) 2012 Fridrich Strba <fridrich.strba@bluewin.ch>
 *
 *
 * All Rights Reserved.
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPLv2+"), or
 * the GNU Lesser General Public License Version 2 or later (the "LGPLv2+"),
 * in which case the provisions of the GPLv2+ or the LGPLv2+ are applicable
 * instead of those above.
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
