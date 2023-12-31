/* $NetBSD: es8316ac.c,v 1.6 2023/12/11 13:27:24 mlelstv Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: es8316ac.c,v 1.6 2023/12/11 13:27:24 mlelstv Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/bitops.h>

#include <dev/audio/audio_dai.h>

#include <dev/i2c/i2cvar.h>

#include <dev/fdt/fdtvar.h>

#define	ESCODEC_RESET_REG		0x00
#define	 RESET_ALL			__BITS(5,0)
#define	 RESET_CSM_ON			__BIT(7)
#define	ESCODEC_CLKMAN1_REG		0x01
#define	 CLKMAN1_MCLK_ON		__BIT(6)
#define	 CLKMAN1_BCLK_ON		__BIT(5)
#define	 CLKMAN1_CLK_CP_ON		__BIT(4)
#define	 CLKMAN1_CLK_DAC_ON		__BIT(2)
#define	 CLKMAN1_ANACLK_DAC_ON		__BIT(0)
#define	ESCODEC_ADC_OSR_REG		0x03
#define	ESCODEC_SD_CLK_REG		0x09
#define	 SD_CLK_MSC			__BIT(7)
#define	 SD_CLK_BCLK_INV		__BIT(5)
#define	ESCODEC_SD_ADC_REG		0x0a
#define	ESCODEC_SD_DAC_REG		0x0b
#define	 SD_FMT_LRP			__BIT(5)
#define	 SD_FMT_WL			__BITS(4,2)
#define	  SD_FMT_WL_16			3
#define	 SD_FMT_MASK			__BITS(1,0)
#define	  SD_FMT_I2S			0
#define	ESCODEC_VMID_REG		0x0c
#define	ESCODEC_PDN_REG			0x0d
#define	ESCODEC_HPSEL_REG		0x13
#define	ESCODEC_HPMIXRT_REG		0x14
#define	 HPMIXRT_LD2LHPMIX		__BIT(7)
#define	 HPMIXRT_RD2RHPMIX		__BIT(3)
#define	ESCODEC_HPMIX_REG		0x15
#define	 HPMIX_LHPMIX_MUTE		__BIT(5)
#define	 HPMIX_PDN_LHP_MIX		__BIT(4)
#define	 HPMIX_RHPMIX_MUTE		__BIT(1)
#define	 HPMIX_PDN_RHP_MIX		__BIT(0)
#define	ESCODEC_HPMIXVOL_REG		0x16
#define	 HPMIXVOL_LHPMIXVOL		__BITS(7,4)
#define	 HPMIXVOL_RHPMIXVOL		__BITS(3,0)
#define	ESCODEC_HPOUTEN_REG		0x17
#define	 HPOUTEN_EN_HPL			__BIT(6)
#define	 HPOUTEN_HPL_OUTEN		__BIT(5)
#define	 HPOUTEN_EN_HPR			__BIT(2)
#define	 HPOUTEN_HPR_OUTEN		__BIT(1)
#define	ESCODEC_HPVOL_REG		0x18
#define	 HPVOL_PDN_LICAL		__BIT(7)
#define	 HPVOL_HPLVOL			__BITS(5,4)
#define	 HPVOL_PDN_RICAL		__BIT(3)
#define	 HPVOL_HPRVOL			__BITS(1,0)
#define	ESCODEC_HPPWR_REG		0x19
#define	 HPPWR_PDN_CPHP			__BIT(2)
#define	ESCODEC_CPPWR_REG		0x1a
#define	 CPPWR_PDN_CP			__BIT(5)
#define	ESCODEC_DACPWR_REG		0x2f
#define	 DACPWR_PDN_DAC_L		__BIT(4)
#define	 DACPWR_PDN_DAC_R		__BIT(0)
#define	ESCODEC_DACCTL1_REG		0x30
#define	 DACCTL1_MUTE			__BIT(5)
#define	ESCODEC_DACVOL_L_REG		0x33
#define	 DACVOL_L_DACVOLUME		__BITS(7,0)
#define	ESCODEC_DACVOL_R_REG		0x34
#define	 DACVOL_R_DACVOLUME		__BITS(7,0)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "everest,es8316" },
	DEVICE_COMPAT_EOL
};

struct escodec_softc {
	device_t		sc_dev;
	i2c_tag_t		sc_i2c;
	i2c_addr_t		sc_addr;
	int			sc_phandle;
	struct clk		*sc_clk;

	struct audio_dai_device	sc_dai;
	audio_dai_tag_t		sc_amplifier;

	uint8_t			sc_swvol;
};

static void
escodec_lock(struct escodec_softc *sc)
{
	iic_acquire_bus(sc->sc_i2c, 0);
}

static void
escodec_unlock(struct escodec_softc *sc)
{
	iic_release_bus(sc->sc_i2c, 0);
}

static uint8_t
escodec_read(struct escodec_softc *sc, uint8_t reg)
{
	uint8_t val;

	if (iic_smbus_read_byte(sc->sc_i2c, sc->sc_addr, reg, &val, 0) != 0)
		val = 0xff;

	return val;
}

static void
escodec_write(struct escodec_softc *sc, uint8_t reg, uint8_t val)
{
	(void)iic_smbus_write_byte(sc->sc_i2c, sc->sc_addr, reg, val, 0);
}

enum escodec_mixer_ctrl {
	ESCODEC_OUTPUT_CLASS,
	ESCODEC_INPUT_CLASS,
	ESCODEC_OUTPUT_MASTER,
	ESCODEC_INPUT_DAC,
	ESCODEC_INPUT_DAC_MUTE,
	ESCODEC_INPUT_HEADPHONE,
	ESCODEC_INPUT_MIXEROUT,
	ESCODEC_INPUT_MIXEROUT_MUTE,

	ESCODEC_MIXER_CTRL_LAST
};

enum escodec_mixer_type {
	ESCODEC_MIXER_CLASS,
	ESCODEC_MIXER_SWVOL,
	ESCODEC_MIXER_AMPLIFIER,
	ESCODEC_MIXER_ATTENUATOR,
	ESCODEC_MIXER_MUTE,
};

static const struct escodec_mixer {
	const char *			name;
	int				mixer_class;
	int				prev, next;
	enum escodec_mixer_ctrl		ctrl;
	enum escodec_mixer_type		type;
	u_int				reg[2];
	uint8_t				mask[2];
	uint8_t				maxval;
} escodec_mixers[ESCODEC_MIXER_CTRL_LAST] = {
	/*
	 * Mixer classes
	 */
	[ESCODEC_OUTPUT_CLASS] = {
		.name = AudioCoutputs,
		.type = ESCODEC_MIXER_CLASS,
	},
	[ESCODEC_INPUT_CLASS] = {
		.name = AudioCinputs,
		.type = ESCODEC_MIXER_CLASS,
	},

	/*
	 * Software master volume
	 */
	[ESCODEC_OUTPUT_MASTER] = {
		.name = AudioNmaster,
		.mixer_class = ESCODEC_OUTPUT_CLASS,
		.prev = AUDIO_MIXER_LAST,
		.next = AUDIO_MIXER_LAST,
		.type = ESCODEC_MIXER_SWVOL,
	},

	/*
	 * Stereo DAC
	 */
	[ESCODEC_INPUT_DAC] = {
		.name = AudioNdac,
		.mixer_class = ESCODEC_INPUT_CLASS,
		.prev = AUDIO_MIXER_LAST,
		.next = ESCODEC_INPUT_DAC_MUTE,
		.type = ESCODEC_MIXER_ATTENUATOR,
		.reg = {
			[AUDIO_MIXER_LEVEL_LEFT] = ESCODEC_DACVOL_L_REG,
			[AUDIO_MIXER_LEVEL_RIGHT] = ESCODEC_DACVOL_R_REG,
		},
		.mask = {
			[AUDIO_MIXER_LEVEL_LEFT] = DACVOL_L_DACVOLUME,
			[AUDIO_MIXER_LEVEL_RIGHT] = DACVOL_R_DACVOLUME,
		},
		.maxval = 0xc0,
	},
	[ESCODEC_INPUT_DAC_MUTE] = {
		.name = AudioNmute,
		.mixer_class = ESCODEC_INPUT_CLASS,
		.prev = ESCODEC_INPUT_DAC,
		.next = AUDIO_MIXER_LAST,
		.type = ESCODEC_MIXER_MUTE,
		.reg = {
			[AUDIO_MIXER_LEVEL_MONO] = ESCODEC_DACCTL1_REG,
		},
		.mask = {
			[AUDIO_MIXER_LEVEL_MONO] = DACCTL1_MUTE,
		}
	},

	/*
	 * Charge Pump Headphones
	 */
	[ESCODEC_INPUT_HEADPHONE] = {
		.name = AudioNheadphone,
		.mixer_class = ESCODEC_INPUT_CLASS,
		.prev = AUDIO_MIXER_LAST,
		.next = AUDIO_MIXER_LAST,
		.type = ESCODEC_MIXER_ATTENUATOR,
		.reg = {
			[AUDIO_MIXER_LEVEL_LEFT] = ESCODEC_HPVOL_REG,
			[AUDIO_MIXER_LEVEL_RIGHT] = ESCODEC_HPVOL_REG,
		},
		.mask = {
			[AUDIO_MIXER_LEVEL_LEFT] = HPVOL_HPLVOL,
			[AUDIO_MIXER_LEVEL_RIGHT] = HPVOL_HPRVOL,
		}
	},

	/*
	 * Headphone mixer
	 */
	[ESCODEC_INPUT_MIXEROUT] = {
		.name = AudioNmixerout,
		.mixer_class = ESCODEC_INPUT_CLASS,
		.prev = AUDIO_MIXER_LAST,
		.next = ESCODEC_INPUT_MIXEROUT_MUTE,
		.type = ESCODEC_MIXER_AMPLIFIER,
		.reg = {
			[AUDIO_MIXER_LEVEL_LEFT] = ESCODEC_HPMIXVOL_REG,
			[AUDIO_MIXER_LEVEL_RIGHT] = ESCODEC_HPMIXVOL_REG,
		},
		.mask = {
			[AUDIO_MIXER_LEVEL_LEFT] = HPMIXVOL_LHPMIXVOL,
			[AUDIO_MIXER_LEVEL_RIGHT] = HPMIXVOL_RHPMIXVOL
		},
		/*
		 * Datasheet says this field goes up to 0xb, but values
		 * above 0x4 result in noisy output in practice.
		 */
		.maxval = 0x4,
	},
	[ESCODEC_INPUT_MIXEROUT_MUTE] = {
		.name = AudioNmute,
		.mixer_class = ESCODEC_INPUT_CLASS,
		.prev = ESCODEC_INPUT_MIXEROUT,
		.next = AUDIO_MIXER_LAST,
		.type = ESCODEC_MIXER_MUTE,
		.reg = {
			[AUDIO_MIXER_LEVEL_MONO] = ESCODEC_HPMIX_REG,
		},
		.mask = {
			[AUDIO_MIXER_LEVEL_MONO] = HPMIX_LHPMIX_MUTE | HPMIX_RHPMIX_MUTE,
		}
	},
};

static void
escodec_swvol_codec(audio_filter_arg_t *arg)
{
	struct escodec_softc * const sc = arg->context;
	const aint_t *src;
	int16_t *dst;
	u_int sample_count;
	u_int i;

	src = arg->src;
	dst = arg->dst;
	sample_count = arg->count * arg->srcfmt->channels;
	for (i = 0; i < sample_count; i++) {
		aint2_t v = (aint2_t)(*src++);
		v = v * sc->sc_swvol / 255;
		*dst++ = (aint_t)v;
	}
}

static const struct escodec_mixer *
escodec_get_mixer(u_int index)
{
	if (index >= ESCODEC_MIXER_CTRL_LAST)
		return NULL;

	return &escodec_mixers[index];
}

static int
escodec_set_format(void *priv, int setmode,
    const audio_params_t *play, const audio_params_t *rec,
    audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	struct escodec_softc * const sc = priv;

	pfil->codec = escodec_swvol_codec;
	pfil->context = sc;

	return 0;
}

static int
escodec_set_port(void *priv, mixer_ctrl_t *mc)
{
	struct escodec_softc * const sc = priv;
	const struct escodec_mixer *mix;
	int nvol, shift, ch;
	uint8_t val;

	if ((mix = escodec_get_mixer(mc->dev)) == NULL)
		return ENXIO;

	switch (mix->type) {
	case ESCODEC_MIXER_SWVOL:
		sc->sc_swvol = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
		return 0;

	case ESCODEC_MIXER_AMPLIFIER:
	case ESCODEC_MIXER_ATTENUATOR:
		escodec_lock(sc);
		for (ch = 0; ch < 2; ch++) {
			val = escodec_read(sc, mix->reg[ch]);
			shift = 8 - fls32(__SHIFTOUT_MASK(mix->mask[ch]));
			nvol = mc->un.value.level[ch] >> shift;
			if (mix->type == ESCODEC_MIXER_ATTENUATOR)
				nvol = __SHIFTOUT_MASK(mix->mask[ch]) - nvol;
			if (mix->maxval != 0 && nvol > mix->maxval)
				nvol = mix->maxval;

			val &= ~mix->mask[ch];
			val |= __SHIFTIN(nvol, mix->mask[ch]);
			escodec_write(sc, mix->reg[ch], val);
		}
		escodec_unlock(sc);
		return 0;

	case ESCODEC_MIXER_MUTE:
		if (mc->un.ord < 0 || mc->un.ord > 1)
			return EINVAL;
		escodec_lock(sc);
		val = escodec_read(sc, mix->reg[0]);
		if (mc->un.ord)
			val |= mix->mask[0];
		else
			val &= ~mix->mask[0];
		escodec_write(sc, mix->reg[0], val);
		escodec_unlock(sc);
		return 0;

	default:
		return ENXIO;
	}
}

static int
escodec_get_port(void *priv, mixer_ctrl_t *mc)
{
	struct escodec_softc * const sc = priv;
	const struct escodec_mixer *mix;
	int nvol, shift, ch;
	uint8_t val;

	if ((mix = escodec_get_mixer(mc->dev)) == NULL)
		return ENXIO;

	switch (mix->type) {
	case ESCODEC_MIXER_SWVOL:
		mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = sc->sc_swvol;
		mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = sc->sc_swvol;
		return 0;

	case ESCODEC_MIXER_AMPLIFIER:
	case ESCODEC_MIXER_ATTENUATOR:
		escodec_lock(sc);
		for (ch = 0; ch < 2; ch++) {
			val = escodec_read(sc, mix->reg[ch]);
			shift = 8 - fls32(__SHIFTOUT_MASK(mix->mask[ch]));
			nvol = __SHIFTOUT(val, mix->mask[ch]);
			if (mix->type == ESCODEC_MIXER_ATTENUATOR)
				nvol = __SHIFTOUT_MASK(mix->mask[ch]) - nvol;
			nvol <<= shift;
			mc->un.value.level[ch] = nvol;
		}
		escodec_unlock(sc);
		return 0;

	case ESCODEC_MIXER_MUTE:
		escodec_lock(sc);
		val = escodec_read(sc, mix->reg[0]);
		mc->un.ord = (val & mix->mask[0]) != 0;
		escodec_unlock(sc);
		return 0;

	default:
		return ENXIO;
	}
}

static int
escodec_query_devinfo(void *priv, mixer_devinfo_t *di)
{
	const struct escodec_mixer *mix;

	if ((mix = escodec_get_mixer(di->index)) == NULL)
		return ENXIO;

	strcpy(di->label.name, mix->name);
	di->mixer_class = mix->mixer_class;
	di->next = mix->next;
	di->prev = mix->prev;

	switch (mix->type) {
	case ESCODEC_MIXER_CLASS:
		di->type = AUDIO_MIXER_CLASS;
		return 0;

	case ESCODEC_MIXER_SWVOL:
		di->type = AUDIO_MIXER_VALUE;
		di->un.v.delta = 1;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;

	case ESCODEC_MIXER_AMPLIFIER:
	case ESCODEC_MIXER_ATTENUATOR:
		di->type = AUDIO_MIXER_VALUE;
		di->un.v.delta =
		    256 / (__SHIFTOUT_MASK(mix->mask[0]) + 1);
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;

	case ESCODEC_MIXER_MUTE:
		di->type = AUDIO_MIXER_ENUM;
		di->un.e.num_mem = 2;
		strcpy(di->un.e.member[0].label.name, AudioNoff);
		di->un.e.member[0].ord = 0;
		strcpy(di->un.e.member[1].label.name, AudioNon);
		di->un.e.member[1].ord = 1;
		return 0;

	default:
		return ENXIO;
	}
}

static const struct audio_hw_if escodec_hw_if = {
	.set_format = escodec_set_format,
	.set_port = escodec_set_port,
	.get_port = escodec_get_port,
	.query_devinfo = escodec_query_devinfo,
};

static int
escodec_dai_set_sysclk(audio_dai_tag_t dai, u_int rate, int dir)
{
	struct escodec_softc * const sc = audio_dai_private(dai);
	int error;

	error = clk_set_rate(sc->sc_clk, rate);
	if (error != 0)
		return error;

	return 0;
}

static int
escodec_dai_set_format(audio_dai_tag_t dai, u_int format)
{
	struct escodec_softc * const sc = audio_dai_private(dai);
	uint8_t sd_clk, sd_fmt, val;

	const u_int fmt = __SHIFTOUT(format, AUDIO_DAI_FORMAT_MASK);
	const u_int clk = __SHIFTOUT(format, AUDIO_DAI_CLOCK_MASK);
	const u_int pol = __SHIFTOUT(format, AUDIO_DAI_POLARITY_MASK);

	if (fmt != AUDIO_DAI_FORMAT_I2S)
		return EINVAL;

	if (clk != AUDIO_DAI_CLOCK_CBS_CFS)
		return EINVAL;

	switch (pol) {
	case AUDIO_DAI_POLARITY_NB_NF:
		sd_clk = 0;
		sd_fmt = 0;
		break;
	case AUDIO_DAI_POLARITY_NB_IF:
		sd_clk = 0;
		sd_fmt = SD_FMT_LRP;
		break;
	case AUDIO_DAI_POLARITY_IB_NF:
		sd_clk = SD_CLK_BCLK_INV;
		sd_fmt = 0;
		break;
	case AUDIO_DAI_POLARITY_IB_IF:
		sd_clk = SD_CLK_BCLK_INV;
		sd_fmt = SD_FMT_LRP;
		break;
	}

	escodec_lock(sc);

	val = escodec_read(sc, ESCODEC_SD_CLK_REG);
	val &= ~(SD_CLK_MSC|SD_CLK_BCLK_INV);
	val |= sd_clk;
	escodec_write(sc, ESCODEC_SD_CLK_REG, val);

	val = escodec_read(sc, ESCODEC_SD_ADC_REG);
	val &= ~SD_FMT_MASK;
	val |= __SHIFTIN(SD_FMT_I2S, SD_FMT_MASK);
	val &= ~SD_FMT_LRP;
	val |= sd_fmt;
	escodec_write(sc, ESCODEC_SD_ADC_REG, val);

	val = escodec_read(sc, ESCODEC_SD_DAC_REG);
	val &= ~SD_FMT_MASK;
	val |= __SHIFTIN(SD_FMT_I2S, SD_FMT_MASK);
	val &= ~SD_FMT_LRP;
	val |= sd_fmt;
	escodec_write(sc, ESCODEC_SD_DAC_REG, val);

	val = escodec_read(sc, ESCODEC_CLKMAN1_REG);
	val |= CLKMAN1_MCLK_ON;
	val |= CLKMAN1_BCLK_ON;
	val |= CLKMAN1_CLK_CP_ON;
	val |= CLKMAN1_CLK_DAC_ON;
	val |= CLKMAN1_ANACLK_DAC_ON;
	escodec_write(sc, ESCODEC_CLKMAN1_REG, val);

	escodec_unlock(sc);

	return 0;
}

static int
escodec_dai_add_device(audio_dai_tag_t dai, audio_dai_tag_t aux)
{
	struct escodec_softc * const sc = audio_dai_private(dai);

	if (sc->sc_amplifier != NULL)
		return 0;

	sc->sc_amplifier = aux;

	return 0;
}

static audio_dai_tag_t
escodec_dai_get_tag(device_t dev, const void *data, size_t len)
{
	struct escodec_softc * const sc = device_private(dev);

	if (len != 4)
		return NULL;

	return &sc->sc_dai;
}

static struct fdtbus_dai_controller_func escodec_dai_funcs = {
	.get_tag = escodec_dai_get_tag
};

static void
escodec_init(struct escodec_softc *sc)
{
	uint8_t val;

	escodec_lock(sc);

	escodec_write(sc, ESCODEC_RESET_REG, RESET_ALL);
	delay(5000);
	escodec_write(sc, ESCODEC_RESET_REG, RESET_CSM_ON);
	delay(30000);

	escodec_write(sc, ESCODEC_VMID_REG, 0xff);
	escodec_write(sc, ESCODEC_ADC_OSR_REG, 0x32);

	val = escodec_read(sc, ESCODEC_SD_ADC_REG);
	val &= ~SD_FMT_WL;
	val |= __SHIFTIN(SD_FMT_WL_16, SD_FMT_WL);
	escodec_write(sc, ESCODEC_SD_ADC_REG, val);

	val = escodec_read(sc, ESCODEC_SD_DAC_REG);
	val &= ~SD_FMT_WL;
	val |= __SHIFTIN(SD_FMT_WL_16, SD_FMT_WL);
	escodec_write(sc, ESCODEC_SD_DAC_REG, val);

	/* Power up */
	escodec_write(sc, ESCODEC_PDN_REG, 0);

	/* Route DAC signal to HP mixer */
	val = HPMIXRT_LD2LHPMIX | HPMIXRT_RD2RHPMIX;
	escodec_write(sc, ESCODEC_HPMIXRT_REG, val);

	/* Power up DAC */
	escodec_write(sc, ESCODEC_DACPWR_REG, 0);

	/* Power up HP mixer and unmute */
	escodec_write(sc, ESCODEC_HPMIX_REG, 0);

	/* Power up HP output driver */
	val = escodec_read(sc, ESCODEC_HPPWR_REG);
	val &= ~HPPWR_PDN_CPHP;
	escodec_write(sc, ESCODEC_HPPWR_REG, val);

	/* Power up HP charge pump circuits */
	val = escodec_read(sc, ESCODEC_CPPWR_REG);
	val &= ~CPPWR_PDN_CP;
	escodec_write(sc, ESCODEC_CPPWR_REG, val);

	/* Set LIN1/RIN1 as inputs for HP mixer */
	escodec_write(sc, ESCODEC_HPSEL_REG, 0);

	/* Power up HP output driver calibration */
	val = escodec_read(sc, ESCODEC_HPVOL_REG);
	val &= ~HPVOL_PDN_LICAL;
	val &= ~HPVOL_PDN_RICAL;
	escodec_write(sc, ESCODEC_HPVOL_REG, val);

	/* Set headphone mixer to -6dB */
	escodec_write(sc, ESCODEC_HPMIXVOL_REG, 0x44);

	/* Set charge pump headphone to -48dB */
	escodec_write(sc, ESCODEC_HPVOL_REG, 0x33);

	/* Set DAC to 0dB */
	escodec_write(sc, ESCODEC_DACVOL_L_REG, 0);
	escodec_write(sc, ESCODEC_DACVOL_R_REG, 0);

	/* Enable HP output */
	val = HPOUTEN_EN_HPL | HPOUTEN_EN_HPR |
	    HPOUTEN_HPL_OUTEN | HPOUTEN_HPR_OUTEN;
	escodec_write(sc, ESCODEC_HPOUTEN_REG, val);

	escodec_unlock(sc);
}

static int
escodec_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, compat_data, &match_result))
		return match_result;

	/* This device is direct-config only */

	return 0;
}

static void
escodec_attach(device_t parent, device_t self, void *aux)
{
	struct escodec_softc * const sc = device_private(self);
	struct i2c_attach_args * const ia = aux;
	const int phandle = ia->ia_cookie;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_phandle = ia->ia_cookie;
	sc->sc_swvol = 0xff;

	sc->sc_clk = fdtbus_clock_get(phandle, "mclk");
	if (sc->sc_clk == NULL || clk_enable(sc->sc_clk) != 0) {
		aprint_error(": couldn't enable mclk\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Everest Semi ES8316 Audio CODEC\n");

	escodec_init(sc);

	sc->sc_dai.dai_set_sysclk = escodec_dai_set_sysclk;
	sc->sc_dai.dai_set_format = escodec_dai_set_format;
	sc->sc_dai.dai_add_device = escodec_dai_add_device;
	sc->sc_dai.dai_hw_if = &escodec_hw_if;
	sc->sc_dai.dai_dev = self;
	sc->sc_dai.dai_priv = sc;
	fdtbus_register_dai_controller(self, phandle, &escodec_dai_funcs);
}

CFATTACH_DECL_NEW(es8316ac, sizeof(struct escodec_softc),
    escodec_match, escodec_attach, NULL, NULL);
