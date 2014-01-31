/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __CDRSTRINGVECTOR_H__
#define __CDRSTRINGVECTOR_H__

#include <libwpd/libwpd.h>

namespace libcdr
{
class CDRStringVectorImpl;

class CDRStringVector
{
public:
  CDRStringVector();
  CDRStringVector(const CDRStringVector &vec);
  ~CDRStringVector();

  CDRStringVector &operator=(const CDRStringVector &vec);

  unsigned size() const;
  bool empty() const;
  const WPXString &operator[](unsigned idx) const;
  void append(const WPXString &str);
  void clear();

private:
  CDRStringVectorImpl *m_pImpl;
};

} // namespace libcdr

#endif /* __CDRSTRINGVECTOR_H__ */
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
