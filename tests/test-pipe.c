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

#include <pthread.h>

int main(int argc, char **argv) {
	struct rrs_params params;
	params.Timeout_IO_Read = 150;
	params.Timeout_IO_Resend = 300;
	params.Timeout_IO_Blackhole = 100;
	params.Timeout_IO_PipeThread = 30;

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

	int pipefd = rrs_pipe_create(&ctx);
	pthread_t tid_pipe;
	pthread_create(&tid_pipe, NULL, rrs_pipe_io_thread, &ctx);


	char buf[128];

	if (argv[1][0] == 's') {
		while (1) {

			ssize_t rc_read = read(STDIN_FILENO, buf, 64);
			fprintf(stderr, "READ STDIN: %zd\n", rc_read);
			ssize_t rc_rw;

				rc_rw = write(pipefd, buf, rc_read);
				fprintf(stderr, "! rrs pipe write: %zd\n", rc_rw);


		}
	} else {
		while (1) {
			ssize_t rc_rr = read(pipefd, buf, 64);
			fprintf(stderr, "! rrs pipe read: %zd\n", rc_rr);
			if (rc_rr)
				write(STDOUT_FILENO, buf, rc_rr);
		}
	}


}