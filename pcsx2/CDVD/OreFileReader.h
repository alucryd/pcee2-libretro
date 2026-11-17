// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "CDVD/ThreadedFileReader.h"

#include <smelt.h>

/// An ORE, oxyromon's disc format, read through libsmelt: the image it was
/// made from, a DVD's ISO or a CD's tracks end to end, which the detector
/// reads as it reads a flat image.
class OreFileReader final : public ThreadedFileReader
{
	DeclareNoncopyableObject(OreFileReader);

	// smelt.h, not smelt.hpp: the core is built without exceptions
	smelt_ingot* m_ingot = nullptr;
	/// Where the first track's index 01 lies in the byte view: past a pregap
	/// the ORE holds, which a flat image of it would not.
	u64 m_start = 0;
	u64 m_size = 0;

public:
	OreFileReader();
	~OreFileReader() override;

	bool Open2(std::string filename, Error* error) override;

	Chunk ChunkForOffset(u64 offset) override;
	int ReadChunk(void* dst, s64 chunkID) override;

	void Close2() override;

	u32 GetBlockCount() const override;
};
