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


void rrs_init_ctx(struct rrs_ctx *ctx, struct rrs_params *params) {
	memset(ctx, 0, sizeof(struct rrs_ctx));
	memcpy(&ctx->Params, params, sizeof(struct rrs_params));

	ctx->Params.Length_PacketHeader = RRS_CONST_RAWHDRSIZE + params->Length_Checksum;
	ctx->Params.LengthMax_Packet = (uint16_t)(ctx->Params.Length_PacketHeader + params->LengthMax_Data + 2);

	ctx->Buffers.LastEncodedPacket = malloc(ctx->Params.LengthMax_Packet);
	ctx->Buffers.LastReceivedPacket = malloc(ctx->Params.LengthMax_Packet);

	ctx->Buffers.LastReceivedPacket_Header = ctx->Buffers.LastReceivedPacket;
	ctx->Buffers.LastReceivedPacket_Data = ctx->Buffers.LastReceivedPacket + ctx->Params.Length_PacketHeader;

	ctx->Buffers.PacketNum_Local = 1;

#ifdef ReimuDebug
	fprintf(stderr, "rrs_init_ctx[%p]: checksum size: %d, raw header size: %d, packet header size: %d\n",
		ctx, ctx->Params.Length_Checksum, RRS_CONST_RAWHDRSIZE, ctx->Params.Length_PacketHeader);
#endif

}