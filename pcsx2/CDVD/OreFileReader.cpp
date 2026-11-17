// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "OreFileReader.h"

#include "common/Console.h"
#include "common/Error.h"

#include <algorithm>
#include <cstring>

// 147 sectors of 2048 bytes, 128 of 2352: whole sectors of a DVD or a CD
static constexpr u32 CHUNK_SIZE = 2048 * 147;

OreFileReader::OreFileReader() = default;

OreFileReader::~OreFileReader()
{
	Close2();
}

// shortcut: opened by path, not through the frontend's VFS, as libsmelt
// reads files itself; a VFS-only frontend (Android SAF) needs a reader
// callback in its ABI
bool OreFileReader::Open2(std::string filename, Error* error)
{
	Close2();
	m_filename = std::move(filename);
	u64 size = 0;
	size_t count = 0;
	smelt_track track = {};
	if (smelt_open(m_filename.c_str(), nullptr, &m_ingot) != SMELT_OK ||
		smelt_logical_size(m_ingot, &size) != SMELT_OK ||
		smelt_track_count(m_ingot, &count) != SMELT_OK ||
		(count > 0 && smelt_track_at(m_ingot, 0, &track) != SMELT_OK))
	{
		Error::SetStringView(error, smelt_last_error());
		Close2();
		return false;
	}
	if (count == 0)
	{
		// Not a CD: a PlayStation 2 DVD, not a ROM or another's disc, which
		// the detector would take for an audio CD
		char platform[16];
		size_t len = sizeof(platform);
		if (smelt_platform(m_ingot, platform, &len) != SMELT_OK || std::strcmp(platform, "ps2") != 0)
		{
			Error::SetStringView(error, "The ORE is not a PlayStation 2 disc.");
			Close2();
			return false;
		}
	}
	m_start = count > 0 ? track.offset + static_cast<u64>(track.pregap) * SMELT_SECTOR : 0;
	if (size <= m_start)
	{
		Error::SetStringView(error, "The ORE holds no sectors.");
		Close2();
		return false;
	}
	m_size = size - m_start;
	return true;
}

ThreadedFileReader::Chunk OreFileReader::ChunkForOffset(u64 offset)
{
	Chunk chunk = {};
	if (offset >= m_size)
	{
		chunk.chunkID = -1;
	}
	else
	{
		chunk.chunkID = offset / CHUNK_SIZE;
		chunk.offset = static_cast<u64>(chunk.chunkID) * CHUNK_SIZE;
		chunk.length = static_cast<u32>(std::min<u64>(m_size - chunk.offset, CHUNK_SIZE));
	}
	return chunk;
}

int OreFileReader::ReadChunk(void* dst, s64 chunkID)
{
	if (chunkID < 0 || !m_ingot)
		return -1;
	const u64 offset = static_cast<u64>(chunkID) * CHUNK_SIZE;
	if (offset >= m_size)
		return -1;
	const u32 len = static_cast<u32>(std::min<u64>(m_size - offset, CHUNK_SIZE));
	if (smelt_read(m_ingot, m_start + offset, static_cast<u8*>(dst), len) != SMELT_OK)
	{
		Console.Error("CDVD: ORE read at %llu: %s", static_cast<unsigned long long>(offset), smelt_last_error());
		return 0;
	}
	return static_cast<int>(len);
}

void OreFileReader::Close2()
{
	smelt_close(m_ingot);
	m_ingot = nullptr;
	m_start = 0;
	m_size = 0;
}

u32 OreFileReader::GetBlockCount() const
{
	return static_cast<u32>((m_size - m_dataoffset) / m_blocksize);
}
