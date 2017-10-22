/*
 * libmad - MPEG audio decoder library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: synth.h,v 1.15 2004/01/23 09:41:33 rob Exp $
 */

# ifndef LIBMAD_SYNTH_H
# define LIBMAD_SYNTH_H

# include "fixed.h"
# include "frame.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*pSetDacSampleRateHandler_t)(int rate);
typedef void (*pRenderSampleBlockHandler_t)(short *sample_buff_ch0, short *sample_buff_ch1, int num_samples, unsigned int num_channels);

struct mad_pcm {
  unsigned int samplerate;		/* sampling frequency (Hz) */
  unsigned short channels;		/* number of channels */
  unsigned short length;		/* number of samples per channel */
};

struct mad_synth {
  mad_fixed_t filter[2][2][2][16][8];	/* polyphase filterbank outputs */
  					/* [ch][eo][peo][s][v] */

  unsigned int phase;			/* current processing phase */

  struct mad_pcm pcm;			/* PCM output */
};

/* single channel PCM selector */
enum {
  MAD_PCM_CHANNEL_SINGLE = 0
};

/* dual channel PCM selector */
enum {
  MAD_PCM_CHANNEL_DUAL_1 = 0,
  MAD_PCM_CHANNEL_DUAL_2 = 1
};

/* stereo PCM selector */
enum {
  MAD_PCM_CHANNEL_STEREO_LEFT  = 0,
  MAD_PCM_CHANNEL_STEREO_RIGHT = 1
};

void mad_synth_init(struct mad_synth *);

# define mad_synth_finish(synth)  /* nothing */

void mad_synth_mute(struct mad_synth *);

void mad_synth_frame(struct mad_synth *, struct mad_frame const *);

void render_sample_block_mono(short *short_sample_buff, int no_samples);
// void render_sample_block(short *sample_buff_ch0, short *sample_buff_ch1, int num_samples, unsigned int num_channels);
// void set_dac_sample_rate(int rate);

void register_set_dac_sample_rate(pSetDacSampleRateHandler_t handler);
void register_render_sample_block(pRenderSampleBlockHandler_t handler);

#ifdef __cplusplus
}
#endif

# endif
