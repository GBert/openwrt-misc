#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "934x.h"
#include "mmio.h"
#include "utils.h"

#define REF_CLK	25*1000*1000
#define ATH_PLL_MAX 0x80
#define ATH_STEREO_MAX 0x20

int main(int argc, char **argv) {
    uint32_t audio_pll_config, audio_pll_modulation, audio_pll_modulation_step;
    uint32_t audio_pll_ext_div, audio_pll_postplldiv, audio_pll_refdiv;
    uint32_t audio_pll_tgt_div_frac, audio_pll_tgt_div_int, audio_pll_mod_start;
    uint32_t audio_pll_mod_step_frac, audio_pll_mod_step_int, audio_pll_mod_step_update_cnt;
    uint32_t div;
    uint32_t stereo_config, stereo_posedge;
    float pll_freq, mclk_freq, tgt_div, pll_freq_multi, pll_postplldiv, vcofreq;
    struct mmio io;

    if (mmio_map(&io, ATH_PLL_CONFIG, ATH_PLL_MAX))
        die_errno("mmio_map() failed for ATH_PLL_CONFIG");

    audio_pll_config = mmio_readl(&io, 0x30);
    audio_pll_modulation = mmio_readl(&io, 0x34);
    audio_pll_modulation_step = mmio_readl(&io, 0x38);

    mmio_unmap(&io);

    if (mmio_map(&io, ATH_STEREO_CONFIG, ATH_STEREO_MAX))
        die_errno("mmio_map() failed for ATH_STEREO_CONFIG");

    stereo_config = mmio_readl(&io, 0x00);

    mmio_unmap(&io);


    if (AUDIO_PLL_CONFIG_BYPASS_GET(audio_pll_config)) {
        printf("bypassing Audio PLL\n");
        printf("reference frequency : %3.03f MHz\n", REF_CLK/1E6);
    } else {
        audio_pll_ext_div=     AUDIO_PLL_CONFIG_EXT_DIV_GET(audio_pll_config);
        audio_pll_postplldiv=  AUDIO_PLL_CONFIG_POSTPLLDIV_GET(audio_pll_config);
        audio_pll_refdiv=      AUDIO_PLL_CONFIG_REFDIV_GET(audio_pll_config);

        audio_pll_tgt_div_frac=AUDIO_PLL_MODULATION_TGT_DIV_FRAC_GET(audio_pll_modulation); 
        audio_pll_tgt_div_int= AUDIO_PLL_MODULATION_TGT_DIV_INT_GET(audio_pll_modulation);
        audio_pll_mod_start=   AUDIO_PLL_MODULATION_START_GET(audio_pll_modulation);

        audio_pll_mod_step_frac = AUDIO_PLL_MOD_STEP_FRAC_GET(audio_pll_modulation_step);
        audio_pll_mod_step_int  = AUDIO_PLL_MOD_STEP_INT_GET(audio_pll_modulation_step);
        audio_pll_mod_step_update_cnt = AUDIO_PLL_MOD_STEP_UPDATE_CNT_GET(audio_pll_modulation_step);

        stereo_posedge = ATH_STEREO_CONFIG_PSEDGE(stereo_config); 

        printf("audio_pll_config: 0x%08X\n", audio_pll_config);
        printf("audio_pll_refdiv:     0x%04X\n", audio_pll_refdiv);
        printf("audio_pll_postplldiv: 0x%04X\n", audio_pll_postplldiv);
        printf("audio_pll_ext_div:    0x%04X\n", audio_pll_ext_div);
        printf("\n");

        printf("audio_pll_modulation: 0x%08X\n", audio_pll_modulation);
        printf("audio_pll_tgt_div_int:  0x%04X\n", audio_pll_tgt_div_int);
        printf("audio_pll_tgt_div_frac: 0x%04X\n", audio_pll_tgt_div_frac);
        printf("audio_pll_mod_start:    0x%04X\n", audio_pll_mod_start);
        printf("\n");

        printf("audio_pll_modulation_step: 0x%08X\n", audio_pll_modulation_step);
        printf("audio_pll_mod_step_int:        0x%04X\n", audio_pll_mod_step_int);
        printf("audio_pll_mod_step_frac:       0x%04X\n", audio_pll_mod_step_frac);
        printf("audio_pll_mod_step_update_cnd: 0x%04X\n", audio_pll_mod_step_update_cnt);
        printf("\n");
        
        printf("ATH_STEREO_CONFIG: 0x%08X\n", ATH_STEREO_CONFIG);
        printf("POSEDGE: 0x%08X -> 0x%02X\n", stereo_config, stereo_posedge);
        printf("\n");

        /*  <PLL frequency> = <REFCLK frequency>/ REFDIV * (DIV_INT = DIV_FRAC/(2<<18-1))/2<<POSTPLLDIV
            <MCLK Frequency> = <PLL Frequency>/EXT_DIV F_mclk = (F_REFCLK/REFDIV) * (NINT.NFRAC)/((2<<POSTPLLPWD) * (EXT_DIV))
        */

        pll_freq_multi = audio_pll_tgt_div_int + (float)(audio_pll_tgt_div_frac/(float)(2<<17));
        vcofreq = (float)(REF_CLK / audio_pll_refdiv) * pll_freq_multi;
        div = (1<< audio_pll_postplldiv) * audio_pll_ext_div;
        mclk_freq = vcofreq / ((float)(1<< audio_pll_postplldiv) * (float)(audio_pll_ext_div>>1));
        printf("reference frequency : %3.03f MHz\n", REF_CLK/1E6);
        printf("Audio PLL multi :     %3.03f\n", pll_freq_multi);
        printf("Audio PLL frequency : %3.03f MHz\n", vcofreq/1E6);
        printf("Audio MCLK frequency: %3.03f MHz\n", mclk_freq/1E6);
        printf("I2S    CLK frequency: %3.03f MHz\n", mclk_freq/1E6/stereo_posedge);
    }
    return 0;
}
