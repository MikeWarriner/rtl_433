/** @file
    Watchman Sonic Advanced Pro oil tank level monitor (GFSK variant).

    Copyright (C) 2024

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "decoder.h"

/**
Watchman Sonic Advanced Pro oil tank level monitor (GFSK variant).

This decoder handles a GFSK-modulated variant of the Watchman Sonic
Advanced/Plus protocol. The signal uses ~543 us NRZ bit periods.

The device uses GFSK modulation at ~1843 bps (543 us per bit). The
signal may appear with either 0xAA or 0x55 preamble depending on the
FSK frequency assignment. Using -Y minmax is recommended.

The bit rate does not divide evenly into the typical 250 kHz sample
rate, causing occasional bit insertion/deletion. To reject corrupted
frames, the decoder uses two bit-level anchors: the model code near
the start and the version constant near the end. Only frames where
the two anchors are at exactly the correct spacing are accepted.

Data layout (after 40-bit preamble):

    SYNC:16h MODEL:16h ID:32h STATUS:8h TEMP:8h ?:4h DEPTH:12d VER:32h CRC:16h

- 2 byte sync word
- 2 byte model identifier (0x3020 observed)
- 4 byte device ID
- 1 byte status / flags
- 1 byte temperature (in 0.5 degree intervals, offset 0x48)
- 4 bits raw sensor nibble
- 12 bits depth (cm)
- 4 byte version constant (0x01050300)
- 2 byte CRC-16

Total: 18 bytes (144 bits) after 40-bit preamble = 184 bits.
*/

static int oil_watchman_sonic_pro_decode(r_device *decoder, bitbuffer_t *bitbuffer)
{
    // Two anchor patterns searched at the bit level:
    //   sync+model 0x5b503020 at byte offset 0 (bit offset 0 from frame start)
    //   version    0x01050300  at byte offset 12 (bit offset 96 from frame start)
    // The gap between sync_start and version_start must be exactly 96 bits.

    // Both FSK polarities produce the same logical data bytes, just at
    // different bit offsets.  No inversion needed - bitbuffer_search will
    // find the patterns at whatever bit offset they appear.
    uint8_t const sync_model_pattern[] = {0x5b, 0x50, 0x30, 0x20};
    uint8_t const version_pattern[]    = {0x01, 0x05, 0x03, 0x00};

    int events = 0;

    for (unsigned row = 0; row < bitbuffer->num_rows; ++row) {
        unsigned num_bits = bitbuffer->bits_per_row[row];
        if (num_bits < 130)
            continue;

        {
            uint8_t const *vpat = version_pattern;
            uint8_t const *spat = sync_model_pattern;

            // Search for every occurrence of the version constant
            unsigned vsearch = 0;
            while (1) {
                unsigned vpos = bitbuffer_search(bitbuffer, row, vsearch, vpat, 32);
                if (vpos >= num_bits)
                    break;
                vsearch = vpos + 1;

                // The sync+model should be exactly 96 bits before the version
                if (vpos < 96)
                    continue;
                unsigned expected_spos = vpos - 96;

                // Check that the sync+model is actually there
                uint8_t scheck[4];
                bitbuffer_extract_bytes(bitbuffer, row, expected_spos, scheck, 32);
                if (memcmp(scheck, spat, 4) != 0) {
                    decoder_logf(decoder, 2, __func__,
                            "sync+model mismatch at expected position (vpos=%u)", vpos);
                    continue;
                }

                // Both anchors verified at correct 96-bit spacing.
                // Frame starts at the sync+model position.
                unsigned frame_start = expected_spos;

                unsigned extract_bits = 144;
                if (frame_start + 144 > num_bits)
                    extract_bits = 128; // truncated CRC

                uint8_t raw[18];
                memset(raw, 0, sizeof(raw));
                bitbuffer_extract_bytes(bitbuffer, row, frame_start, raw, extract_bits);

                int mcode       = (raw[2] << 8) | raw[3];
                uint32_t id     = ((unsigned)raw[4] << 24) | (raw[5] << 16) | (raw[6] << 8) | raw[7];
                uint8_t status  = raw[8];
                float temperature = (raw[9] - 0x48) / 2.0f;
                uint16_t depth  = ((raw[10] & 0x0f) << 8) | raw[11];

                // Additional sanity: depth should be < 500 for an oil tank
                if (depth > 500) {
                    decoder_logf(decoder, 2, __func__,
                            "depth %u out of range, skipping", depth);
                    continue;
                }

                char const *mic = "CHECKSUM";
                if (extract_bits == 144 && crc16(raw, 18, 0x8005, 0) == 0)
                    mic = "CRC";

                /* clang-format off */
                data_t *data = data_make(
                        "model",                "Model",        DATA_STRING, "Oil-SonicPro",
                        "id",                   "ID",           DATA_FORMAT, "%08x", DATA_INT, id,
                        "temperature_C",        "Temperature",  DATA_FORMAT, "%.1f C", DATA_DOUBLE, (double)temperature,
                        "depth_cm",             "Depth",        DATA_INT,    depth,
                        "status",               "Status",       DATA_FORMAT, "%02x", DATA_INT, status,
                        "mic",                  "Integrity",    DATA_STRING, mic,
                        NULL);
                /* clang-format on */

                decoder_output_data(decoder, data);
                events++;
                goto next_row;
            }
        }
next_row:;
    }

    return events > 0 ? events : DECODE_FAIL_SANITY;
}

static char const *const output_fields[] = {
        "model",
        "id",
        "temperature_C",
        "depth_cm",
        "status",
        "mic",
        NULL,
};

r_device const oil_watchman_sonic_pro = {
        .name        = "Watchman Sonic Advanced Pro (GFSK)",
        .modulation  = FSK_PULSE_PCM,
        .short_width = 500,
        .long_width  = 500,
        .reset_limit = 12500,
        .decode_fn   = &oil_watchman_sonic_pro_decode,
        .fields      = output_fields,
};
