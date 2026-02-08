#include "WI_def.h"
#include "WI_act3.h"
void T_Player(objtype *ob);
/*
statetype gamestates[MAXSTATES]= {
  	{false,0,0,T_Player,NULL, s_none}//s_player
};
*/
statetype gamestates[MAXSTATES];
void InitVariable_act3(void)
{
	//
	gamestates[s_player].rotate=false;
	gamestates[s_player].shapenum=0;
	gamestates[s_player].tictime=0;
	gamestates[s_player].think=T_Player;
	gamestates[s_player].action=NULL;
	gamestates[s_player].next=s_none;
	
}
