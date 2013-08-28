// Copyright 2011 Juri Glass, Mathias Runge, Nadim El Sayed
// DAI-Labor, TU-Berlin
//
// This file is part of libSML.
//
// libSML is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// libSML is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with libSML.  If not, see <http://www.gnu.org/licenses/>.

#ifndef TEST_HELPER_H_
#define TEST_HELPER_H_

#include <sml/sml_shared.h>
#include <sml/sml_octet_string.h>

int hex2binary(char *hex, unsigned char *buf);
void expected_buf(sml_buffer *buf, char *hex, int len);
void expected_octet_string(octet_string *str, char *content, int len);

#endif
