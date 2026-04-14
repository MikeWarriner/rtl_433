/** @file
    Watchman Sonic Advanced/Plus oil tank level monitor.

    Copyright (C) 2023 Gareth Potter

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "decoder.h"

/**
Watchman Sonic Advanced/Plus oil tank level monitor.

Tested devices:
- Watchman Sonic Advanced, model code 0x0401 (seen on two devices)
- Tekelek, model code 0x0106 (seen on two devices)

The devices uses GFSK with Manchester-encoded data at ~1032 us bit period.
Using -Y minmax should be sufficient to get it to work.

The NRZ slicer at 1000 us captures Manchester half-bits directly.
Manchester decoding is applied to recover the 64-bit payload.

Data Layout (64 bits after Manchester decoding):

- 4 byte unit ID (changes when rebinding)
- 1 byte flags/status:
  - 0x80 - calibrating or initial period
  - 0xa0 - normal operation
- 6 bits temperature (inversely proportional, formula: (145 - 5*val) / 3)
- 10 bits depth (distance from sensor to oil in cm)
- 1 byte CRC-8 poly 0x8c init 0x00 (bit-reflected)

The NRZ preamble is a run of alternating Manchester half-bits (101010...)
followed by a sync break. The Manchester decoder auto-synchronises on this.
*/

static int oil_watchman_advanced_decode_me(r_device *decoder, bitbuffer_t *bitbuffer)
{
    int events = 0;

    for (unsigned row = 0; row < bitbuffer->num_rows; ++row) {
        unsigned num_bits = bitbuffer->bits_per_row[row];

        // Need at least ~130 NRZ half-bits for a 64-bit Manchester message
        if (num_bits < 100) {
            continue;
        }

        // Try multiple starting offsets to find the correct Manchester phase.
        // The best offset produces the most decoded bits (fewest violations).
        bitbuffer_t mc_bits = {0};
        unsigned best_start = 0;
        unsigned best_bits  = 0;

        for (unsigned start = 0; start < 8 && start < num_bits; ++start) {
            bitbuffer_t trial = {0};
            bitbuffer_manchester_decode(bitbuffer, row, start, &trial, 0);
            if (trial.bits_per_row[0] > best_bits) {
                best_bits  = trial.bits_per_row[0];
                best_start = start;
            }
        }

        mc_bits = (bitbuffer_t){0};
        bitbuffer_manchester_decode(bitbuffer, row, best_start, &mc_bits, 0);

        if (mc_bits.bits_per_row[0] < 64) {
            continue;
        }

        uint8_t *b = mc_bits.bb[0];

        decoder_logf(decoder, 1, __func__, "MC start=%u bits=%u: %02x %02x %02x %02x %02x %02x %02x %02x",
                best_start, mc_bits.bits_per_row[0],
                b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);

        if (b[7] != crc8le(b, 7, 0x31, 0)) {
            decoder_logf(decoder, 1, __func__, "failed CRC check (got %02x, expected %02x)",
                    b[7], crc8le(b, 7, 0x31, 0));
            continue;
        }

        // as printed on the side of the unit
        uint32_t unit_id = ((unsigned)b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];

        uint8_t flags = b[4];

        // Temperature: inversely proportional, 6-bit field
        uint8_t maybetemp  = b[5] >> 2;
        double temperature = (double)(145.0 - 5.0 * maybetemp) / 3.0;

        // Depth in cm: 10-bit field spanning bytes 5-6
        uint16_t depth = ((b[5] & 3) << 8) | b[6];

        /* clang-format off */
        data_t *data = data_make(
                "model",                "Model",        DATA_STRING, "Oil-SonicAdv",
                "id",                   "ID",           DATA_FORMAT, "%08x", DATA_INT, unit_id,
                "flags",                "Flags",        DATA_FORMAT, "%02x", DATA_INT, flags,
                "temperature_C",        "Temperature",  DATA_FORMAT, "%.1f C", DATA_DOUBLE, temperature,
                "depth_cm",             "Depth",        DATA_INT,    depth,
                "mic",                  "Integrity",    DATA_STRING, "CRC",
                NULL);
        /* clang-format on */

        decoder_output_data(decoder, data);
        events++;
    }
    return events;
}

static char const *const output_fields[] = {
        "model",
        "id",
        "flags",
        "temperature_C",
        "depth_cm",
        "mic",
        NULL,
};

r_device const oil_watchman_advanced_me = {
        .name        = "Watchman Sonic Advanced / Plus, Tekelek (Manchester Encoding)",
        .modulation  = FSK_PULSE_PCM,
        .short_width = 1000,
        .long_width  = 1000,
        .reset_limit = 25000,
        .decode_fn   = &oil_watchman_advanced_decode_me,
        .fields      = output_fields,
};
