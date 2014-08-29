/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __LIBCDR_API_H__
#define __LIBCDR_API_H__

#ifdef DLL_EXPORT
#ifdef LIBCDR_BUILD
#define CDRAPI __declspec(dllexport)
#else
#define CDRAPI __declspec(dllimport)
#endif
#else // !DLL_EXPORT
#ifdef LIBCDR_VISIBILITY
#define CDRAPI __attribute__((visibility("default")))
#else
#define CDRAPI
#endif
#endif

#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
