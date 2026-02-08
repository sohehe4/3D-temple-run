#include "WI_def.h"
#include "WI_act3.h"
#include "main.h"

#define ANGLE    360

boolean		madenoise;		// true when shooting or screaming

exit_t		playstate;

int		DebugOk;

objtype 	objlist[MAXACTORS],*newObj,*obj,*player,*lastobj,
			*objfreelist,*killerobj;

unsigned	farmapylookup[MAPSIZE];

boolean		singlestep,godmode,noclip;

byte		tilemap[MAPSIZE][MAPSIZE];	// wall values only
byte		spotvis[MAPSIZE][MAPSIZE];
int		actorat[MAPSIZE][MAPSIZE];

int tics;

//
// control info
//
boolean		mouseenabled,joystickenabled,joypadenabled;
int			joystickport;
int			dirscan[4] ;
int			buttonscan[NUMBUTTONS] ;
int			buttonmouse[4];
int			buttonjoy[4];

boolean		buttonheld[NUMBUTTONS];

boolean		demorecord,demoplayback;
byte		*demoptr, *lastdemoptr;
memptr		demobuffer;

//
// curent user input
//
int		controlx,controly;	/* range from -100 to 100 per tic */
boolean		buttonstate[NUMBUTTONS];

#define BASEMOVE		35

void TryLaneChange(int dir); // dir=-1左 / +1右

/*
						 LOCAL CONSTANTS
*/

#define MOVESCALE		150
#define BACKMOVESCALE		100
#define ANGLESCALE		20

/*
						 GLOBAL VARIABLES
*/

//
// player state info
//
long		thrustspeed;

unsigned	plux, pluy;	// player coordinates scaled to unsigned

int		anglefrac;
int		gotgatgun;

objtype		*LastAttacker;

/*
= TryMove
=
= returns true if move ok
=
判断是否可以移动。每个玩家占据一个半径为PLAYERSIZE	的圆周。
===================
*/

boolean TryMove(objtype *ob)
{
	int		xl,yl,xh,yh,x,y;
	objtype		*check;
	long		deltax,deltay;

	xl = (ob->x-PLAYERSIZE) >>TILESHIFT;
	yl = (ob->y-PLAYERSIZE) >>TILESHIFT;

	xh = (ob->x+PLAYERSIZE) >>TILESHIFT;
	yh = (ob->y+PLAYERSIZE) >>TILESHIFT;

//检查是否有挡路的墙。
// check for solid walls
//
	for (y=yl;y<=yh;y++)
		for (x=xl;x<=xh;x++)
		{
			//actorat[x][y]!=0表明其已经被占用(玩家走到了这里)，
			//而actorat[x][y] & 0x8000=1表示这是一块墙壁。
			//if (actorat[x][y] && !(actorat[x][y] & 0x8000))
			if (actorat[x][y] & 0x80)
				return false;
		}

//检查附近是否有挡路的活动物体。
// check for actors
//
	if (yl>0)
		yl--;
	if (yh<MAPSIZE-1)
		yh++;
	if (xl>0)
		xl--;
	if (xh<MAPSIZE-1)
		xh++;
	
	//在这个矩形区域内扫描。
	for (y=yl;y<=yh;y++)
		for (x=xl;x<=xh;x++)
		{
			if (actorat[x][y] & 0x8000) //这个地方有物体。
			{
				//根据物体的编号得到其数据结构指针。
				check = &objlist[actorat[x][y] & ~0x8000]; 
				//物体是可攻击的。
				if (check->flags & FL_SHOOTABLE)
				{
					deltax = ob->x - check->x;
					//X轴距离在玩家半径外
					if (deltax < -MINACTORDIST || deltax > MINACTORDIST)
						continue;
					//Y轴距离在玩家半径外
					deltay = ob->y - check->y;
					if (deltay < -MINACTORDIST || deltay > MINACTORDIST)
						continue;
					//在这个半径内又怎样?停下来攻击?
					return false;
				}
			}
		}

	return true;
}

/*
= ClipMove
=玩家走动。X坐标增量为xmove，Y坐标增量为ymove
*/

void ClipMove(objtype *ob, long xmove, long ymove)
{
	long	basex,basey;

	basex = ob->x;
	basey = ob->y;

	ob->x = basex+xmove;
	ob->y = basey+ymove;
	if (TryMove(ob))//判断是否可以移动到目的坐标点。
		return;

	if (noclip && ob->x > 2*TILEGLOBAL && ob->y > 2*TILEGLOBAL &&
	ob->x < (((long)(mapwidth-1))<<TILESHIFT)
	&& ob->y < (((long)(mapheight-1))<<TILESHIFT) )
		return;		// walk through walls Debug测试
		
	//if (!SD_SoundPlaying())
	//	SD_PlaySound (HITWALLSND);

	ob->x = basex+xmove;
	ob->y = basey;
	if (TryMove(ob))//尝试X轴方向运动；
		return;

	ob->x = basex;
	ob->y = basey+ymove;
	if (TryMove(ob))//尝试Y轴方向运动。
		return;

	ob->x = basex;//无法移动，恢复为尝试之前的值。
	ob->y = basey;
}


void VictoryTile(void)
{
#ifndef SPEAR
	//SpawnBJVictory();
#endif
	gamestate.victoryflag = true;
}


/*
								ACTIONS
*/

void Cmd_Fire(void)
{
/*
	buttonheld[bt_attack] = true;

	gamestate.weaponframe = 0;

	player->state = s_attack;

	gamestate.attackframe = 0;
	gamestate.attackcount =
		attackinfo[gamestate.weapon][gamestate.attackframe].tics;
	gamestate.weaponframe =
		attackinfo[gamestate.weapon][gamestate.attackframe].frame;
*/		
}


void Cmd_Use(void)
{

	int checkx, checky, doornum, dir;
	boolean elevatorok;

//
// find which cardinal direction the player is facing
//
	if (player->angle < ANGLES/8 || player->angle > 7*ANGLES/8)
	{
		checkx = player->tilex + 1;
		checky = player->tiley;
		dir = di_east;
		elevatorok = true;
	}
	else if (player->angle < 3*ANGLES/8)
	{
		checkx = player->tilex;
		checky = player->tiley-1;
		dir = di_north;
		elevatorok = false;
	}
	else if (player->angle < 5*ANGLES/8)
	{
		checkx = player->tilex - 1;
		checky = player->tiley;
		dir = di_west;
		elevatorok = true;
	}
	else
	{
		checkx = player->tilex;
		checky = player->tiley + 1;
		dir = di_south;
		elevatorok = false;
	}
/*
	doornum = tilemap[checkx][checky];
	if (*(mapsegs[1]+farmapylookup[checky]+checkx) == PUSHABLETILE)
	{
	//
	// pushable wall
	//

		PushWall (checkx,checky,dir);
		return;
	}
	if (!buttonheld[bt_use] && doornum == ELEVATORTILE && elevatorok)
	{
	//
	// use elevator
	//
		buttonheld[bt_use] = true;

		tilemap[checkx][checky]++;		// flip switch
		if (*(mapsegs[0]+farmapylookup[player->tiley]+player->tilex) == ALTELEVATORTILE)
			playstate = ex_secretlevel;
		else
			playstate = ex_completed;
		SD_PlaySound(LEVELDONESND);
		SD_WaitSoundDone();
	}
	else if (!buttonheld[bt_use] && doornum & 0x80)
	{
		buttonheld[bt_use] = true;
		OperateDoor(doornum & ~0x80);
	}
	else
		SD_PlaySound(DONOTHINGSND);
*/
}


void InitVariable_agent(void)
{
	//初始化agent相关变量。
}


void InitVariable_play(void)
{
	dirscan[0] = sc_UpArrow;
	dirscan[1] = sc_RightArrow;
	dirscan[2] = sc_DownArrow;
	dirscan[3] = sc_LeftArrow;
	
	buttonscan[0] =sc_Enter;
	buttonscan[0] =sc_BackSpace;
	buttonscan[0] =sc_LShift;
	buttonscan[0] =sc_Space;
	buttonscan[0] =sc_1;
	buttonscan[0] =sc_2;
	buttonscan[0] =sc_3;
	buttonscan[0] =sc_4;
	
	buttonmouse[0]=bt_attack;
	buttonmouse[1]=bt_strafe;
	buttonmouse[2]=bt_use;
	buttonmouse[3]=bt_nobutton;
	
	buttonjoy[0]=bt_attack;
	buttonjoy[1]=bt_strafe;
	buttonjoy[2]=bt_use;
	buttonjoy[3]=bt_run;

}

/*
#######################################

				  objlist 数据结构

objlist 包含当前正在游戏中的每个角色的结构。该结构作为一个链表访问，从 *player 开始，当 ob->next == NULL 时结束。GetNewObj 会在列表末尾插入一个新对象，这意味着如果一个角色生成另一个角色，新生成的角色将在同一帧执行思考和反应。RemoveObj 会取消给定对象的链接并将其返回到空闲列表，但不会破坏对象的 ->next 指针，因此如果当前对象移除自己，跟随的链表循环仍然可以安全地访问下一个元素。

<向后链接的空闲列表>
################################################
*/


/*
= InitActorList
=
= 用于清空演员对象列表，将它们全部返回到空闲列表。为玩家分配一个特殊位置。
*/


/*
=========================
=
= GetNewActor
=
= 将全局变量 new 设置为指向 objlist 中的一个空闲位置。
= 空闲位置会被插入到链表的末尾
=
=========================
*/

void GetNewActor(void)
{
	int id;
	
	//if (!objfreelist)
		//Quit("GetNewActor: No free spots in objlist!");
	if(!objfreelist)
		return;
	
	newObj = objfreelist;
	id = newObj->id;
	objfreelist = newObj->prev;
	
	memset(newObj, 0, sizeof(*newObj));
	newObj->id = id;
	
	if (lastobj)
		lastobj->next = newObj;
	newObj->prev = lastobj;	// newObj->next is already NULL from memset

	newObj->active = ac_no;
	lastobj = newObj;
}

/*
= RemoveObj
= 将给定对象重新添加到空闲列表中，并将其与邻居断开连接
*/

// 补充：RemoveObj 函数（对象移除，避免内存泄漏）
static void RemoveObj(objtype *gone) {
	if (gone == player) 
		//return; // 玩家不可移除

		gone->state = s_none;
	if (gone == lastobj) lastobj = gone->prev;
	else gone->next->prev = gone->prev;

	gone->prev->next = gone->next;
	gone->prev = objfreelist;
	objfreelist = gone;
}

void InitActorList(void)
{
	int	i;
//
// init the actor lists
//
	memset(objlist,0,MAXACTORS*sizeof(objtype));
	for (i = 0; i < MAXACTORS; i++)
	{
		objlist[i].id = i;
		objlist[i].prev = &objlist[i+1];
		objlist[i].next = NULL;
	}

	objlist[MAXACTORS-1].prev = NULL;

	objfreelist = &objlist[0];
	lastobj = NULL;

/* give the player the first free spots */
	GetNewActor();
	player = newObj;
}

/*
						   PLAYER CONTROL
*/


void SpawnPlayer (int tilex, int tiley, int dir)
{
	player->obclass = playerobj;
	player->active = ac_yes;
	player->tilex = tilex;
	player->tiley = tiley;
	//player->areanumber =
	//	*(mapsegs[0] + farmapylookup[player->tiley]+player->tilex);
	//初始化时，玩家的坐标是地砖坐标转换为16bit定点小数。
	player->x = ((long)tilex<<TILESHIFT)+TILEGLOBAL/2;
	player->y = ((long)tiley<<TILESHIFT)+TILEGLOBAL/2;
	player->state = s_player;
	player->angle = (1-dir)*90;
	if (player->angle<0)
		player->angle += ANGLES;//玩家的角度。0，90，180，270度。
	player->flags = FL_NEVERMARK;
	Thrust (0,0);				// set some variables

	//InitAreas();
}

void VictorySpin(void)
{
	long desty;

	if (player->angle > 270)
	{
		player->angle -= tics * 3;
		if (player->angle < 270)
			player->angle = 270;
	}
	else if (player->angle < 270)
	{
		player->angle += tics * 3;
		if (player->angle > 270)
			player->angle = 270;
	}
/*
	desty = (((long)player->tiley-5)<<TILESHIFT)-0x3000;

	if (player->y > desty)
	{
		player->y -= tics*4096;
		if (player->y < desty)
			player->y = desty;
	}
*/	
}

// Thrust函数新增结束检测（玩家回到起点）
void Thrust(int angle, long speed) {
    long xmove, ymove;
    unsigned offset;
	int32 i;

    thrustspeed += speed;
    
    if (speed >= MINDIST*2)
        speed = MINDIST*2 - 1;
    
    xmove = FixedByFrac(speed, costable[angle]);
    ymove = -FixedByFrac(speed, sintable[angle]);
    ClipMove(player, xmove, ymove);
    
    player->tilex = player->x >> TILESHIFT;
    player->tiley = player->y >> TILESHIFT;
    gamestate.player_tilex = player->tilex;
    gamestate.player_tiley = player->tiley;
    
    // 障碍碰撞检测
    
    for (i = 0; i < MAX_OBSTACLES; i++) {
        if (g_obstacles[i].exists && !g_is_jumping && 
            player->tilex == g_obstacles[i].tilex && 
            player->tiley == g_obstacles[i].tiley) {
            // 碰撞成功：隐藏障碍+减生命值
            g_obstacles[i].exists = FALSE;
            gamestate.lives--;
            
            // 生命值为0则游戏结束
            if (gamestate.lives <= 0) {
                g_GameOver = TRUE;
                gamestate.victoryflag = FALSE; // 标记为失败
            }
            
            break;
        }
    }
    
    // 检测是否回到起点（结束游戏）
    if (tilemap[player->tiley][player->tilex] == EXITTILE && g_GameOver) {
        g_GameOver = TRUE;
        gamestate.victoryflag = TRUE;
    }
    
    offset = farmapylookup[player->tiley] + player->tilex;
}

/*
= 改变玩家的角度和位置
= 有一个角度修正，因为在 70 帧每秒时，四舍五入变得很明显
*/

void ControlMovement(objtype *ob)
{
    int angle;
    int angleunits;
    
    thrustspeed = 0;
    
    if (buttonstate[bt_strafe])  //未用
    {
        // strafing
        if (controlx > 0)
        {
            angle = ob->angle - ANGLE/4;
            if (angle < 0)
                angle += ANGLE;
            Thrust(angle, controlx * MOVESCALE);  // 左移
        }
        else if (controlx < 0)
        {
            angle = ob->angle + ANGLE/4;
            if (angle >= ANGLE)
                angle -= ANGLE;
            Thrust(angle, -controlx * MOVESCALE);  // 右移
        }
    }
    else
    {
        // 转动角度（这部分现在由 PollKeyboardMove 处理）
        // 这里可以留空或处理其他逻辑
    }
    
    // 前进/后退（如果玩家手动操作）
    if (controly < 0)
    {
        Thrust(ob->angle, -controly * MOVESCALE);  // 前进
    }
    else if (controly > 0)
    {
        angle = ob->angle + ANGLE/2;
        if (angle >= ANGLE)
            angle -= ANGLE;
        Thrust(angle, controly * BACKMOVESCALE);  // 后退
    }
    
    if (gamestate.victoryflag)  // 观看胜利动画
        return;
}

void T_Player(objtype *ob)
{
	if (gamestate.victoryflag)		// watching the BJ actor
	{
		VictorySpin();
		return;
	}

	//UpdateFace();
	//CheckWeaponChange();

	if (buttonstate[bt_use])
		Cmd_Use();

	if (buttonstate[bt_attack] && !buttonheld[bt_attack])
		Cmd_Fire();

	ControlMovement(ob);
	if (gamestate.victoryflag)		// watching the BJ actor
		return;

	plux = player->x >> UNSIGNEDSHIFT;			// scale to fit in unsigned
	pluy = player->y >> UNSIGNEDSHIFT;
	player->tilex = player->x >> TILESHIFT;		// scale to tile values
	player->tiley = player->y >> TILESHIFT;
}

void DoActor(objtype *ob)
{
	void (*think)(objtype *);//这里声明了一个函数指针，函数的型参是objtype *

	if (!ob->active )
		return;//玩家不是活动状态。

	if (!(ob->flags & (FL_NONMARK|FL_NEVERMARK)))
		actorat[ob->tilex][ob->tiley] = 0;//这个是干嘛的呢?

//
// non transitional object
//
	gamestates[ob->state].rotate=false;
	gamestates[ob->state].shapenum=0;
	gamestates[ob->state].tictime=0;
	gamestates[ob->state].think=T_Player;
	gamestates[ob->state].action=NULL;
	gamestates[ob->state].next=s_none;
	
	if (!ob->ticcount)
	{
		think =	gamestates[ob->state].think;
		if (think)
		{
			think(ob);
			if (ob->state == s_none)
			{//删除对象
				RemoveObj (ob);
				return;
			}
		}

		if (ob->flags&FL_NEVERMARK)
			return;

		if ((ob->flags&FL_NONMARK) && actorat[ob->tilex][ob->tiley])
			return;

		actorat[ob->tilex][ob->tiley] = ob->id | 0x8000;
		return;
	}

//
// transitional object
//
	ob->ticcount-=tics;
	while (ob->ticcount <= 0)
	{
		think = gamestates[ob->state].action;	// end of state action
		if (think)
		{
			think(ob);
			if (ob->state == s_none)
			{
				RemoveObj(ob);
				return;
			}
		}

		ob->state = gamestates[ob->state].next;

		if (ob->state == s_none)
		{
			RemoveObj(ob);
			return;
		}

		if (!gamestates[ob->state].tictime)
		{
			ob->ticcount = 0;
			goto think;
		}

		ob->ticcount += gamestates[ob->state].tictime;
	}

think:
	//
	// think
	//
	think =	gamestates[ob->state].think;
	if (think)
	{
		think(ob);
		if (ob->state == s_none)
		{
			RemoveObj(ob);
			return;
		}
	}

	if (ob->flags&FL_NEVERMARK)
		return;

	if ((ob->flags&FL_NONMARK) && actorat[ob->tilex][ob->tiley])
		return;

	actorat[ob->tilex][ob->tiley] = ob->id | 0x8000;
}

int GetNearestDirection(int angle)
{
    // 定义四个方向的角度（以度为单位的近似值）
    // 注意：ANGLE 是 360，但代码中的角度可能是基于 ANGLES 或 ANGLE
    // 这里假设 ANGLE = 360 度
    
    int right_angle = 0;        // 0度 = 右
    int up_angle = ANGLE/4;     // 90度 = 上
    int left_angle = ANGLE/2;   // 180度 = 左
    int down_angle = 3*ANGLE/4; // 270度 = 下
    int min_diff,direction,threshold;
    // 计算角度差值
    int diff_up = abs(angle - up_angle);
    int diff_down = abs(angle - down_angle);
    int diff_left = abs(angle - left_angle);
    int diff_right = abs(angle - right_angle);
    
    // 考虑角度循环（0度和360度相同）
    if (diff_up > ANGLE/2) 
		diff_up = ANGLE - diff_up;
    if (diff_down > ANGLE/2) 
		diff_down = ANGLE - diff_down;
    if (diff_left > ANGLE/2) 
		diff_left = ANGLE - diff_left;
    if (diff_right > ANGLE/2) 
		diff_right = ANGLE - diff_right;
    
    // 找出最小差值的方向
    min_diff = diff_up;
    direction = 90;  // 上
    
    if (diff_down < min_diff)
    {
        min_diff = diff_down;
        direction = -90;  // 下
    }
    if (diff_left < min_diff)
    {
        min_diff = diff_left;
        direction = 180;   // 左
    }
    if (diff_right < min_diff)
    {
        min_diff = diff_right;
        direction = 0;   // 右
    }
    
    // 检查是否在45度（ANGLE/8）范围内
    threshold = ANGLE/8;  // 45度
    
    if (min_diff < threshold)
    {
        return direction;
    }
    
    return -1;  // 不在任何方向的45度范围内
}

void AutoThrustByDirection(void)
{
    
    // 获取玩家当前角度最接近的方向
    int nearest_dir = GetNearestDirection(player->angle);
    
    
    if (nearest_dir != -1)
    {      
        Thrust(nearest_dir, BASEMOVE * 2 * tics);  // 前进
    }
    
}

void PollKeyboardButtons(void)
{
	int i;
	for (i=0;i<NUMBUTTONS;i++)//扫描8个命令键当前状态。
		if (IN_KeyDown(buttonscan[i]))//回车、退格、左shift、空格、1、2、3、4。
			buttonstate[i] = true;		
}

// 左右变道（检查是否是道路再跳）
void TryLaneChange(int dir)
{
    int moveAngle, moveDistance;
    long xmove, ymove;
    
    // 使用和 strafing 相同的逻辑
    if (dir > 0)  // 右移
    {
        moveAngle = player->angle + ANGLE/4;  // 右侧90度
        if (moveAngle >= ANGLE)
            moveAngle -= ANGLE;
    }
    else  // 左移
    {
        moveAngle = player->angle - ANGLE/4;  // 左侧90度
        if (moveAngle < 0)
            moveAngle += ANGLE;
    }
    
    // 移动一个地砖的距离
    moveDistance = TILEGLOBAL;  // 一个地砖的宽度
    
    // 计算移动向量
    xmove = FixedByFrac(moveDistance, costable[moveAngle]);
    ymove = -FixedByFrac(moveDistance, sintable[moveAngle]);
    
    // 使用 ClipMove 移动
    ClipMove(player, xmove, ymove);
    
    // 更新坐标
    player->tilex = player->x >> TILESHIFT;
    player->tiley = player->y >> TILESHIFT;
    
}

void PollKeyboardMove(void)
{
    static int leftKeyHoldTime = 0;
    static int rightKeyHoldTime = 0;
   

    // 重置 controly，避免累积
    controly = 0;
    controlx = 0;
    
	// 上下键保持原来的前进后退功能
    if (IN_KeyDown(sc_UpArrow))
        controly -= BASEMOVE * tics;
    /*
    if (IN_KeyDown(sc_DownArrow))
        controly += BASEMOVE * tics;
    */
    
    // 左右键处理 - 改变角度或变道
    if (IN_KeyDown(sc_LeftArrow))
    {
        leftKeyHoldTime += tics;
        if (leftKeyHoldTime > 6)  // 长按：转动角度
        {
            // 左转（逆时针）每次转动约5度
            player->angle += 2 * tics;  // 每帧
            if (player->angle >= ANGLE)
                player->angle -= ANGLE;
            leftKeyHoldTime = 0;
        }
    }
    else if (leftKeyHoldTime > 0 && leftKeyHoldTime <= 5)  // 短按释放：左移
    {
        TryLaneChange(-1);  // 左变道
        leftKeyHoldTime = 0;
    }
    else
    {
        leftKeyHoldTime = 0;
    }
    
    if (IN_KeyDown(sc_RightArrow))
    {
        rightKeyHoldTime += tics;
        if (rightKeyHoldTime > 6)  // 长按：转动角度
        {
            // 右转（顺时针）每次转动约5度
            player->angle -= 2 * tics;  // 每帧转
            if (player->angle < 0)
                player->angle += ANGLE;
            rightKeyHoldTime = 0;
        }
    }
    else if (rightKeyHoldTime > 0 && rightKeyHoldTime <= 5)  // 短按释放：右移
    {
        TryLaneChange(1);   // 右变道
        rightKeyHoldTime = 0;
    }
    else
    {
        rightKeyHoldTime = 0;
    }
       
}



void UpdateInput(void)
{
//
// get button states
//扫描、读取被按下的命令、动作键值
	PollKeyboardButtons();
//
// get movements
//根据运动方向和状态修改运动增量的x、y轴方向分量controlx、controly
	PollKeyboardMove();
}


/*
===================
=
= PollControls
=
= 获取用户或演示输入，每帧调用一次
=
= controlx	每个刻度设置在 -100 到 100 之间
= controly
= buttonheld[]	上一帧按钮状态
= buttonstate[]	本帧按钮状态
=
===================
*/

void PollControls(void)
{
	int max, min;

	controlx = 0;
	controly = 0;
	memcpy(buttonheld, buttonstate, sizeof(buttonstate));
	memset(buttonstate, 0, sizeof(buttonstate));

	/* Update keys */
	IN_CheckAck(); 
	//获取当前的命令键状态和controlx、controly的值。
	UpdateInput();
	
//
// bound movement to a maximum
//
	max = 100*tics;
	min = -max;
	if (controlx > max)
		controlx = max;
	else if (controlx < min)
		controlx = min;

	if (controly > max)
		controly = max;
	else if (controly < min)
		controly = min;
}

void CheckKeys(void)
{
	//检查用户输入的命令并作出反应、处理。
}

// 帧循环核心函数（放在 game_logic.c 中）
void PlayLoop(void)
{
    // 1. 计算帧间隔
    CalcTics();
    
    // 2. 读取玩家输入
    PollControls();
    
    // 只有当玩家角度与四个方向，某方向夹角小于45度时，即角度是唯一的，自动前进
    AutoThrustByDirection();
    
    // 使用当前玩家角度自动前进
    Thrust(player->angle, BASEMOVE * 50);  // 10倍基础速度
    
    // 4. 执行玩家逻辑
    for (obj = player; obj; obj = obj->next)
    {
        DoActor(obj);
    }
    
    // 5. 触发3D渲染
    ThreeDRefresh();
    
    // 6. 更新游戏时间计数
    gamestate.TimeCount += tics;
}


