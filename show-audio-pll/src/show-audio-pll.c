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

int main(int argc, char **argv) {
    uint32_t audio_pll_config, audio_pll_modulation;
    uint32_t audio_pll_ext_div, audio_pll_postplldiv, audio_pll_refdiv;
    uint32_t audio_pll_tgt_div_frac, audio_pll_tgt_div_int, audio_pll_mod_start;
    float pll_freq, mclk_freq, tgt_div, pll_freq_multi, pll_postplldiv, vcofreq;
    struct mmio pll_io;
    char *pll_input[ATH_PLL_MAX];

    bzero(pll_input, sizeof(pll_input));
    if (mmio_map(&pll_io, ATH_PLL_CONFIG, ATH_PLL_MAX))
        die_errno("mmio_map() failed");

    audio_pll_config = mmio_readl(&pll_io, 0x30);
    audio_pll_modulation = mmio_readl(&pll_io, 0x34);  
    
    mmio_unmap(&pll_io);

    if (AUDIO_PLL_CONFIG_BYPASS_GET(audio_pll_config)) {
        printf("bypassing Audio PLL\n");
        printf("reference frequency : %3.03f MHz\n", REF_CLK/1E6);
    } else {
        printf("audio_pll_modulation: 0x%08X\n", audio_pll_modulation);
        audio_pll_ext_div=AUDIO_PLL_CONFIG_EXT_DIV_GET(audio_pll_config);
        audio_pll_postplldiv=AUDIO_PLL_CONFIG_POSTPLLDIV_GET(audio_pll_config);
        audio_pll_refdiv=AUDIO_PLL_CONFIG_REFDIV_GET(audio_pll_config);

        audio_pll_tgt_div_frac=AUDIO_PLL_MODULATION_TGT_DIV_FRAC_GET(audio_pll_modulation); 
        audio_pll_tgt_div_int=AUDIO_PLL_MODULATION_TGT_DIV_INT_GET(audio_pll_modulation);
        audio_pll_mod_start=AUDIO_PLL_MODULATION_START_GET(audio_pll_modulation);

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

        /*  <PLL frequency> = <REFCLK frequency>/ REFDIV * (DIV_INT = DIV_FRAC/(2<<18-1))/2<<POSTPLLDIV
            <MCLK Frequency> = <PLL Frequency>/EXT_DIV F_mclk = (F_REFCLK/REFDIV) * (NINT.NFRAC)/((2<<POSTPLLPWD) * (EXT_DIV))
        */

        pll_freq_multi = audio_pll_tgt_div_int + (float)(audio_pll_tgt_div_frac/(float)(2<<17));
        vcofreq = (float)(REF_CLK / audio_pll_refdiv) * pll_freq_multi; 
        mclk_freq = vcofreq / (float)(2<< audio_pll_postplldiv) / (float)(audio_pll_ext_div >>1) ;
        printf("reference frequency : %3.03f MHz\n", REF_CLK/1E6);
        printf("Audio PLL multi :     %3.03f\n", pll_freq_multi);
        printf("Audio PLL frequency : %3.03f MHz\n", vcofreq/1E6);
        printf("Audio MCLK frequency: %3.03f MHz\n", mclk_freq/1E6);
        printf("I2S   MCLK frequency: %3.03f MHz\n", mclk_freq/1E6/2);
    }
    return 0;
}
