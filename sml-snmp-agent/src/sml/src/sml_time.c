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

#include <sml/sml_number.h>
#include <sml/sml_shared.h>
#include <sml/sml_time.h>
#include <stdio.h>

sml_time *sml_time_init() {
	sml_time *t = (sml_time *)malloc(sizeof(sml_time));
	*t = (sml_time){.tag = NULL, .data.sec_index = NULL};
	return t;
}

sml_time *sml_time_parse(sml_buffer *buf) {
	if (sml_buf_optional_is_skipped(buf)) {
		return NULL;
	}

	sml_time *tme = sml_time_init();

	// workaround Holley DTZ541
	// if SML_ListEntry valTime (SML_Time) is given there are missing bytes:
	// 0x72: indicate a list for SML_Time with 2 entries
	// 0x62 0x01: indicate secIndex
	// instead, the DTZ541 starts with 0x65 + 4 bytes secIndex
	// the workaround will add this information during parsing
	if (sml_buf_get_current_byte(buf) == (SML_TYPE_UNSIGNED | 5)) {
		tme->tag = malloc(sizeof(u8));
		*(tme->tag) = SML_TIME_SEC_INDEX;
	} else {
		if (sml_buf_get_next_type(buf) != SML_TYPE_LIST) {
			buf->error = 1;
			goto error;
		}

		if (sml_buf_get_next_length(buf) != 2) {
			buf->error = 1;
			goto error;
		}

		tme->tag = sml_u8_parse(buf);
		if (sml_buf_has_errors(buf))
			goto error;
	}

	int type = sml_buf_get_next_type(buf);
	switch (type) {
	case SML_TYPE_UNSIGNED:
		tme->data.timestamp = sml_u32_parse(buf);
		if (sml_buf_has_errors(buf))
			goto error;
		break;
	case SML_TYPE_LIST:
		// Some meters (e.g. FROETEC Multiflex ZG22) giving not one uint32
		// as timestamp, but a list of 3 values.
		// Ignoring these values, so that parsing does not fail.
		sml_buf_get_next_length(buf); // should we check the length here?
		u32 *t1 = sml_u32_parse(buf);
		if (sml_buf_has_errors(buf)) {
			if (t1)
				sml_number_free(t1);
			goto error;
		}
		i16 *t2 = sml_i16_parse(buf);
		if (sml_buf_has_errors(buf)) {
			if (t1)
				sml_number_free(t1);
			if (t2)
				sml_number_free(t2);
			goto error;
		}
		i16 *t3 = sml_i16_parse(buf);
		if (sml_buf_has_errors(buf)) {
			if (t1)
				sml_number_free(t1);
			if (t2)
				sml_number_free(t2);
			if (t3)
				sml_number_free(t3);
			goto error;
		}
		fprintf(
			stderr,
			"libsml: error: sml_time as list[3]: ignoring value[0]=%u value[1]=%d value[2]=%d\n",
			t1 ? *t1 : 0, t2 ? *t2 : 0, t3 ? *t3 : 0);
		if (t1)
			sml_number_free(t1);
		if (t2)
			sml_number_free(t2);
		if (t3)
			sml_number_free(t3);
		break;
	default:
		goto error;
	}

	return tme;

error:
	sml_time_free(tme);
	return NULL;
}

void sml_time_write(sml_time *t, sml_buffer *buf) {
	if (t == 0) {
		sml_buf_optional_write(buf);
		return;
	}

	sml_buf_set_type_and_length(buf, SML_TYPE_LIST, 2);
	sml_u8_write(t->tag, buf);
	sml_u32_write(t->data.timestamp, buf);
}

void sml_time_free(sml_time *tme) {
	if (tme) {
		sml_number_free(tme->tag);
		sml_number_free(tme->data.timestamp);
		free(tme);
	}
}
