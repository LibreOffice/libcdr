/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * This file is part of the libcdr project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __CDRDOCUMENTSTRUCTURE_H__
#define __CDRDOCUMENTSTRUCTURE_H__

// integer representations of fourcc

#define CDR_FOURCC_arrt 0x74727261
#define CDR_FOURCC_arrw 0x77727261
#define CDR_FOURCC_arrx 0x78727261

#define CDR_FOURCC_bbox 0x786f6262
#define CDR_FOURCC_bcfg 0x67666362
#define CDR_FOURCC_bmkt 0x746b6d62
#define CDR_FOURCC_bmp  0x20706d62
#define CDR_FOURCC_bmpf 0x66706d62
#define CDR_FOURCC_bmpt 0x74706d62
#define CDR_FOURCC_bmpx 0x78706d62
#define CDR_FOURCC_bmrk 0x6b726d62
#define CDR_FOURCC_bpft 0x74667062

#define CDR_FOURCC_CDR 0x524443
#define CDR_FOURCC_ccmm 0x6d6d6363
#define CDR_FOURCC_cdr 0x726463
#define CDR_FOURCC_clo  0x206f6c63
#define CDR_FOURCC_cloa 0x616f6c63
#define CDR_FOURCC_clof 0x666f6c63
#define CDR_FOURCC_clol 0x6c6f6c63
#define CDR_FOURCC_cloo 0x6f6f6c63
#define CDR_FOURCC_clpt 0x74706c63
#define CDR_FOURCC_cmpr 0x72706d63
#define CDR_FOURCC_cont 0x746e6f63
#define CDR_FOURCC_CPng 0x676e5043

#define CDR_FOURCC_data 0x61746164
#define CDR_FOURCC_digk 0x6b676964
#define CDR_FOURCC_DISP 0x50534944
#define CDR_FOURCC_doc  0x20636f64

#define CDR_FOURCC_envd 0x64766e65

#define CDR_FOURCC_filc 0x636c6966
#define CDR_FOURCC_fild 0x646c6966
#define CDR_FOURCC_fill 0x6c6c6966
#define CDR_FOURCC_filt 0x746c6966
#define CDR_FOURCC_flgs 0x73676c66
#define CDR_FOURCC_fntt 0x74746e66
#define CDR_FOURCC_font 0x746e6f66
#define CDR_FOURCC_ftil 0x6c697466
#define CDR_FOURCC_fver 0x72657666

#define CDR_FOURCC_gobj 0x6a626f67
#define CDR_FOURCC_grp  0x20707267
#define CDR_FOURCC_guid 0x64697567

#define CDR_FOURCC_iccd 0x64636369
#define CDR_FOURCC_iccp 0x70636369
#define CDR_FOURCC_imag 0x67616d69
#define CDR_FOURCC_indx 0x78646e69
#define CDR_FOURCC_info 0x6f666e69
#define CDR_FOURCC_ixef 0x66657869
#define CDR_FOURCC_ixlr 0x726c7869
#define CDR_FOURCC_ixmr 0x726d7869
#define CDR_FOURCC_ixpc 0x63707869
#define CDR_FOURCC_ixpg 0x67707869
#define CDR_FOURCC_ixtl 0x6c747869
#define CDR_FOURCC_ICMT 0x544d4349
#define CDR_FOURCC_IKEY 0x59454b49
#define CDR_FOURCC_INFO 0x4f464e49

#define CDR_FOURCC_layr 0x7279616c
#define CDR_FOURCC_lgob 0x626f676c
#define CDR_FOURCC_LIST 0x5453494c
#define CDR_FOURCC_lnkg 0x676b6e6c
#define CDR_FOURCC_lnkt 0x746b6e6c
#define CDR_FOURCC_loda 0x61646f6c
#define CDR_FOURCC_lobj 0x6a626f6c

#define CDR_FOURCC_mask 0x6b73616d
#define CDR_FOURCC_mcfg 0x6766636d

#define CDR_FOURCC_npps 0x7370706e

#define CDR_FOURCC_obbx 0x7862626f
#define CDR_FOURCC_obj  0x206a626f
#define CDR_FOURCC_osfp 0x7066736f
#define CDR_FOURCC_otlt 0x746c746f
#define CDR_FOURCC_outl 0x6c74756f

#define CDR_FOURCC_page 0x65676170
#define CDR_FOURCC_pfrd 0x64726670
#define CDR_FOURCC_ppdt 0x74647070
#define CDR_FOURCC_ppid 0x64697070
#define CDR_FOURCC_pref 0x66657270
#define CDR_FOURCC_proc 0x636f7270
#define CDR_FOURCC_ptrt 0x74727470

#define CDR_FOURCC_RIFF 0x46464952
#define CDR_FOURCC_RIFX 0x58464952
#define CDR_FOURCC_rarw 0x77726172
#define CDR_FOURCC_rclr 0x726c6372
#define CDR_FOURCC_rdot 0x746f6472
#define CDR_FOURCC_rlst 0x74736c72
#define CDR_FOURCC_rota 0x61746f72
#define CDR_FOURCC_rotl 0x6c746f72
#define CDR_FOURCC_rott 0x74746f72
#define CDR_FOURCC_rpen 0x6e657072
#define CDR_FOURCC_rscr 0x72637372
#define CDR_FOURCC_rvsn 0x6e737672

#define CDR_FOURCC_spnd 0x646e7073
#define CDR_FOURCC_stlt 0x746c7473
#define CDR_FOURCC_styd 0x64797473
#define CDR_FOURCC_styl 0x6c797473
#define CDR_FOURCC_sumi 0x696d7573
#define CDR_FOURCC_summ 0x6d6d7573

#define CDR_FOURCC_trfd 0x64667274
#define CDR_FOURCC_trfl 0x6c667274
#define CDR_FOURCC_ttil 0x6c697474
#define CDR_FOURCC_txsm 0x6d737874
#define CDR_FOURCC_txtj 0x6a747874

#define CDR_FOURCC_udta 0x61746475
#define CDR_FOURCC_uidr 0x72646975
#define CDR_FOURCC_uil  0x206c6975
#define CDR_FOURCC_url  0x206c7275
#define CDR_FOURCC_urll 0x6c6c7275
#define CDR_FOURCC_urls 0x736c7275
#define CDR_FOURCC_usdn 0x6e647375
#define CDR_FOURCC_user 0x72657375
#define CDR_FOURCC_usft 0x74667375

#define CDR_FOURCC_vect 0x74636576
#define CDR_FOURCC_vpat 0x74617076
#define CDR_FOURCC_vrsn 0x6e737276

// Types of Styd arguments

#define STYD_NAME 0xc8
#define STYD_FILL_ID 0xcd
#define STYD_OUTL_ID 0xd2
#define STYD_FONTS 0xdc
#define STYD_ALIGN 0xe1
#define STYD_BULLETS 0xe6
#define STYD_INTERVALS 0xeb
#define STYD_TABS 0xf0
#define STYD_IDENTS 0xf5
#define STYD_HYPHENS 0xfa
#define STYD_SET5S 0xff
#define STYD_DROPCAPS 0x104


#endif // __CDRDOCUMENTSTRUCTURE_H__
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
