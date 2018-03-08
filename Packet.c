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

/*
 * Structure:
 * +----------------------+--------+--------+--------+-------------+----------+
 * |       Variable       | 2 byte | 2 byte | 2 byte |   Variable  | Variable | <- Length
 * +----------------------+--------+--------+--------+-------------+----------+
 * | cksum(pktype+datlen) | pktype | pktnum | datlen | cksum(data) | data.... | <- Type
 * +----------------------+--------+--------+--------+-------------+----------+
 *
 * ACK and NAK packets doesn't have data fields
 */

//extern inline void rrs_packet_local_increment(struct rrs_ctx *ctx) {
//	ctx->Buffers.PacketNum_Local++;
//	if (ctx->Buffers.PacketNum_Local == 0)
//		ctx->Buffers.PacketNum_Local = 1;
//}

int rrs_packet_verify_header(struct rrs_ctx *ctx, void *header) {
	uint8_t *buf = alloca(ctx->Params.Length_Checksum);

	ctx->Callback_Checksum(RRS_CONST_RAWHDRSIZE, header+ctx->Params.Length_Checksum, buf, ctx->CallbackContext_Checksum);

	int emmm = memcmp(buf, header, ctx->Params.Length_Checksum);

	fprintf(stderr, "pkt_verify_header[%p]: %s\n", header, emmm ? "bad" : "good");

	return emmm == 0;
}

int rrs_packet_verify_data(struct rrs_ctx *ctx, void *data, uint16_t data_len) {
	uint8_t *buf = alloca(ctx->Params.Length_Checksum);

	ctx->Callback_Checksum(data_len, data+ctx->Params.Length_Checksum, buf, ctx->CallbackContext_Checksum);

	int emmm = memcmp(buf, data, ctx->Params.Length_Checksum);

	fprintf(stderr, "pkt_verify_data[%p,%u]: %s\n", data, data_len, emmm ? "bad" : "good");

	return emmm == 0;
}

ssize_t rrs_packet_extract_data(struct rrs_ctx *ctx, struct rrs_packet_header *pkt, void *data, uint16_t data_len) {
	// TODO

	if (data_len > pkt->DataLength)
		data_len = pkt->DataLength;

	memcpy(data, ctx->Buffers.LastReceivedPacket_Data + ctx->Params.Length_Checksum, data_len);

	return data_len;
}

int rrs_packet_assert_data_length(struct rrs_ctx *ctx, uint16_t data_len) {
	if (data_len > ctx->Params.LengthMax_Data)
		return 0;
	else
		return 1;
}

uint16_t rrs_packet_encode(struct rrs_ctx *ctx, uint16_t packet_type, uint16_t packet_num, uint16_t data_len, void *data) {
	uint16_t len_cksum = ctx->Params.Length_Checksum;
	uint8_t *buf_packet = ctx->Buffers.LastEncodedPacket;

	// Packet type
	memcpy(buf_packet+len_cksum, &packet_type, 2);

	// Packet number
	memcpy(buf_packet+len_cksum+2, &packet_num, 2);

	// Data length
	memcpy(buf_packet+len_cksum+4, &data_len, 2);

	// Checksum of above
	ctx->Callback_Checksum(RRS_CONST_RAWHDRSIZE, buf_packet+len_cksum, buf_packet, ctx->CallbackContext_Checksum);

	ctx->Buffers.Length_LastEncodedPacket = (uint16_t)(len_cksum+RRS_CONST_RAWHDRSIZE);

	if (data_len) {
		// Data
		memcpy(buf_packet + len_cksum * 2 + RRS_CONST_RAWHDRSIZE, data, data_len);

		// Checksum of data
		ctx->Callback_Checksum(data_len, data, buf_packet + len_cksum + RRS_CONST_RAWHDRSIZE,
				       ctx->CallbackContext_Checksum);


		ctx->Buffers.Length_LastEncodedPacket += len_cksum + data_len;
	}

	return ctx->Buffers.Length_LastEncodedPacket;
}

uint16_t rrs_packet_data_decode(struct rrs_ctx *ctx, uint16_t data_len, void *data, void *buf) {
	memcpy(buf, data+ctx->Params.Length_Checksum, data_len);

	return data_len;
}

uint16_t rrs_packet_ack_encode(struct rrs_ctx *ctx, uint16_t packet_num) {
	return rrs_packet_encode(ctx, RRS_PKTYPE_ACK, packet_num, 0, NULL);
}

uint16_t rrs_packet_nak_encode(struct rrs_ctx *ctx, uint16_t packet_num) {
	return rrs_packet_encode(ctx, RRS_PKTYPE_NAK, packet_num, 0, NULL);
}