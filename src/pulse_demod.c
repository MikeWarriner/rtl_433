<<<<<<< HEAD
/**
 * Pulse demodulation functions
 *
 * Binary demodulators (PWM/PPM/Manchester/...) using a pulse data structure as input
 *
 * Copyright (C) 2015 Tommy Vestermark
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "pulse_demod.h"
#include "bitbuffer.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


int pulse_demod_pcm(const pulse_data_t *pulses, struct protocol_state *device)
{
	int events = 0;
	bitbuffer_t bits = {0};
	const int MAX_ZEROS = device->reset_limit / device->long_limit;
	const int TOLERANCE = device->long_limit / 4;		// Tolerance is ±25% of a bit period

	for(unsigned n = 0; n < pulses->num_pulses; ++n) {
		// Determine number of high bit periods for NRZ coding, where bits may not be separated
		int highs = (pulses->pulse[n] + device->short_limit/2) / device->short_limit;
		// Determine number of bit periods in current pulse/gap length (rounded)
		int periods = (pulses->pulse[n] + pulses->gap[n] + device->long_limit/2) / device->long_limit;

		// Add run of ones (1 for RZ, many for NRZ)
		for (int i=0; i < highs; ++i) {
			bitbuffer_add_bit(&bits, 1);
		}
		// Add run of zeros
		periods -= highs;					// Remove 1s from whole period
		periods = min(periods, MAX_ZEROS); 	// Dont overflow at end of message
		for (int i=0; i < periods; ++i) {
			bitbuffer_add_bit(&bits, 0);
		}

		// Validate data
		if ((device->short_limit != device->long_limit) 		// Only for RZ coding
		 && (fabsf(pulses->pulse[n] - device->short_limit) > TOLERANCE)		// Pulse must be within tolerance
		) {
			// Data is corrupt
			if (debug_output > 3) {
			        fprintf(stderr,"bitbuffer cleared at %d: pulse %d, gap %d, period %d\n",
					n,pulses->pulse[n],pulses->gap[n],
					pulses->pulse[n] + pulses->gap[n]);
			}
			bitbuffer_clear(&bits);
		}

		// End of Message?
		if (((n == pulses->num_pulses-1) 	// No more pulses? (FSK)
		 || (pulses->gap[n] > device->reset_limit))	// Loong silence (OOK)
		 && (bits.bits_per_row[0] > 0)		// Only if data has been accumulated
		) {
			if (device->callback) {
				events += device->callback(&bits);
			}
			// Debug printout
			if(!device->callback || (debug_output && events > 0)) {
				fprintf(stderr, "pulse_demod_pcm(): %s \n", device->name);
				bitbuffer_print(&bits);
			}
			bitbuffer_clear(&bits);
		}
	} // for
	return events;
}


int pulse_demod_ppm(const pulse_data_t *pulses, struct protocol_state *device) {
	int events = 0;
	bitbuffer_t bits = {0};

	for(unsigned n = 0; n < pulses->num_pulses; ++n) {
		// Short gap
		if(pulses->gap[n] < device->short_limit) {
			bitbuffer_add_bit(&bits, 0);
		// Long gap
		} else if(pulses->gap[n] < device->long_limit) {
			bitbuffer_add_bit(&bits, 1);
		// Check for new packet in multipacket
		} else if(pulses->gap[n] < device->reset_limit) {
			bitbuffer_add_row(&bits);
		// End of Message?
		} else {
			if (device->callback) {
				events += device->callback(&bits);
			}
			// Debug printout
			if(!device->callback || (debug_output && events > 0)) {
				fprintf(stderr, "pulse_demod_ppm(): %s \n", device->name);
				bitbuffer_print(&bits);
			}
			bitbuffer_clear(&bits);
		}
	} // for pulses
	return events;
}


int pulse_demod_pwm(const pulse_data_t *pulses, struct protocol_state *device) {
	int events = 0;
	int start_bit_detected = 0;
	bitbuffer_t bits = {0};
	int start_bit = device->demod_arg;

	for(unsigned n = 0; n < pulses->num_pulses; ++n) {
		// Should we disregard startbit?
		if(start_bit == 1 && start_bit_detected == 0) {
			start_bit_detected = 1;
		} else {
			// Detect pulse width
			if(pulses->pulse[n] <= device->short_limit) {
				bitbuffer_add_bit(&bits, 1);
			} else {
				bitbuffer_add_bit(&bits, 0);
			}
		}
		// End of Message?
                if (n == pulses->num_pulses - 1                           // No more pulses (FSK)
		    || pulses->gap[n] > device->reset_limit) {  // Long silence (OOK)
			if (device->callback) {
				events += device->callback(&bits);
			}
			// Debug printout
			if(!device->callback || (debug_output && events > 0)) {
				fprintf(stderr, "pulse_demod_pwm(): %s\n", device->name);
				bitbuffer_print(&bits);
			}
			bitbuffer_clear(&bits);
			start_bit_detected = 0;
		// Check for new packet in multipacket
		} else if(pulses->gap[n] > device->long_limit) {
			bitbuffer_add_row(&bits);
			start_bit_detected = 0;
		}
	}
	return events;
}


int pulse_demod_pwm_precise(const pulse_data_t *pulses, struct protocol_state *device)
{
	int events = 0;
	bitbuffer_t bits = {0};
	PWM_Precise_Parameters *p = (PWM_Precise_Parameters *)device->demod_arg;

	for(unsigned n = 0; n < pulses->num_pulses; ++n) {
		// 'Short' 1 pulse
		if (fabsf(pulses->pulse[n] - device->short_limit) < p->pulse_tolerance) {
			bitbuffer_add_bit(&bits, 1);
		// 'Long' 0 pulse
		} else if (fabsf(pulses->pulse[n] - device->long_limit) < p->pulse_tolerance) {
			bitbuffer_add_bit(&bits, 0);
		// Sync pulse
		} else if (p->pulse_sync_width && (abs(pulses->pulse[n] - p->pulse_sync_width) < p->pulse_tolerance)) {
			bitbuffer_add_row(&bits);
		// Ignore spurious short pulses
		} else if (pulses->pulse[n] < (device->short_limit - p->pulse_tolerance)) {
			// Do nothing
		} else {
			return 0;	// Pulse outside specified timing
		}

		// End of Message?
		if(pulses->gap[n] > device->reset_limit) {
			if (device->callback) {
				events += device->callback(&bits);
			}
			// Debug printout
			if(!device->callback || (debug_output && events > 0)) {
				fprintf(stderr, "pulse_demod_pwm_precise(): %s \n", device->name);
				bitbuffer_print(&bits);
			}
			bitbuffer_clear(&bits);
		}
	} // for
	return events;
}


int pulse_demod_pwm_ternary(const pulse_data_t *pulses, struct protocol_state *device)
{
	int events = 0;
	bitbuffer_t bits = {0};
	unsigned sync_bit = device->demod_arg;

	for(unsigned n = 0; n < pulses->num_pulses; ++n) {
		// Short pulse
		if (pulses->pulse[n] < device->short_limit) {
			if (sync_bit == 0) {
				bitbuffer_add_row(&bits);
			} else {
				bitbuffer_add_bit(&bits, 0);
			}
		// Middle pulse
		} else if (pulses->pulse[n] < device->long_limit) {
			if (sync_bit == 0) {
				bitbuffer_add_bit(&bits, 0);
			} else if (sync_bit == 1) {
				bitbuffer_add_row(&bits);
			} else {
				bitbuffer_add_bit(&bits, 1);
			}
		// Long pulse
		} else {
			if (sync_bit == 2) {
				bitbuffer_add_row(&bits);
			} else {
				bitbuffer_add_bit(&bits, 1);
			}
		}

		// End of Message?
		if(pulses->gap[n] > device->reset_limit) {
			if (device->callback) {
				events += device->callback(&bits);
			}
			// Debug printout
			if(!device->callback || (debug_output && events > 0)) {
				fprintf(stderr, "pulse_demod_pwm_ternary(): %s \n", device->name);
				bitbuffer_print(&bits);
			}
			bitbuffer_clear(&bits);
		}
	} // for
	return events;
}


int pulse_demod_manchester_zerobit(const pulse_data_t *pulses, struct protocol_state *device) {
	int events = 0;
	int time_since_last = 0;
	bitbuffer_t bits = {0};

	// First rising edge is allways counted as a zero (Seems to be hardcoded policy for the Oregon Scientific sensors...)
	bitbuffer_add_bit(&bits, 0);

	for(unsigned n = 0; n < pulses->num_pulses; ++n) {
		// Falling edge is on end of pulse
		if(pulses->pulse[n] + time_since_last > (device->short_limit * 1.5)) {
			// Last bit was recorded more than short_limit*1.5 samples ago
			// so this pulse start must be a data edge (falling data edge means bit = 1)
			bitbuffer_add_bit(&bits, 1);
			time_since_last = 0;
		} else {
			time_since_last += pulses->pulse[n];
		}

		// End of Message?
		if(pulses->gap[n] > device->reset_limit) {
			int newevents = 0;
			if (device->callback) {
				events += device->callback(&bits);
			}
			// Debug printout
			if(!device->callback || (debug_output && events > 0)) {
				fprintf(stderr, "pulse_demod_manchester_zerobit(): %s \n", device->name);
				bitbuffer_print(&bits);
			}
			bitbuffer_clear(&bits);
			bitbuffer_add_bit(&bits, 0);		// Prepare for new message with hardcoded 0
			time_since_last = 0;
		// Rising edge is on end of gap
		} else if(pulses->gap[n] + time_since_last > (device->short_limit * 1.5)) {
			// Last bit was recorded more than short_limit*1.5 samples ago
			// so this pulse end is a data edge (rising data edge means bit = 0)
			bitbuffer_add_bit(&bits, 0);
			time_since_last = 0;
		} else {
			time_since_last += pulses->gap[n];
		}
	}
	return events;
}

int pulse_demod_clock_bits(const pulse_data_t *pulses, struct protocol_state *device) {
   int symbol[PD_MAX_PULSES * 2];
   unsigned int n;

   PWM_Precise_Parameters *p = (PWM_Precise_Parameters *)device->demod_arg;
   bitbuffer_t bits = {0};
   int events = 0;

   for(n = 0; n < pulses->num_pulses; n++) {
      symbol[n*2] = pulses->pulse[n];
      symbol[(n*2)+1] = pulses->gap[n];
   }

   for(n = 0; n < pulses->num_pulses * 2; ++n) {
      if ( fabsf(symbol[n] - device->short_limit) < p->pulse_tolerance) {
         // Short - 1
         bitbuffer_add_bit(&bits, 1);
         if ( fabsf(symbol[++n] - device->short_limit) > p->pulse_tolerance) {
            if (symbol[n] >= device->reset_limit - p->pulse_tolerance ) {
               // Don't expect another short gap at end of message
               n--;
            } else {
/*
               fprintf(stderr, "Detected error during pulse_demod_clock_bits(): %s\n",
                       device->name);
*/
               return events;
            }
         }
      } else if ( fabsf(symbol[n] - device->long_limit) < p->pulse_tolerance) {
         // Long - 0
         bitbuffer_add_bit(&bits, 0);
      } else if (symbol[n] >= device->reset_limit - p->pulse_tolerance ) {
         //END message ?
         if (device->callback) {
            events += device->callback(&bits);
         }
         if(!device->callback || (debug_output && events > 0)) {
            fprintf(stderr, "pulse_demod_clock_bits(): %s \n", device->name);
            bitbuffer_print(&bits);
         }
         bitbuffer_clear(&bits);
      }
   }

   return events;
}

/*
 * Oregon Scientific V1 Protocol
 * Starts with a clean preamble of 12 pulses with
 * consistent timing followed by an out of time Sync pulse.
 * Data then follows with manchester encoding, but
 * care must be taken with the gap after the sync pulse since it
 * is outside of the normal clocking.  Because of this a data stream
 * beginning with a 0 will have data in this gap.
 * This code looks at pulse and gap width and clocks bits
 * in from this.  Since this is manchester encoded every other
 * bit is discarded.
 */

int pulse_demod_osv1(const pulse_data_t *pulses, struct protocol_state *device) {
	unsigned int n;
	int preamble = 0;
	int events = 0;
	int manbit = 0;
	bitbuffer_t bits = {0};

	/* preamble */
	for(n = 0; n < pulses->num_pulses; ++n) {
		if(pulses->pulse[n] >= 350 && pulses->gap[n] >= 200) {
			preamble++;
			if(pulses->gap[n] >= 400)
				break;
		} else
			return(events);
	}
	if(preamble != 12) {
		if(debug_output) fprintf(stderr, "preamble %d  %d %d\n", preamble, pulses->pulse[0], pulses->gap[0]);
		return(events);
	}

	/* sync */
	++n;
	if(pulses->pulse[n] < 1000 || pulses->gap[n] < 1000) {
		return(events);
	}

	/* data bits - manchester encoding */

	/* sync gap could be part of data when the first bit is 0 */
	if(pulses->gap[n] > pulses->pulse[n]) {
		manbit ^= 1;
		if(manbit) bitbuffer_add_bit(&bits, 0);
	}

	/* remaining data bits */
	for(n++; n < pulses->num_pulses; ++n) {
		manbit ^= 1;
		if(manbit) bitbuffer_add_bit(&bits, 1);
		if(pulses->pulse[n] > 615) {
			manbit ^= 1;
			if(manbit) bitbuffer_add_bit(&bits, 1);
		}
		if (n == pulses->num_pulses - 1 || pulses->gap[n] > device->reset_limit) {
			if((bits.bits_per_row[bits.num_rows-1] == 32) && device->callback) {
				events += device->callback(&bits);
			}
			return(events);
		}
		manbit ^= 1;
		if(manbit) bitbuffer_add_bit(&bits, 0);
		if(pulses->gap[n] > 450) {
			manbit ^= 1;
			if(manbit) bitbuffer_add_bit(&bits, 0);
		}
	}
	return events;
}
=======
/** @file
    Pulse demodulation functions.

    Binary demodulators (PWM/PPM/Manchester/...) using a pulse data structure as input.

    Copyright (C) 2015 Tommy Vestermark

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "pulse_demod.h"
#include "bitbuffer.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <limits.h>

static int account_event(r_device *device, int ret)
{
    // statistics accounting
    device->decode_events += 1;
    if (ret > 0) {
        device->decode_ok += 1;
        device->decode_messages += ret;
    }
    else {
        device->decode_fails[-ret] += 1;
        ret = 0;
    }
    return ret;
}

int pulse_demod_pcm(const pulse_data_t *pulses, r_device *device)
{
    int events = 0;
    bitbuffer_t bits = {0};
    const int max_zeros = device->s_reset_limit / device->s_long_width;
    const int tolerance = device->s_long_width / 4; // Tolerance is ±25% of a bit period

    for (unsigned n = 0; n < pulses->num_pulses; ++n) {
        // Determine number of high bit periods for NRZ coding, where bits may not be separated
        int highs = (pulses->pulse[n]) * device->f_short_width + 0.5;
        // Determine number of bit periods in current pulse/gap length (rounded)
        int periods = (pulses->pulse[n] + pulses->gap[n]) * device->f_long_width + 0.5;

        // Add run of ones (1 for RZ, many for NRZ)
        for (int i = 0; i < highs; ++i) {
            bitbuffer_add_bit(&bits, 1);
        }
        // Add run of zeros
        periods -= highs;                  // Remove 1s from whole period
        periods = MIN(periods, max_zeros); // Don't overflow at end of message
        for (int i = 0; i < periods; ++i) {
            bitbuffer_add_bit(&bits, 0);
        }

        // Validate data
        if ((device->s_short_width != device->s_long_width)                    // Only for RZ coding
                && (abs(pulses->pulse[n] - device->s_short_width) > tolerance) // Pulse must be within tolerance
        ) {
            // Data is corrupt
            if (device->verbose > 3) {
                fprintf(stderr, "bitbuffer cleared at %d: pulse %d, gap %d, period %d\n",
                        n, pulses->pulse[n], pulses->gap[n],
                        pulses->pulse[n] + pulses->gap[n]);
            }
            bitbuffer_clear(&bits);
        }

        // End of Message?
        if (((n == pulses->num_pulses - 1)                       // No more pulses? (FSK)
                    || (pulses->gap[n] > device->s_reset_limit)) // Long silence (OOK)
                && (bits.bits_per_row[0] > 0)                    // Only if data has been accumulated
        ) {
            if (device->decode_fn) {
                events += account_event(device, device->decode_fn(device, &bits));
            }
            // Debug printout
            if (!device->decode_fn || (device->verbose && events > 0)) {
                fprintf(stderr, "pulse_demod_pcm(): %s \n", device->name);
                bitbuffer_print(&bits);
            }
            bitbuffer_clear(&bits);
        }
    } // for
    return events;
}

int pulse_demod_ppm(const pulse_data_t *pulses, r_device *device)
{
    int events = 0;
    bitbuffer_t bits = {0};

    // lower and upper bounds (non inclusive)
    int zero_l, zero_u;
    int one_l, one_u;
    int sync_l = 0, sync_u = 0;

    if (device->s_tolerance > 0) {
        // precise
        zero_l = device->s_short_width - device->s_tolerance;
        zero_u = device->s_short_width + device->s_tolerance;
        one_l  = device->s_long_width - device->s_tolerance;
        one_u  = device->s_long_width + device->s_tolerance;
        if (device->s_sync_width > 0) {
            sync_l = device->s_sync_width - device->s_tolerance;
            sync_u = device->s_sync_width + device->s_tolerance;
        }
    }
    else {
        // no sync, short=0, long=1
        zero_l = 0;
        zero_u = (device->s_short_width + device->s_long_width) / 2 + 1;
        one_l  = zero_u - 1;
        one_u  = device->s_gap_limit ? device->s_gap_limit : device->s_reset_limit;
    }

    for (unsigned n = 0; n < pulses->num_pulses; ++n) {
        if (pulses->gap[n] > zero_l && pulses->gap[n] < zero_u) {
            // Short gap
            bitbuffer_add_bit(&bits, 0);
        }
        else if (pulses->gap[n] > one_l && pulses->gap[n] < one_u) {
            // Long gap
            bitbuffer_add_bit(&bits, 1);
        }
        else if (pulses->gap[n] > sync_l && pulses->gap[n] < sync_u) {
            // Sync gap
            bitbuffer_add_sync(&bits);
        }

        // Check for new packet in multipacket
        else if (pulses->gap[n] < device->s_reset_limit) {
            bitbuffer_add_row(&bits);
        }
        // End of Message?
        if (((n == pulses->num_pulses - 1)                            // No more pulses? (FSK)
                    || (pulses->gap[n] >= device->s_reset_limit))     // Long silence (OOK)
                && (bits.bits_per_row[0] > 0 || bits.num_rows > 1)) { // Only if data has been accumulated

            if (device->decode_fn) {
                events += account_event(device, device->decode_fn(device, &bits));
            }
            // Debug printout
            if (!device->decode_fn || (device->verbose && events > 0)) {
                fprintf(stderr, "pulse_demod_ppm(): %s \n", device->name);
                bitbuffer_print(&bits);
            }
            bitbuffer_clear(&bits);
        }
    } // for pulses
    return events;
}

int pulse_demod_pwm(const pulse_data_t *pulses, r_device *device)
{
    int events = 0;
    bitbuffer_t bits = {0};

    // lower and upper bounds (non inclusive)
    int one_l, one_u;
    int zero_l, zero_u;
    int sync_l = 0, sync_u = 0;

    if (device->s_tolerance > 0) {
        // precise
        one_l  = device->s_short_width - device->s_tolerance;
        one_u  = device->s_short_width + device->s_tolerance;
        zero_l = device->s_long_width - device->s_tolerance;
        zero_u = device->s_long_width + device->s_tolerance;
        if (device->s_sync_width > 0) {
            sync_l = device->s_sync_width - device->s_tolerance;
            sync_u = device->s_sync_width + device->s_tolerance;
        }
    }
    else if (device->s_sync_width <= 0) {
        // no sync, short=1, long=0
        one_l  = 0;
        one_u  = (device->s_short_width + device->s_long_width) / 2 + 1;
        zero_l = one_u - 1;
        zero_u = INT_MAX;
    }
    else if (device->s_sync_width < device->s_short_width) {
        // short=sync, middle=1, long=0
        sync_l = 0;
        sync_u = (device->s_sync_width + device->s_short_width) / 2 + 1;
        one_l  = sync_u - 1;
        one_u  = (device->s_short_width + device->s_long_width) / 2 + 1;
        zero_l = one_u - 1;
        zero_u = INT_MAX;
    }
    else if (device->s_sync_width < device->s_long_width) {
        // short=1, middle=sync, long=0
        one_l  = 0;
        one_u  = (device->s_short_width + device->s_sync_width) / 2 + 1;
        sync_l = one_u - 1;
        sync_u = (device->s_sync_width + device->s_long_width) / 2 + 1;
        zero_l = sync_u - 1;
        zero_u = INT_MAX;
    }
    else {
        // short=1, middle=0, long=sync
        one_l  = 0;
        one_u  = (device->s_short_width + device->s_long_width) / 2 + 1;
        zero_l = one_u - 1;
        zero_u = (device->s_long_width + device->s_sync_width) / 2 + 1;
        sync_l = zero_u - 1;
        sync_u = INT_MAX;
    }

    for (unsigned n = 0; n < pulses->num_pulses; ++n) {
        if (pulses->pulse[n] > one_l && pulses->pulse[n] < one_u) {
            // 'Short' 1 pulse
            bitbuffer_add_bit(&bits, 1);
        }
        else if (pulses->pulse[n] > zero_l && pulses->pulse[n] < zero_u) {
            // 'Long' 0 pulse
            bitbuffer_add_bit(&bits, 0);
        }
        else if (pulses->pulse[n] > sync_l && pulses->pulse[n] < sync_u) {
            // Sync pulse
            bitbuffer_add_sync(&bits);
        }
        else if (pulses->pulse[n] <= one_l) {
            // Ignore spurious short pulses
        }
        else {
            // Pulse outside specified timing
            return 0;
        }

        // End of Message?
        if (((n == pulses->num_pulses - 1)                       // No more pulses? (FSK)
                    || (pulses->gap[n] > device->s_reset_limit)) // Long silence (OOK)
                && (bits.num_rows > 0)) {                        // Only if data has been accumulated
            if (device->decode_fn) {
                events += account_event(device, device->decode_fn(device, &bits));
            }
            // Debug printout
            if (!device->decode_fn || (device->verbose && events > 0)) {
                fprintf(stderr, "pulse_demod_pwm(): %s \n", device->name);
                bitbuffer_print(&bits);
            }
            bitbuffer_clear(&bits);
        }
        else if (device->s_gap_limit > 0 && pulses->gap[n] > device->s_gap_limit
                && bits.num_rows > 0 && bits.bits_per_row[bits.num_rows - 1] > 0) {
            // New packet in multipacket
            bitbuffer_add_row(&bits);
        }
    }
    return events;
}

int pulse_demod_manchester_zerobit(const pulse_data_t *pulses, r_device *device)
{
    int events = 0;
    int time_since_last = 0;
    bitbuffer_t bits = {0};

    // First rising edge is always counted as a zero (Seems to be hardcoded policy for the Oregon Scientific sensors...)
    bitbuffer_add_bit(&bits, 0);

    for (unsigned n = 0; n < pulses->num_pulses; ++n) {
        // Falling edge is on end of pulse
        if (device->s_tolerance > 0
                && (pulses->pulse[n] < device->s_short_width - device->s_tolerance
                || pulses->pulse[n] > device->s_short_width * 2 + device->s_tolerance
                || pulses->gap[n] < device->s_short_width - device->s_tolerance
                || pulses->gap[n] > device->s_short_width * 2 + device->s_tolerance)) {
            // The pulse or gap is too long or too short, thus invalid
            bitbuffer_add_row(&bits);
            bitbuffer_add_bit(&bits, 0); // Prepare for new message with hardcoded 0
            time_since_last = 0;
        }
        else if (pulses->pulse[n] + time_since_last > (device->s_short_width * 1.5)) {
            // Last bit was recorded more than short_width*1.5 samples ago
            // so this pulse start must be a data edge (falling data edge means bit = 1)
            bitbuffer_add_bit(&bits, 1);
            time_since_last = 0;
        }
        else {
            time_since_last += pulses->pulse[n];
        }

        // End of Message?
        if (((n == pulses->num_pulses - 1)                       // No more pulses? (FSK)
                    || (pulses->gap[n] > device->s_reset_limit)) // Long silence (OOK)
                && (bits.num_rows > 0)) {                        // Only if data has been accumulated
            int newevents = 0;
            if (device->decode_fn) {
                events += account_event(device, device->decode_fn(device, &bits));
            }
            // Debug printout
            if (!device->decode_fn || (device->verbose && events > 0)) {
                fprintf(stderr, "pulse_demod_manchester_zerobit(): %s \n", device->name);
                bitbuffer_print(&bits);
            }
            bitbuffer_clear(&bits);
            bitbuffer_add_bit(&bits, 0); // Prepare for new message with hardcoded 0
            time_since_last = 0;
        }
        // Rising edge is on end of gap
        else if (pulses->gap[n] + time_since_last > (device->s_short_width * 1.5)) {
            // Last bit was recorded more than short_width*1.5 samples ago
            // so this pulse end is a data edge (rising data edge means bit = 0)
            bitbuffer_add_bit(&bits, 0);
            time_since_last = 0;
        }
        else {
            time_since_last += pulses->gap[n];
        }
    }
    return events;
}

int pulse_demod_dmc(const pulse_data_t *pulses, r_device *device)
{
    int symbol[PD_MAX_PULSES * 2 + 1] = {0};
    unsigned int n;

    bitbuffer_t bits = {0};
    int events = 0;

    for (n = 0; n < pulses->num_pulses; n++) {
        symbol[n * 2] = pulses->pulse[n];
        symbol[n * 2 + 1] = pulses->gap[n];
    }

    for (n = 0; n < pulses->num_pulses * 2; ++n) {
        if (abs(symbol[n] - device->s_short_width) < device->s_tolerance) {
            // Short - 1
            bitbuffer_add_bit(&bits, 1);
            if (abs(symbol[++n] - device->s_short_width) > device->s_tolerance) {
                if (symbol[n] >= device->s_reset_limit - device->s_tolerance) {
                    // Don't expect another short gap at end of message
                    n--;
                }
                else if (bits.num_rows > 0 && bits.bits_per_row[bits.num_rows - 1] > 0) {
                    bitbuffer_add_row(&bits);
/*
                    fprintf(stderr, "Detected error during pulse_demod_dmc(): %s\n",
                            device->name);
*/
                }
            }
        }
        else if (abs(symbol[n] - device->s_long_width) < device->s_tolerance) {
            // Long - 0
            bitbuffer_add_bit(&bits, 0);
        }
        else if (symbol[n] >= device->s_reset_limit - device->s_tolerance
                && bits.num_rows > 0) { // Only if data has been accumulated
            //END message ?
            if (device->decode_fn) {
                events += account_event(device, device->decode_fn(device, &bits));
            }
            if (!device->decode_fn || (device->verbose && events > 0)) {
                fprintf(stderr, "pulse_demod_dmc(): %s \n", device->name);
                bitbuffer_print(&bits);
            }
            bitbuffer_clear(&bits);
        }
    }

    return events;
}

int pulse_demod_piwm_raw(const pulse_data_t *pulses, r_device *device)
{
    int symbol[PD_MAX_PULSES * 2];
    unsigned int n;
    int w;

    bitbuffer_t bits = {0};
    int events = 0;

    for (n = 0; n < pulses->num_pulses; n++) {
        symbol[n * 2] = pulses->pulse[n];
        symbol[n * 2 + 1] = pulses->gap[n];
    }

    for (n = 0; n < pulses->num_pulses * 2; ++n) {
        w = symbol[n] * device->f_short_width + 0.5;
        if (symbol[n] > device->s_long_width) {
            bitbuffer_add_row(&bits);
        }
        else if (abs(symbol[n] - w * device->s_short_width) < device->s_tolerance) {
            // Add w symbols
            for (; w > 0; --w)
                bitbuffer_add_bit(&bits, 1 - n % 2);
        }
        else if (symbol[n] < device->s_reset_limit
                && bits.num_rows > 0
                && bits.bits_per_row[bits.num_rows - 1] > 0) {
            bitbuffer_add_row(&bits);
/*
            fprintf(stderr, "Detected error during pulse_demod_piwm_raw(): %s\n",
                    device->name);
*/
        }

        if (((n == pulses->num_pulses * 2 - 1)              // No more pulses? (FSK)
                    || (symbol[n] > device->s_reset_limit)) // Long silence (OOK)
                && (bits.num_rows > 0)) {                   // Only if data has been accumulated
            //END message ?
            if (device->decode_fn) {
                events += account_event(device, device->decode_fn(device, &bits));
            }
            if (!device->decode_fn || (device->verbose && events > 0)) {
                fprintf(stderr, "pulse_demod_piwm_raw(): %s \n", device->name);
                bitbuffer_print(&bits);
            }
            bitbuffer_clear(&bits);
        }
    }

    return events;
}

int pulse_demod_piwm_dc(const pulse_data_t *pulses, r_device *device)
{
    int symbol[PD_MAX_PULSES * 2];
    unsigned int n;

    bitbuffer_t bits = {0};
    int events = 0;

    for (n = 0; n < pulses->num_pulses; n++) {
        symbol[n * 2] = pulses->pulse[n];
        symbol[n * 2 + 1] = pulses->gap[n];
    }

    for (n = 0; n < pulses->num_pulses * 2; ++n) {
        if (abs(symbol[n] - device->s_short_width) < device->s_tolerance) {
            // Short - 1
            bitbuffer_add_bit(&bits, 1);
        }
        else if (abs(symbol[n] - device->s_long_width) < device->s_tolerance) {
            // Long - 0
            bitbuffer_add_bit(&bits, 0);
        }
        else if (symbol[n] < device->s_reset_limit
                && bits.num_rows > 0
                && bits.bits_per_row[bits.num_rows - 1] > 0) {
            bitbuffer_add_row(&bits);
/*
            fprintf(stderr, "Detected error during pulse_demod_piwm_dc(): %s\n",
                    device->name);
*/
        }

        if (((n == pulses->num_pulses * 2 - 1)              // No more pulses? (FSK)
                    || (symbol[n] > device->s_reset_limit)) // Long silence (OOK)
                && (bits.num_rows > 0)) {                   // Only if data has been accumulated
            //END message ?
            if (device->decode_fn) {
                events += account_event(device, device->decode_fn(device, &bits));
            }
            if (!device->decode_fn || (device->verbose && events > 0)) {
                fprintf(stderr, "pulse_demod_piwm_dc(): %s \n", device->name);
                bitbuffer_print(&bits);
            }
            bitbuffer_clear(&bits);
        }
    }

    return events;
}

/*
 * Oregon Scientific V1 Protocol
 * Starts with a clean preamble of 12 pulses with
 * consistent timing followed by an out of time Sync pulse.
 * Data then follows with manchester encoding, but
 * care must be taken with the gap after the sync pulse since it
 * is outside of the normal clocking.  Because of this a data stream
 * beginning with a 0 will have data in this gap.
 * This code looks at pulse and gap width and clocks bits
 * in from this.  Since this is manchester encoded every other
 * bit is discarded.
 */

int pulse_demod_osv1(const pulse_data_t *pulses, r_device *device)
{
    unsigned int n;
    int preamble = 0;
    int events = 0;
    int manbit = 0;
    bitbuffer_t bits = {0};
    int halfbit_min = device->s_short_width / 2;
    int halfbit_max = device->s_short_width * 3 / 2;
    int sync_min = 2 * halfbit_max;

    /* preamble */
    for (n = 0; n < pulses->num_pulses; ++n) {
        if (pulses->pulse[n] > halfbit_min && pulses->gap[n] > halfbit_min) {
            preamble++;
            if (pulses->gap[n] > halfbit_max)
                break;
        }
        else
            return events;
    }
    if (preamble != 12) {
        if (device->verbose)
            fprintf(stderr, "preamble %d  %d %d\n", preamble, pulses->pulse[0], pulses->gap[0]);
        return events;
    }

    /* sync */
    ++n;
    if (pulses->pulse[n] < sync_min || pulses->gap[n] < sync_min) {
        return events;
    }

    /* data bits - manchester encoding */

    /* sync gap could be part of data when the first bit is 0 */
    if (pulses->gap[n] > pulses->pulse[n]) {
        manbit ^= 1;
        if (manbit)
            bitbuffer_add_bit(&bits, 0);
    }

    /* remaining data bits */
    for (n++; n < pulses->num_pulses; ++n) {
        manbit ^= 1;
        if (manbit)
            bitbuffer_add_bit(&bits, 1);
        if (pulses->pulse[n] > halfbit_max) {
            manbit ^= 1;
            if (manbit)
                bitbuffer_add_bit(&bits, 1);
        }
        if ((n == pulses->num_pulses - 1
                    || pulses->gap[n] > device->s_reset_limit)
                && (bits.num_rows > 0)) { // Only if data has been accumulated
            //END message ?
            if (device->decode_fn) {
                events += account_event(device, device->decode_fn(device, &bits));
            }
            return events;
        }
        manbit ^= 1;
        if (manbit)
            bitbuffer_add_bit(&bits, 0);
        if (pulses->gap[n] > halfbit_max) {
            manbit ^= 1;
            if (manbit)
                bitbuffer_add_bit(&bits, 0);
        }
    }
    return events;
}

int pulse_demod_string(const char *code, r_device *device)
{
    int events = 0;
    bitbuffer_t bits = {0};

    bitbuffer_parse(&bits, code);

    if (device->decode_fn) {
        events += account_event(device, device->decode_fn(device, &bits));
    }
    // Debug printout
    if (!device->decode_fn || (device->verbose && events > 0)) {
        fprintf(stderr, "pulse_demod_pcm(): %s \n", device->name);
        bitbuffer_print(&bits);
    }

    return events;
}
>>>>>>> e5a6083a0a1677f4b0a435602fc623fbbb54ecc3
