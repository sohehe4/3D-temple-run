#ifndef __WL_ACT3_H__
#define __WL_ACT3_H__

#include "WI_def.h"

typedef enum {
	s_none,
	
	s_boom1,
	s_boom2,
	s_boom3,

	s_rocket,

	s_smoke1,
	
	s_grdstand,

	s_grdpath1,

	s_grdpain,
	s_grdpain1,

	s_grdshoot1,
	s_grdshoot2,
	s_grdshoot3,

	s_grdchase1,

	s_grddie1,
	s_grddie2,
	s_grddie3,
	s_grddie4,


	s_dogpath1,

	s_dogjump1,
	

	s_dogchase1,

	s_dogdie1,
	s_dogdie2,
	s_dogdie3,
	s_dogdead,

//
// officers
//

	s_ofcstand,

	s_ofcpath1,

	s_ofcpain,
	s_ofcpain1,

	s_ofcshoot1,
	s_ofcshoot2,
	s_ofcshoot3,

	s_ofcchase1,

	s_ofcdie1,



//
// mutant
//

	s_mutstand,

	s_mutpath1,

	s_mutpain,
	s_mutpain1,

	s_mutshoot1,
	s_mutshoot2,
	s_mutshoot3,
	s_mutshoot4,

	s_mutchase1,

	s_mutdie1,
	s_mutdie2,
	s_mutdie3,
	s_mutdie4,
	s_mutdie5,


//
// SS
//

	s_ssstand,

	s_sspath1,
	s_sspath1s,
	s_sspath2,
	s_sspath3,
	s_sspath3s,
	s_sspath4,

	s_sspain,
	s_sspain1,

	s_ssshoot1,

	s_sschase1,

	s_ssdie1,
	s_ssdie2,
	s_ssdie3,
	s_ssdie4,

#ifndef SPEAR

	s_blinkychase1,

//
// hans
//
	s_bossstand,

	s_bosschase1,
	s_bosschase1s,
	s_bosschase2,
	s_bosschase3,
	s_bosschase3s,
	s_bosschase4,

	s_bossdie1,
	s_bossdie2,
	s_bossdie3,
	s_bossdie4,

	s_bossshoot1,
	s_bossshoot2,
	s_bossshoot3,
	s_bossshoot4,
	s_bossshoot5,
	s_bossshoot6,
	s_bossshoot7,
	s_bossshoot8,

//
// gretel
//
	s_gretelstand,

	s_gretelchase1,
	s_gretelchase1s,
	s_gretelchase2,
	s_gretelchase3,
	s_gretelchase3s,
	s_gretelchase4,

	s_greteldie1,
	s_greteldie2,
	s_greteldie3,
	s_greteldie4,

	s_gretelshoot1,
	s_gretelshoot2,
	s_gretelshoot3,
	s_gretelshoot4,
	s_gretelshoot5,
	s_gretelshoot6,
	s_gretelshoot7,
	s_gretelshoot8,

//
// schabb
//
	s_schabbstand,

	s_schabbchase1,
	s_schabbchase1s,
	s_schabbchase2,
	s_schabbchase3,
	s_schabbchase3s,
	s_schabbchase4,

	s_schabbdeathcam,

	s_schabbdie1,
	s_schabbdie2,
	s_schabbdie3,
	s_schabbdie4,
	s_schabbdie5,
	s_schabbdie6,

	s_schabbshoot1,
	s_schabbshoot2,

	s_needle1,
	s_needle2,
	s_needle3,
	s_needle4,


//
// gift
//
	s_giftstand,

	s_giftchase1,
	s_giftchase1s,
	s_giftchase2,
	s_giftchase3,
	s_giftchase3s,
	s_giftchase4,

	s_giftdeathcam,

	s_giftdie1,
	s_giftdie2,
	s_giftdie3,
	s_giftdie4,
	s_giftdie5,
	s_giftdie6,

	s_giftshoot1,
	s_giftshoot2,

//
// fat
//
	s_fatstand,

	s_fatchase1,
	s_fatchase1s,
	s_fatchase2,
	s_fatchase3,
	s_fatchase3s,
	s_fatchase4,

	s_fatdeathcam,

	s_fatdie1,
	s_fatdie2,
	s_fatdie3,
	s_fatdie4,
	s_fatdie5,
	s_fatdie6,

	s_fatshoot1,
	s_fatshoot2,
	s_fatshoot3,
	s_fatshoot4,
	s_fatshoot5,
	s_fatshoot6,


//
// fake
//
	s_fakestand,

	s_fakechase1,
	s_fakechase1s,
	s_fakechase2,
	s_fakechase3,
	s_fakechase3s,
	s_fakechase4,

	s_fakedie1,
	s_fakedie2,
	s_fakedie3,
	s_fakedie4,
	s_fakedie5,
	s_fakedie6,

	s_fakeshoot1,
	s_fakeshoot2,
	s_fakeshoot3,
	s_fakeshoot4,
	s_fakeshoot5,
	s_fakeshoot6,
	s_fakeshoot7,
	s_fakeshoot8,
	s_fakeshoot9,

	s_fire1,
	s_fire2,


//
// hitler
//

	s_mechastand,

	s_mechachase1,
	s_mechachase1s,
	s_mechachase2,
	s_mechachase3,
	s_mechachase3s,
	s_mechachase4,

	s_mechadie1,
	s_mechadie2,
	s_mechadie3,
	s_mechadie4,

	s_mechashoot1,
	s_mechashoot2,
	s_mechashoot3,
	s_mechashoot4,
	s_mechashoot5,
	s_mechashoot6,


	s_hitlerchase1,
	s_hitlerchase1s,
	s_hitlerchase2,
	s_hitlerchase3,
	s_hitlerchase3s,
	s_hitlerchase4,

	s_hitlerdeathcam,

	s_hitlerdie1,
	s_hitlerdie2,
	s_hitlerdie3,
	s_hitlerdie4,
	s_hitlerdie5,
	s_hitlerdie6,
	s_hitlerdie7,
	s_hitlerdie8,
	s_hitlerdie9,
	s_hitlerdie10,

	s_hitlershoot1,
	s_hitlershoot2,
	s_hitlershoot3,
	s_hitlershoot4,
	s_hitlershoot5,
	s_hitlershoot6,

//
// BJ victory
//

	s_bjrun1,
	s_bjrun1s,
	s_bjrun2,
	s_bjrun3,
	s_bjrun3s,
	s_bjrun4,

	s_bjjump1,
	s_bjjump2,
	s_bjjump3,
	s_bjjump4,

	s_deathcam,

#endif

	s_player,
	s_attack,

	MAXSTATES
} stateenum;

void T_Player(objtype *ob);
extern statetype gamestates[MAXSTATES];

#endif

