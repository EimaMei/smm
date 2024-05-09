#include <sili.h>

/*
 * SPECIFICATION:
 * Description:
 * The Silifig format is a file format used for storing any type of data information
 * in a very quick and easy to parse manner, while also being very flexibile and
 * programmer-friendly. The format is not meant to be human-readable friendly,
 * it specializes purely on simplicity and performance.
 *
 * Silifig uses an entry system, where each entry has a specified length of the
 * data and then the data itself right after. Silifig additionally provides categories
 * as another option, acting like an array of entries for when the data requires
 * more  organization. A category header contains two unsigned 32-bit integers,
 * one for  the size of the category in bytes (not counting the size of the
 * entry/category headers), other for how denoting the amount of entries in the
 * category. A category header always goes before the actual category.
 *
 * Silifig has no other strict limitations or rules that need to be followed.
 * How should the data should be stored, read or written is up to the application
 * to decide. The specification only provides the bare necessities to store any
 * kind of datas, thus there are no checksums, 'frames', endianness denotaations
 * or other any kind of fluff.
 *
 *
 * Structure (without categories):
 * | Header (16 bytes) | Entry header (4 bytes) | Entry data (N bytes) | ... | EOF
 * Example:
 * | 0x01 0x00 0x00000001 0x0 0x0000000 | 0x00000006 | 'L' 'a' 'b' 'a' 's' '!' | EOF
 *
 * Structure (with categories):
 * | Header (16 bytes) | Category header (8 bytes) | Entry header (4 bytes) | entry's data (N bytes) | ... | EOF
 * Example:
 * | 0x01 0x00 0x00000002 0x1 0x0000000 | 0x00000006 0x00000001 | 0x00000006 | 'L' 'a' 'b' 'a' 's' '!' |
 *										| 0x00000006 0x00000001 | 0x00000006 | 'H' 'e' 'l' 'l' 'o' '!' | EOF

 *
 * Reading:
 * File type - .sfg
 * Ending - little endian.
 *
 * File header:
 * u8  - Silifig version (starting from version 0).
 * u8  - Blank byte, must be set to 0.
 * u16 - The amount of entries in a file. If SI_CONFIG_CATEGORIES is set, this
 *		 becomes the number of categories instead.
 * u1  - Additional features bitfield. So far only reaches up to bit 0.
 * u31 - Blank bytes, must be set to 0.
 * u64 - The size of the entire data inside, not counting the size of entry or
 *		 category headers
 *
 * Entry header:
 * u32 - Length of the data (not counting the size of the header).
 *
 * Category header:
 * u32 - Length of the category's data (not counting the entry and category header sizes)
 * u32 - Amount of entries in the category.
 *
 *
*/

typedef struct {
    u8 version;
    u8 __blank0;
    u16 entriesLen;
	u32 config : 1;
    u32 __blank1 : 31;
	u64 dataSize;
} siConfigHeader;

typedef SI_ENUM(u8, siConfigOptions) {
	SI_CONFIG_CATEGORIES = SI_BIT(0),
};

typedef struct {
    u32 len;
    rawptr value;
} siConfigEntry;

typedef struct {
	u32 dataSize;
	u32 entryCount;
	rawptr data;
} siConfigCategory;


usize silifig_configMake(u32 version, u32 amount, siConfigOptions options, u64 dataSize,
		rawptr outBuffer);
usize silifig_configMakeEx(u32 version, u32 amount, siConfigOptions options,
		const rawptr inData, rawptr outBuffer);

siConfigEntry* silifig_entryRead(const rawptr content, usize index);
siConfigEntry* silifig_entryReadFromCategory(const siConfigCategory* category, usize index);

siConfigCategory* silifig_categoryRead(const rawptr content, usize index);

rawptr silifig_entryGetData(siConfigEntry* entry);

void silifig_entryNext(siConfigEntry** outEntry);

#if defined(SILIFIG_IMPLEMENTATION)


usize silifig_configMake(u32 version, u32 amount, siConfigOptions options,
		u64 dataSize, rawptr outBuffer) {
    SI_ASSERT_NOT_NULL(outBuffer);


    siConfigHeader header = {0};
    header.version = version;
    header.entriesLen = si_swap16le(amount);
	header.config = options;
	header.dataSize = si_swap16le(dataSize);
    memcpy(outBuffer, &header, sizeof(header));

	return sizeof(header);
}
usize silifig_configMakeEx(u32 version, u32 amount, siConfigOptions options,
		const rawptr inData, rawptr outBuffer) {

	usize offset = silifig_configMake(version, amount, options, 0, outBuffer);
	siByte* buf = (siByte*)outBuffer;
	u64 dataSize = 0;

	if ((options & SI_CONFIG_CATEGORIES) == 0) {
		for_range (i, 0, amount) {
			siConfigEntry entry = ((const siConfigEntry*)inData)[i];

			memcpy(&buf[offset], &entry.len, sizeof(entry.len));
			offset += sizeof(entry.len);
			memcpy(&buf[offset], entry.value, entry.len);
			offset += entry.len;

			dataSize += entry.len;
		}
	}
	else {
		for_range (i, 0, amount) {
			siConfigCategory category = ((const siConfigCategory*)inData)[i];

			memcpy(&buf[offset], &category.dataSize, sizeof(category.dataSize));
			offset += sizeof(u32);
			memcpy(&buf[offset], &category.entryCount, sizeof(category.entryCount));
			offset += sizeof(u32);

			for_range (j, 0, category.entryCount) {
				const siConfigEntry entry = ((const siConfigEntry*)category.data)[j];

				memcpy(&buf[offset], &entry.len, sizeof(entry.len));
				offset += sizeof(entry.len);
				memcpy(&buf[offset], entry.value, entry.len);
				offset += entry.len;

				dataSize += entry.len;
			}
		}
	}
	siConfigHeader* header = (siConfigHeader*)outBuffer;
	header->dataSize = dataSize;

    return offset;
}

siConfigEntry* silifig_entryRead(const rawptr content, usize index) {
	SI_ASSERT_NOT_NULL(content);

	siByte* buf = content;
	siConfigHeader* header = (siConfigHeader*)buf;
	SI_ASSERT(index < header->entriesLen);

	usize offset = sizeof(siConfigHeader);
	while (index) {
		siConfigEntry* entry = (siConfigEntry*)&buf[offset];
		offset += sizeof(u32) + entry->len;
		index -= 1;
	}

	return (siConfigEntry*)&buf[offset];
}

siConfigEntry* silifig_entryReadFromCategory(const siConfigCategory* category, usize index) {
	SI_ASSERT_NOT_NULL(category);
	SI_ASSERT(index < category->entryCount);

	const siByte* buf = (const siByte*)category;
	usize offset = sizeof(u32) * 2;

	while (index) {
		siConfigEntry* entry = (siConfigEntry*)&buf[offset];
		offset += sizeof(u32) + entry->len;
		index -= 1;
	}

	return (siConfigEntry*)&buf[offset];
}

void silifig_entryNext(siConfigEntry** outEntry) {
	siConfigEntry* entry = *outEntry;

	*outEntry = si_cast(siConfigEntry*, (siByte*)entry + sizeof(u32) + entry->len);
}

siConfigCategory* silifig_categoryRead(const rawptr content, usize index) {
	SI_ASSERT_NOT_NULL(content);

	const siByte* buf = (const siByte*)content;
	siConfigHeader* header = (siConfigHeader*)buf;
	SI_ASSERT(index < header->entriesLen);
	SI_ASSERT(header->config & SI_CONFIG_CATEGORIES);

	usize offset = sizeof(siConfigHeader);
	while (index) {
		siConfigCategory* entry = (siConfigCategory*)&buf[offset];
		offset += sizeof(u32) * 2 + entry->dataSize + entry->entryCount * sizeof(u32);
		index -= 1;
	}

	return (siConfigCategory*)&buf[offset];
}

rawptr silifig_entryGetData(siConfigEntry* entry) {
	return &entry->len + 1;
}


#endif
