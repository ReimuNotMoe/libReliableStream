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

ssize_t rrs_builtin_callback_read(size_t len, void *buf, void *userp, size_t timeout_ms) {
	struct rrs_builtin_callback_ctx *ctx = userp;

	if (timeout_ms) {
		int rc_select;
		struct timeval tv;
		fd_set read_fds;
		int fd = ctx->FD_Base;

		FD_ZERO(&read_fds);
		FD_SET(fd, &read_fds);

		tv.tv_sec  = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;

		rc_select = select(fd + 1, &read_fds, NULL, NULL, &tv);

		if (rc_select == 0)
			return RRS_STATUS_IO_TIMEOUT;
		else if (rc_select < 0)
			return RRS_STATUS_IO_SYSERR;
	}

	ssize_t rc_read = read(ctx->FD_Base, buf, len);

	if (rc_read >= 0)
		return rc_read;
	else
		return RRS_STATUS_IO_SYSERR;
}

ssize_t rrs_builtin_callback_write(size_t len, void *buf, void *userp){
	struct rrs_builtin_callback_ctx *ctx = userp;

	ssize_t rc_write = write(ctx->FD_Base, buf, len);

	if (rc_write >= 0)
		return rc_write;
	else
		return RRS_STATUS_IO_SYSERR;

}

ssize_t rrs_internal_bulk_read(struct rrs_ctx *ctx, size_t len, void *buf, size_t timeout_ms) {

	size_t read_bytes = 0;
	ssize_t rc_cbread;

	while (read_bytes < len) {
		rc_cbread = ctx->Callback_Read(len-read_bytes, buf+read_bytes, ctx->CallbackContext_Read, timeout_ms);

		fprintf(stderr, "low_read[%p,%zu]: %zd (%zu/%zu)\n", buf, len, rc_cbread, read_bytes, len);

		if (rc_cbread > 0)
			read_bytes += rc_cbread;
		else if (rc_cbread == 0)
			return read_bytes;
		else if (rc_cbread < 0)
			return rc_cbread + RRS_STATUS_IO_READ;
	}


	return read_bytes;
}

ssize_t rrs_internal_bulk_write(struct rrs_ctx *ctx, size_t len, void *buf) {
	size_t written_bytes = 0;
	ssize_t rc_cbwrite;

	while (written_bytes < len) {
		rc_cbwrite = ctx->Callback_Write(len-written_bytes, buf+written_bytes, ctx->CallbackContext_Write);

		fprintf(stderr, "low_write[%p,%zu]: %zd (%zu/%zu)\n", buf, len, rc_cbwrite, written_bytes, len);

		if (rc_cbwrite > 0)
			written_bytes += rc_cbwrite;
		else if (rc_cbwrite == 0)
			return written_bytes;
		else if (rc_cbwrite < 0)
			return rc_cbwrite + RRS_STATUS_IO_WRITE;
	}


	return written_bytes;
}

void rrs_blackhole_input(struct rrs_ctx *ctx) {
	uint8_t buf[128];

	while (rrs_internal_bulk_read(ctx, ctx->Params.Length_PacketHeader, ctx->Buffers.LastReceivedPacket,
				      ctx->Params.Timeout_IO_Blackhole) > 0);

}

ssize_t rrs_send_ack(struct rrs_ctx *ctx) {
	rrs_packet_ack_encode(ctx, ctx->Buffers.PacketNum_Local);
	ssize_t rc = rrs_internal_bulk_write(ctx, ctx->Buffers.Length_LastEncodedPacket,
					     ctx->Buffers.LastEncodedPacket);

	if (rc > 0) {
		rrs_packet_local_increment(ctx);
	}

	return rc;
}

ssize_t rrs_send_nak(struct rrs_ctx *ctx) {
	rrs_packet_nak_encode(ctx, ctx->Buffers.PacketNum_Local);
	ssize_t rc = rrs_internal_bulk_write(ctx, ctx->Buffers.Length_LastEncodedPacket,
					     ctx->Buffers.LastEncodedPacket);

	if (rc > 0) {
		rrs_packet_local_increment(ctx);
	}

	return rc;
}

ssize_t rrs_write(struct rrs_ctx *ctx, uint16_t len, void *buf) {
	rrs_packet_encode(ctx, RRS_PKTYPE_USERDATA, ctx->Buffers.PacketNum_Local, len, buf);

	ssize_t rc_bwrite, rc_bread;

	rc_bwrite = rrs_internal_bulk_write(ctx, ctx->Buffers.Length_LastEncodedPacket,
					    ctx->Buffers.LastEncodedPacket);

	fprintf(stderr, "rrs_write[%p]: sent packet, rc=%zd\n", ctx, rc_bwrite);

	if (rc_bwrite < 0)
		return rc_bwrite;

	fprintf(stderr, "rrs_write[%p]: recving ACK\n", ctx);

	rc_bread = rrs_internal_bulk_read(ctx, ctx->Params.Length_PacketHeader, ctx->Buffers.LastReceivedPacket,
					  ctx->Params.Timeout_IO_Resend);
	if (rc_bread < 0)
		return rc_bread;

	if (!rrs_packet_verify_header(ctx, ctx->Buffers.LastReceivedPacket)) { // Bad header
		rrs_blackhole_input(ctx);
		return RRS_STATUS_IO_BADMAC;
	}

	fprintf(stderr, "rrs_write[%p]: recv'd pkt header correct\n", ctx);

	struct rrs_packet_header *hdr = (struct rrs_packet_header *)(ctx->Buffers.LastReceivedPacket +
					ctx->Params.Length_Checksum);

	if (hdr->Type != RRS_PKTYPE_ACK) {
		fprintf(stderr, "rrs_write[%p]: recv'd pkt isn't an ACK pkt (%u)\n", ctx, hdr->Type);
		rrs_blackhole_input(ctx);
		return RRS_STATUS_IO_BADMAC;
	}

	fprintf(stderr, "pkt_num[%p]: local=%u, remote_last=%u, remote_now=%u\n", ctx, ctx->Buffers.PacketNum_Local,
		ctx->Buffers.PacketNum_Remote, hdr->Number);

	if (hdr->Number == ctx->Buffers.PacketNum_Remote) { // Check for duplicated packet
		rrs_blackhole_input(ctx);
		return RRS_STATUS_IO_TAMPERED; // TODO: Validate behavior
	} else if (hdr->Number < ctx->Buffers.PacketNum_Remote && ctx->Buffers.PacketNum_Remote != UINT16_MAX) { // And bad pktnum
		rrs_blackhole_input(ctx);
		return RRS_STATUS_IO_TAMPERED;
	}

	fprintf(stderr, "rrs_write[%p]: recv'd pkt num correct\n", ctx);

	ctx->Buffers.PacketNum_Remote = hdr->Number;
	rrs_packet_local_increment(ctx);

	return rc_bwrite;
}

ssize_t rrs_read(struct rrs_ctx *ctx, uint16_t len, void *buf) {
	ssize_t rc_bwrite, rc_bread, rc_dread, rc_kkk;

	rc_bread = rrs_internal_bulk_read(ctx, ctx->Params.Length_PacketHeader, ctx->Buffers.LastReceivedPacket,
					  ctx->Params.Timeout_IO_Read);

	if (rc_bread < 0)
		return rc_bread;

	if (!rrs_packet_verify_header(ctx, ctx->Buffers.LastReceivedPacket)) { // Header checksum
		rrs_blackhole_input(ctx);
		rc_kkk = rrs_send_nak(ctx);
		if (rc_kkk > 0)
			return RRS_STATUS_IO_BADMAC;
		else
			return rc_kkk;
	}

	fprintf(stderr, "rrs_read[%p]: recv'd pkt header correct\n", ctx);

	struct rrs_packet_header *hdr = (struct rrs_packet_header *)(ctx->Buffers.LastReceivedPacket +
					ctx->Params.Length_Checksum);

	fprintf(stderr, "pkt_num[%p]: local=%u, remote_last=%u, remote_now=%u\n", ctx, ctx->Buffers.PacketNum_Local,
		ctx->Buffers.PacketNum_Remote, hdr->Number);

	if (hdr->Number == ctx->Buffers.PacketNum_Remote) { // Check for duplicated packet
		rrs_blackhole_input(ctx);
		rc_kkk = rrs_send_ack(ctx);
		if (rc_kkk > 0)
			return RRS_STATUS_IO_DUP;
		else
			return rc_kkk;
	} else if (hdr->Number < ctx->Buffers.PacketNum_Remote && ctx->Buffers.PacketNum_Remote != UINT16_MAX) { // And bad pktnum
		rrs_blackhole_input(ctx);
		return RRS_STATUS_IO_TAMPERED;
	}

	fprintf(stderr, "rrs_read[%p]: recv'd pkt num correct\n", ctx);

	if (!rrs_packet_assert_data_length(ctx, hdr->DataLength)) { // Check if data length is valid
		rrs_blackhole_input(ctx);
		return RRS_STATUS_IO_TAMPERED;
	}

	fprintf(stderr, "rrs_read[%p]: recv'd pkt data len correct\n", ctx);

	// Read remaining data
	rc_bread = rrs_internal_bulk_read(ctx, hdr->DataLength + ctx->Params.Length_Checksum, ctx->Buffers.LastReceivedPacket_Data,
					  ctx->Params.Timeout_IO_Read);

	if (rc_bread < 0)
		return rc_bread;

	if (!rrs_packet_verify_data(ctx, ctx->Buffers.LastReceivedPacket_Data, hdr->DataLength)) {
		rrs_blackhole_input(ctx);
		return RRS_STATUS_IO_BADMAC;
	}

	fprintf(stderr, "rrs_read[%p]: recv'd pkt data correct\n", ctx);

	// Finally everything is correct
	rc_dread = rrs_packet_extract_data(ctx, hdr, buf, len);

	ctx->Buffers.PacketNum_Remote = hdr->Number;

	// Give remote an ACK
	rc_kkk = rrs_send_ack(ctx);
	if (rc_kkk > 0) {
		return rc_dread;
	} else
		return rc_kkk;
}