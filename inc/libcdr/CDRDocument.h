/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __CDRDOCUMENT_H__
#define __CDRDOCUMENT_H__

#include <librevenge/librevenge.h>
#include "libcdr_api.h"

namespace libcdr
{
class CDRDocument
{
public:

  static CDRAPI bool isSupported(librevenge::RVNGInputStream *input);

  static CDRAPI bool parse(librevenge::RVNGInputStream *input, librevenge::RVNGDrawingInterface *painter);
};

} // namespace libcdr

#endif //  __CDRDOCUMENT_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
