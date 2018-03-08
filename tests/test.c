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

#include "../ReliableStream.h"

int main(int argc, char **argv) {
	struct rrs_params params;
	params.Timeout_IO_Read = 150;
	params.Timeout_IO_Resend = 300;
	params.Timeout_IO_Blackhole = 100;
	params.LengthMax_Data = 64;
	params.Length_Checksum = 2;


	struct rrs_ctx ctx;
	rrs_init_ctx(&ctx, &params);


	ctx.Callback_Read = rrs_builtin_callback_read;
	ctx.Callback_Write = rrs_builtin_callback_write;
	ctx.Callback_Checksum = rrs_builtin_callback_checksum_crc16;

	struct rrs_builtin_callback_ctx bcctx;
	bcctx.FD_Base = open(argv[2], O_RDWR);
	ctx.CallbackContext_Read = ctx.CallbackContext_Write = &bcctx;

	char buf[128];

	if (argv[1][0] == 's') {
		while (1) {

			ssize_t rc_read = read(STDIN_FILENO, buf, 64);
			fprintf(stderr, "READ STDIN: %zd\n", rc_read);
			ssize_t rc_rw;
			do {
				rc_rw = rrs_write(&ctx, rc_read, buf);
				fprintf(stderr, "rrs_write: %zd\n", rc_rw);
			} while ((rc_rw % 100) == RRS_STATUS_IO_BADMAC || (rc_rw % 100) == RRS_STATUS_IO_TIMEOUT);

		}
	} else {
		while (1) {
			ssize_t rc_rr = rrs_read(&ctx, 64, buf);
			fprintf(stderr, "rrs_read: %zd\n", rc_rr);
			if (rc_rr)
				write(STDOUT_FILENO, buf, rc_rr);
		}
	}


}