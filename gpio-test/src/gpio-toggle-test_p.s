        .loc 1 62 0
        lui     $3,%hi(jiffies)  # tmp253,
        li      $2,983040                       # 0xf0000        # tmp221,
        lw      $4,%lo(jiffies)($3)      # t1, jiffies
        move    $5,$3    # tmp277, tmp253
        addiu   $2,$2,16960      # D.16462, tmp221,
$LBB18 = .
$LBB19 = .
        .file 3 "/openwrt/build_dir/target-mips_34kc_uClibc-0.9.33.2/linux-ar71xx_generic/linux-3.14.32/arch/mips/include/asm/io.h"
        .loc 3 428 0
        li      $3,-1342177280                  # 0xffffffffb0000000     # tmp278,
        addiu   $2,$2,-1         # D.16462, D.16462,
$L10:
        sw      $16,1580($3)     # mask, MEM[(volatile u32 *)2952791596B]
$LBE19 = .
$LBE18 = .
$LBB20 = .
$LBB21 = .
        sw      $16,1584($3)     # mask, MEM[(volatile u32 *)2952791600B]
$LBE21 = .
$LBE20 = .
        .loc 1 63 0
        bne     $2,$0,$L10       #, D.16462,,
        addiu   $2,$2,-1         # D.16462, D.16462,

        .loc 1 76 0
        lw      $7,%lo(jiffies)($5)      # t2, jiffies
        .loc 1 77 0
        li      $2,1000                 # 0x3e8  # tmp260,
        .loc 1 78 0
        li      $6,983040                       # 0xf0000        # tmp272,
        .loc 1 77 0
        subu    $7,$7,$4         # D.16464, t2, t1
        mul     $7,$7,$2         # D.16464, D.16464, tmp260
        li      $2,100                  # 0x64   # tmp265,

