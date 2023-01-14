
#include "ninutils/dol.hpp"
#include "ninutils/utils.hpp"

#define DEF_DOL_TEXT_SECTION_NAME ".text"
#define DEF_DOL_DATA_SECTION_NAME ".data"
#define DEF_DOL_BSS_SECTION_NAME ".bss"

#define DOLHDR_OFFSETS_OFF 0x0
#define DOLHDR_OFFSET_SIZE 0x4
#define DOLHDR_ADDRESSES_OFF 0x48
#define DOLHDR_ADDRESS_SIZE 0x4
#define DOLHDR_SIZES_OFF 0x90
#define DOLHDR_SIZE_SIZE 0x4
#define DOLHDR_BSSADDR_OFF 0xd8
#define DOLHDR_BSSLEN_OFF 0xdc
#define DOLHDR_ENTRYPOINT_OFF 0xe0

namespace ninutils {

DolHeaderRaw::DolHeaderRaw(uint8_t* dol) {
    for (int i = 0; i < DOL_MAX_SECTION_COUNT; i++) {
        offsets[i] = readbe32(dol + DOLHDR_OFFSETS_OFF + i*DOLHDR_OFFSET_SIZE);
        addresses[i] = readbe32(dol + DOLHDR_ADDRESSES_OFF + i*DOLHDR_ADDRESS_SIZE);
        lengths[i] = readbe32(dol + DOLHDR_SIZES_OFF + i*DOLHDR_SIZE_SIZE);
    }

    bss_address = readbe32(dol + DOLHDR_BSSADDR_OFF);
    bss_length = readbe32(dol + DOLHDR_BSSLEN_OFF);
    entry_point = readbe32(dol + DOLHDR_ENTRYPOINT_OFF);
}

DolSection::DolSection(uint32_t offset, uint32_t address, uint32_t length, bool text)
        : offset(offset), address(address), length(length), isText(text) {
}

void Dol::setSectionName(uint8_t sec, std::optional<ExtraInfo> extra_info) {
    if (extra_info.has_value() && extra_info->modules.find(0) != extra_info->modules.end()) {
        secs[sec].name = extra_info->modules[0].sections[sec].name;
        return;
    }

    // generate sensible default name
    uint8_t similarSectionTypeCount = 0;
    uint8_t sectionTypeIdx = 0;
    for (uint8_t i = 0; i < secs.size(); i++) {
        if (i == sec) continue;

        if (secs[i].isText == secs[sec].isText && secs[i].isBss() == secs[sec].isBss()) {
            similarSectionTypeCount++;
            if (i < sec) sectionTypeIdx++;
        }
    }
    std::string sectionBaseName;
    if (secs[sec].isText) {
        sectionBaseName = DEF_DOL_TEXT_SECTION_NAME;
    } else if (secs[sec].isBss()) {
        sectionBaseName = DEF_DOL_DATA_SECTION_NAME;
    } else {
        sectionBaseName = DEF_DOL_BSS_SECTION_NAME;
    }
    if (similarSectionTypeCount == 0) {
        secs[sec].name = secs[sec].isText ? DEF_DOL_TEXT_SECTION_NAME : DEF_DOL_DATA_SECTION_NAME;
    } else {
        secs[sec].name = (secs[sec].isText ? DEF_DOL_TEXT_SECTION_NAME : DEF_DOL_DATA_SECTION_NAME) +
            std::to_string(similarSectionTypeCount+1);
    }
}

Dol::Dol(uint8_t* dol, size_t size, std::optional<ExtraInfo> extra_info) : hdr(dol), fileSize(size) {
    secs.reserve(DOL_MAX_SECTION_COUNT);
    for (int i = 0; i < DOL_MAX_SECTION_COUNT; i++) {
        if (hdr.offsets[i] != 0 && hdr.addresses[i] != 0 && hdr.lengths[i] != 0) {
            secs.emplace_back(hdr.offsets[i], hdr.addresses[i], hdr.lengths[i], i < 7);
            setSectionName(secs.size()-1, extra_info);
        }
    }
    secs.shrink_to_fit();

    memcpy(file, dol, size);
}

Dol::~Dol() {
    if (file != nullptr)
        free(file);
}

std::ostream& DolHeaderRaw::print(std::ostream& os) const {
    os << "Sections:" << "\n";
    os << WIDTH("Offset", 12) << WIDTH("Address", 12) << WIDTH("Size", 10) << "\n";
    for (int i = 0; i < DOL_MAX_SECTION_COUNT; i++) {
        os << HEX_FMTW(offsets[i], 12) << HEX_FMTW(addresses[i], 12) << HEX_FMTW(lengths[i], 12) << "\n";
    }
    os << WIDTH(".bss address", 14) << HEX_FMT(bss_address) << "\n";
    os << WIDTH(".bss length", 14) << HEX_FMT(bss_length) << "\n";
    os << WIDTH("entry point", 14) << HEX_FMT(entry_point) << "\n";
    return os;
}

std::ostream& DolSection::print(std::ostream& os) const {
    os << HEX_FMTW(offset, 12) << HEX_FMTW(address, 12) << HEX_FMTW(length, 12) << "\n";
    return os;
}

std::ostream& Dol::print(std::ostream& os) const {
    os << "Sections:" << "\n";
    os << WIDTH("Offset", 12) << WIDTH("Address", 12) << WIDTH("Size", 10) << "\n";
    for (int i = 0; i < secs.size(); i++) {
        secs[i].print(os);
    }
    os << WIDTH(".bss address", 14) << HEX_FMT(hdr.bss_address) << "\n";
    os << WIDTH(".bss length", 14) << HEX_FMT(hdr.bss_length) << "\n";
    os << WIDTH("entry point", 14) << HEX_FMT(hdr.entry_point) << "\n";
    return os;
}
} // ns ninutils