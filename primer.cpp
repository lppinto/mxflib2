/*! \file	primer.cpp
 *	\brief	Implementation of Primer class
 *
 *			The Primer class holds data about the mapping between local
 *          tags in a partition and the UL that gives access to the full
 *			definition
 */
/*
 *	Copyright (c) 2003, Matt Beard
 *
 *	This software is provided 'as-is', without any express or implied warranty.
 *	In no event will the authors be held liable for any damages arising from
 *	the use of this software.
 *
 *	Permission is granted to anyone to use this software for any purpose,
 *	including commercial applications, and to alter it and redistribute it
 *	freely, subject to the following restrictions:
 *
 *	  1. The origin of this software must not be misrepresented; you must
 *	     not claim that you wrote the original software. If you use this
 *	     software in a product, an acknowledgment in the product
 *	     documentation would be appreciated but is not required.
 *	
 *	  2. Altered source versions must be plainly marked as such, and must
 *	     not be misrepresented as being the original software.
 *	
 *	  3. This notice may not be removed or altered from any source
 *	     distribution.
 */

#include <mxflib/mxflib.h>

extern "C"
{
#include "Endian.h"
}

using namespace mxflib;


//! Read the primer from a buffer
/*!	\return Number of bytes read
 */
Uint32 Primer::ReadValue(const Uint8 *Buffer, Uint32 Size)
{
	debug("Reading Primer\n");

	// Start off empty
	clear();

	if(Size < 8)
	{
		error("Primer too small, must be at least 8 bytes!\n");
		return 0;
	}

	// Each entry in the primer is 18 bytes
	Uint32 Items = (Size-8) / 18;

	// Validate the size and only read whole items
	if((Items * 18) != (Size-8))
	{
		error("Primer not an integer number of multiples of 18 bytes!\n");
		Size = (Items * 18) + 8;
	}

	// Read the vector header
	Uint32 ClaimedItems = GetU32(Buffer);
	Uint32 ClaimedItemSize = GetU32(&Buffer[4]);
	Buffer += 8;

	if(ClaimedItemSize != 18)
	{
		error("Malformed vector header in Primer - each entry is 18 bytes, size in vector header is %d\n", ClaimedItemSize);
	}
	else
	{
		if(Items != ClaimedItems)
		{
			error("Malformed vector header in Primer - number of entries is %d, vector header claims %d\n", Items, ClaimedItems);
		}
	}

	// Read each item
	while(Items--)
	{
		Tag ThisTag = GetU16(Buffer);
		Buffer += 2;

		UL ThisUL(Buffer);
		Buffer += 16;

		// Add this new entry to the primer
		insert(Primer::value_type(ThisTag, ThisUL));

		debug("  %s -> %s\n", Tag2String(ThisTag).c_str(), ThisUL.GetString().c_str());
	}

	// Return how many bytes we actually read
	return Size;
}


//! Primer for use when no primer is available (such as for index tables)
PrimerPtr Primer::StaticPrimer;

//! Determine the tag to use for a given UL - when no primer is availabe
Tag Primer::StaticLookup(ULPtr ItemUL, Tag TryTag /*=0*/)
{
	if(!StaticPrimer) StaticPrimer = MDOType::MakePrimer();

	return StaticPrimer->Lookup(ItemUL, TryTag);
}


//! Determine the tag to use for a given UL
/*! If the UL has not yet been used the correct static or dynamic tag will 
 *	be determined and added to the primer
 *	\return The tag to use, or 0 if no more dynamic tags available
 */
Tag Primer::Lookup(ULPtr ItemUL, Tag TryTag /*=0*/)
{
	// If a tag has been suggested then try that
	if(TryTag != 0)
	{
		// Is it known by us?
		Primer::iterator it = find(TryTag);
		if(it != end())
		{
			// Only use it if the UL matches
			if(!memcmp((*it).second.GetValue(), ItemUL->GetValue(), 16)) return TryTag;
		}
		else
		{
			// It could be the right tag, but not yet in this primer
			// DRAGONS: Not implementer yet!!!
		}
	}

	// Do we have this UL already?
	std::map<UL, Tag>::iterator it = TagLookup.find(ItemUL);
	if(it != TagLookup.end())
	{
		return (*it).second;
	}

	// Try and find the type with this UL
	MDOTypePtr Type = MDOType::Find(ItemUL);
	if(Type)
	{
		const DictEntry *Dict = Type->GetDict();
		if((!Dict) || (Dict->KeyLen != 2) || (Dict->Key == NULL))
		{
			// No static tag supplied - fall through and use a dynamic tag
		}
		else
		{
			Tag ThisTag = (Dict->Key[0] << 8) + Dict->Key[1];
			insert(Primer::value_type(ThisTag, ItemUL));
			return ThisTag;
		}
	}

	// Generate a dynamic tag
	// DRAGONS: Not very efficient
	while(NextDynamic >= 0x8000)
	{
		if(find(NextDynamic) == end())
		{
			Tag Ret = NextDynamic;
			NextDynamic--;
			insert(Primer::value_type(Ret, ItemUL));
			return Ret;
		}
		NextDynamic--;
	}

	//! Out of dynamic tags!
	error("Run out of dynamic tags!\n");
	return 0;
}


//! Write this primer to a memory buffer
/*! The primer will be <b>appended</b> to the DataChunk */
Uint32 Primer::WritePrimer(DataChunk &Buffer)
{
	Uint32 Bytes;

	// Work out the primer value size first (to allow us to pre-allocate)
	Uint64 PrimerLen = Uint64(size()) * 18 + 8;

	// Re-size buffer to the probable final size
	Buffer.ResizeBuffer(Buffer.Size + 16 + 4 + PrimerLen);

	// Lookup the type to get the key - Static so only need to lookup once
	static MDOTypePtr PrimerType = MDOType::Find("Primer");
	ASSERT(PrimerType);
	static const DictEntry *PrimerDict = PrimerType->GetDict();
	ASSERT(PrimerDict);
	ASSERT(PrimerDict->Key);

	Buffer.Append(PrimerDict->KeyLen, PrimerDict->Key);
	Bytes = PrimerDict->KeyLen;

	// Add the length
	DataChunkPtr BER = MakeBER(PrimerLen);
	Buffer.Append(*BER);
	Bytes += BER->Size;

	// Add the vector header
	Uint8 Temp[4];
	PutU32(size(), Temp);
	Buffer.Append(4, Temp);
	Bytes += 4;

	PutU32(18, Temp);
	Buffer.Append(4, Temp);
	Bytes += 4;

	// Write the primer data
	iterator it = begin();
	while(it != end())
	{
		PutU16((*it).first, Temp);
		Buffer.Append(2, Temp);
		Buffer.Append(16, (*it).second.GetValue());
		it++;
	}

	return Bytes;
}

