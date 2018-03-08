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

#ifndef Reimu_ReliableStream_H
#define Reimu_ReliableStream_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/un.h>
#include <sys/poll.h>
#include <sys/socket.h>

#define ReimuDebug

#define RRS_CONST_RAWHDRSIZE			((uint8_t)6) // Raw header size

#define RRS_STATUS_INCOMPLETE_IO		(-11)

#define RRS_STATUS_IO_BADMAC			(-10)
#define RRS_STATUS_IO_TAMPERED			(-11)
#define RRS_STATUS_IO_TIMEOUT			(-12)
#define RRS_STATUS_IO_SYSERR			(-13)
#define RRS_STATUS_IO_DUP			(-14)

#define RRS_STATUS_IO_READ			(-100)
#define RRS_STATUS_IO_WRITE			(-200)

#define RRS_PKTYPE_USERDATA			(UINT16_MAX >> 1)
#define RRS_PKTYPE_ACK				(UINT16_MAX >> 2)
#define RRS_PKTYPE_NAK				(UINT16_MAX >> 3)


struct rrs_params {
	uint16_t Length_Checksum;
	uint16_t Length_PacketHeader;

	uint16_t LengthMax_Data;
	uint16_t LengthMax_Packet;

	size_t Timeout_IO_Read;
	size_t Timeout_IO_Blackhole; // Discard incoming data for some time within this interval
	size_t Timeout_IO_Resend;
};

struct __attribute__((__packed__)) rrs_packet_header {
	volatile uint16_t Type;
	volatile uint16_t Number;
	volatile uint16_t DataLength;
};

struct rrs_buffers {
	uint16_t PacketNum_Local;
	uint16_t PacketNum_Remote;

	uint8_t *LastEncodedPacket;
	uint16_t Length_LastEncodedPacket;
	uint8_t *Checksum_Header_LastEncodedPacket;
	uint8_t *Checksum_Data_LastEncodedPacket;

	uint8_t *LastReceivedPacket;
	uint8_t *LastReceivedPacket_Header;
	uint8_t *LastReceivedPacket_Data;

	uint8_t *Checksum_Header_LastReceivedPacket;
	uint8_t *Checksum_Data_LastReceivedPacket;
	uint16_t Length_Data_LastReceivedPacket;
};

struct rrs_ctx {
	int Status;
	struct rrs_params Params;
	struct rrs_buffers Buffers;

	ssize_t (*Callback_Read)(size_t len, void *buf, void *userp, size_t timeout_usec);
	ssize_t (*Callback_Write)(size_t len, void *buf, void *userp);
	void (*Callback_Checksum)(size_t len, void *buf, void *buf_cksum, void *userp);

	void *CallbackContext_Read;
	void *CallbackContext_Write;
	void *CallbackContext_Checksum;

	size_t Size_LastEncodedPacket;

};

struct rrs_builtin_callback_ctx {
	int FD_Base;

};

#define rrs_packet_local_increment(ctx)		{(ctx)->Buffers.PacketNum_Local++;if ((ctx)->Buffers.PacketNum_Local == 0)\
(ctx)->Buffers.PacketNum_Local = 1;}

extern void rrs_builtin_callback_checksum_crc16(size_t len, void *buf, void *buf_cksum, void *userp);

extern ssize_t rrs_builtin_callback_read(size_t len, void *buf, void *userp, size_t timeout_ms);
extern ssize_t rrs_builtin_callback_write(size_t len, void *buf, void *userp);

extern void rrs_init_ctx(struct rrs_ctx *ctx, struct rrs_params *params);

extern ssize_t rrs_write(struct rrs_ctx *ctx, uint16_t len, void *buf);
extern ssize_t rrs_read(struct rrs_ctx *ctx, uint16_t len, void *buf);

//void rrs_packet_local_increment(struct rrs_ctx *ctx);

int rrs_packet_verify_header(struct rrs_ctx *ctx, void *header);
int rrs_packet_verify_data(struct rrs_ctx *ctx, void *data, uint16_t data_len);

ssize_t rrs_packet_extract_data(struct rrs_ctx *ctx, struct rrs_packet_header *pkt, void *data, uint16_t data_len);

int rrs_packet_assert_data_length(struct rrs_ctx *ctx, uint16_t data_len);

uint16_t rrs_packet_encode(struct rrs_ctx *ctx, uint16_t packet_type, uint16_t packet_num, uint16_t data_len, void *data);
uint16_t rrs_packet_data_decode(struct rrs_ctx *ctx, uint16_t data_len, void *data, void *buf);
uint16_t rrs_packet_ack_encode(struct rrs_ctx *ctx, uint16_t packet_num);
uint16_t rrs_packet_nak_encode(struct rrs_ctx *ctx, uint16_t packet_num);

#endif // Reimu_ReliableStream_H