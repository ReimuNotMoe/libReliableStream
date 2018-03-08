/*
    This file is part of ReliableStream.
    Copyright (C) 2018  ReimuNotMoe <reimuhatesfdt@gmail.com>

    ReliableStream is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ReliableStream is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ReliableStream.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ReliableStream.h"


void rrs_builtin_callback_checksum_crc16(size_t len, void *buf, void *buf_cksum, void *userp) {
	const uint16_t CRC16 = 0x8005;

	uint16_t out = 0;
	int bits_read = 0, bit_flag;

	/* Sanity check: */
	if (buf == NULL)
		return;

	while (len > 0) {
		bit_flag = out >> 15;

		/* Get next bit: */
		out <<= 1;
		out |= (*(uint8_t *)buf >> bits_read) & 1; // item a) work from the least significant bits

		/* Increment bit counter: */
		bits_read++;
		if (bits_read > 7) {
			bits_read = 0;
			buf++;
			len--;
		}

		/* Cycle check: */
		if (bit_flag)
			out ^= CRC16;

	}

	// item b) "push out" the last 16 bits
	int i;
	for (i = 0; i < 16; ++i) {
		bit_flag = out >> 15;
		out <<= 1;
		if (bit_flag)
			out ^= CRC16;
	}

	// item c) reverse the bits
	uint16_t crc = 0;
	i = 0x8000;
	int j = 0x0001;
	for (; i != 0; i >>=1, j <<= 1) {
		if (i & out) crc |= j;
	}

	fprintf(stderr, "crc16: %u\n", crc);

	memcpy(buf_cksum, &crc, 2);
}