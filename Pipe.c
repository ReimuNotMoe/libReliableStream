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

int rrs_pipe_create(struct rrs_ctx *ctx) {
	socketpair(AF_UNIX, SOCK_STREAM, 0, ctx->FD_Pipe);
	return ctx->FD_Pipe[1];
}

ssize_t rrs_util_readall(int fd, void *buf, size_t count) {
	size_t readbytes = 0;
	ssize_t rc_read;

	while (readbytes < count) {
		rc_read = read(fd, buf+readbytes, count-readbytes);

		if (rc_read > 0) {
			readbytes += rc_read;
		} else if (rc_read == 0) {
			return readbytes;
		} else if (rc_read < 0) {
			return rc_read;
		}
	}

	return readbytes;
}

ssize_t rrs_util_writeall(int fd, void *buf, size_t count) {
	size_t writtenbytes = 0;
	ssize_t rc_write;

	while (writtenbytes < count) {
		rc_write = write(fd, buf+writtenbytes, count-writtenbytes);

		if (rc_write > 0) {
			writtenbytes += rc_write;
		} else if (rc_write == 0) {
			return writtenbytes;
		} else if (rc_write < 0) {
			return rc_write;
		}
	}

	return writtenbytes;
}

void rrs_pipe_do_io(struct rrs_ctx *ctx, int timeout_ms, ssize_t *rc_io, ssize_t *rc_read, ssize_t *rc_write) {
	fprintf(stderr, "pipe_do_io[%p]: timeout=%d\n", ctx, timeout_ms);

	uint16_t bufsize = ctx->Params.LengthMax_Data;
	uint8_t *buf = alloca(bufsize);

	struct pollfd pfd_pipe;

	pfd_pipe.fd = ctx->FD_Pipe[0];
	pfd_pipe.events = POLLIN | POLLOUT | POLLERR | POLLHUP;

	int rc_poll = poll(&pfd_pipe, 1, timeout_ms);

	ssize_t rc_rread, rc_rwrite, rc_pread = 0, rc_pwrite = 0;

	fprintf(stderr, "pipe_do_io[%p]: poll: %d\n", ctx, rc_poll);

	if (rc_poll > 0) {
		if (rc_io)
			*rc_io = 0;

		if (pfd_pipe.revents & POLLOUT) {
			rc_rread = rrs_read(ctx, bufsize, buf);
			fprintf(stderr, "pipe_do_io[%p]: rrs read: %zd\n", ctx, rc_rread);
			if (rc_rread > 0) {
				rc_pwrite = rrs_util_writeall(ctx->FD_Pipe[0], buf, (size_t) rc_rread);
				fprintf(stderr, "pipe_do_io[%p]: pipe write: %zd\n", ctx, rc_pwrite);
				if (rc_write)
					*rc_write = rc_pwrite;
			} else {
				if ((rc_rread % 100) == RRS_STATUS_IO_BADMAC ||
				    (rc_rread % 100) == RRS_STATUS_IO_TIMEOUT) {
					// Do nothing here
				} else {
					if (rc_io)
						*rc_io = rc_rread;
					return;
				}
			}
		}

		if (pfd_pipe.revents & POLLIN) {
			rc_pread = read(ctx->FD_Pipe[0], buf, bufsize);
			fprintf(stderr, "pipe_do_io[%p]: pipe read: %zd\n", ctx, rc_pread);

			if (rc_read)
				*rc_read = rc_pread;

			if (rc_pread > 0) {
				do {
					rc_rwrite = rrs_write(ctx, (uint16_t) rc_pread, buf);
					fprintf(stderr, "pipe_do_io[%p]: rrs write: %zd\n", ctx, rc_rwrite);
				} while ((rc_rwrite % 100) == RRS_STATUS_IO_BADMAC ||
					 (rc_rwrite % 100) == RRS_STATUS_IO_TIMEOUT);

				if (rc_rwrite < 0) {
					if (rc_io)
						*rc_io = rc_rwrite;
					return;
				}
			}
		}
	}
}

void *rrs_pipe_io_thread(void *userp) {
	ssize_t rc_io = 0;
	struct rrs_ctx *ctx = (struct rrs_ctx *)userp;

	do {
		rrs_pipe_do_io(ctx, ctx->Params.Timeout_IO_PipeThread, &rc_io, NULL, NULL);
	} while (rc_io != RRS_STATUS_IO_TAMPERED || rc_io != RRS_STATUS_IO_SYSERR);

	close(ctx->FD_Pipe[0]);

	return (void *)(intptr_t *)(intptr_t)rc_io;
}