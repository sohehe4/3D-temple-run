#include "WI_def.h"
#include "Vi_comm.h"

//
// 配置 variables
//

// 	Global variables
boolean		Keyboard[NumCodes];
boolean		InternalKeyboard[NumCodes];
boolean		Paused;
char		LastASCII;
ScanCode	LastScan;

KeyboardDef KbdDefs;

ControlType	Controls[MaxPlayers];


static	boolean		IN_Started;
static	boolean		CapsLock;
static	ScanCode	CurCode,LastCode;

static	Direction	DirTable[9];// Quick lookup for total direction

static  boolean btnstate[8];


uint16 *gfxbuf;

int xfrac, yfrac;


void VL_Bar(int x, int y, int width, int height, int color)
{
	uint16 *ptr = gfxbuf + vwidth * y + x;
	while (height--) {
		memset(ptr, color, width*sizeof(uint16));
		ptr += vwidth;
	}	
}


void VW_Startup(void)
{
	VL_Startup();//视频子系统初始化，如分配gfxbuf缓冲区。
	
	xfrac = (vwidth << 16) / 176;
	yfrac = (vheight << 16) / 208;
}

void InitVariable_MyThroad(void)
{
	return;
}

void VW_UpdateScreen(void)
{
	//VL_WaitVBL(1); //这个函数用于等待射线返回，防止屏幕闪烁。
	//memcpy(graphmem, gfxbuf, vwidth*vheight);//写显存。
	//memcpy(VideoBuf.Data, gfxbuf, vwidth*vheight);//写显存。
	//mrc_bitmapShowEx(gfxbuf,0,0,240,240,320,BM_COPY,0,0);
	mrc_refreshScreen(0, 0, 240, 320);	
}

void INL_Update(void)
{
	//扫描键盘状态变化。
	//由于采用了异步事件处理方式，将由键盘事件的
	//回调函数进行处理。
}


void VL_Startup(void)
{
	//gfxbuf = mrc_malloc(vwidth * vheight * sizeof(uint16));
	gfxbuf=0;
}

void VL_Shutdown(void)
{
	if (gfxbuf != NULL) {
		mrc_free(gfxbuf);
		gfxbuf = NULL;
	}
}


//	IN_ClearKeysDown() - 清除键盘数组
void IN_ClearKeysDown(void)
{
	LastScan = sc_None;
	LastASCII = key_None;
	memset(Keyboard, 0, sizeof(Keyboard));
}


//	INL_StartKbd() - 设置我的键盘功能以便使用
static void INL_StartKbd(void)
{
	IN_ClearKeysDown();
}



//	IN_Ack() -等待按钮或按键被按下。如果有按钮被按下，则在
// 调用，它必须被释放才能被识别
void IN_StartAck(void)
{
// get initial state of everything
	IN_ClearKeysDown();
	memset (btnstate,0,sizeof(btnstate));
}

boolean IN_CheckAck(void)
{	
	unsigned i, buttons=0;
	
	INL_Update();		
	if (LastScan)
		return true;	
	for (i=0;i<8;i++,buttons>>=1)//这个地方在搞什么鬼呢?
		if ( buttons&1 )
		{
			if (!btnstate[i])
				return true;
		}
		else
			btnstate[i]=false;

	return false;
}


///////////////////////////////////////////////////////////////////////////
//
//	IN_UserInput() - Waits for the specified delay time (in ticks) or the
//		user pressing a key or a mouse button. If the clear flag is set, it
//		then either clears the key or waits for the user to let the mouse
//		button up.
//
///////////////////////////////////////////////////////////////////////////
boolean IN_UserInput(longword delay)
{
	longword	lasttime;

	lasttime = mrc_getUptime();
	
	IN_StartAck();
	do {
		if (IN_CheckAck())
			return true;
	} while ( (mrc_getUptime() - lasttime) < delay );
	
	return false;
}


//	INL_ShutKbd() - Restores keyboard control to the BIOS
static void INL_ShutKbd(void)
{
}

//	IN_Startup() - Starts up the Input Mgr
void IN_Startup(void)
{
	if (IN_Started)
		return;
	INL_StartKbd();
	IN_Started = true;
}


//	IN_Shutdown() - Shuts down the Input Mgr
void IN_Shutdown(void)
{
	if (!IN_Started)
		return;
	INL_ShutKbd();

	IN_Started = false;
}


//	IN_ReadControl() - Reads the device associated with the specified
//		player and fills in the control info struct
void IN_ReadControl(int player,ControlInfo *info)
{
			boolean		realdelta = false;
			word		buttons;
			int			dx,dy;
			Motion		mx,my;
			ControlType	type;
			KeyboardDef	*def;

	dx = dy = 0;
	mx = my = motion_None;
	buttons = 0;

		IN_CheckAck();

		switch (type = Controls[player])
		{
		case ctrl_Keyboard:
			def = &KbdDefs;

			if (Keyboard[def->upleft])
				mx = motion_Left,my = motion_Up;
			else if (Keyboard[def->upright])
				mx = motion_Right,my = motion_Up;
			else if (Keyboard[def->downleft])
				mx = motion_Left,my = motion_Down;
			else if (Keyboard[def->downright])
				mx = motion_Right,my = motion_Down;

			if (Keyboard[def->up])
				my = motion_Up;
			else if (Keyboard[def->down])
				my = motion_Down;

			if (Keyboard[def->left])
				mx = motion_Left;
			else if (Keyboard[def->right])
				mx = motion_Right;

			if (Keyboard[def->button0])
				buttons += 1 << 0;
			if (Keyboard[def->button1])
				buttons += 1 << 1;
			realdelta = false;
			break;
		}

	if (realdelta)
	{
		mx = (dx < 0)? motion_Left : ((dx > 0)? motion_Right : motion_None);
		my = (dy < 0)? motion_Up : ((dy > 0)? motion_Down : motion_None);
	}
	else
	{
		dx = mx * 127;
		dy = my * 127;
	}

	info->x = dx;
	info->xaxis = mx;
	info->y = dy;
	info->yaxis = my;
	info->button0 = buttons & (1 << 0);
	info->button1 = buttons & (1 << 1);
	info->button2 = buttons & (1 << 2);
	info->button3 = buttons & (1 << 3);
	info->dir = DirTable[((my + 1) * 3) + (mx + 1)];
}


void IN_Ack(void)
{
	IN_StartAck();

	while(!IN_CheckAck()) ;
}

void InitVariable_Vi_comm(void)
{
	// Quick lookup for total direction
	DirTable[0] =dir_NorthWest	;
	DirTable[1] =dir_North	;
	DirTable[2] =dir_NorthEast	;
	DirTable[3] =dir_West	;
	DirTable[4] =dir_None	;
	DirTable[5] =dir_East	;
	DirTable[6] =dir_SouthWest;
	DirTable[7] =dir_South;
	DirTable[8] =dir_SouthEast;

	KbdDefs.button0= sc_Control;
	KbdDefs.button1= sc_Alt;
	KbdDefs.upleft= sc_Home;
	KbdDefs.up= sc_UpArrow;
	KbdDefs.upright= sc_PgUp;
	KbdDefs.left= sc_LeftArrow;
	KbdDefs.right = sc_RightArrow;
	KbdDefs.downleft= sc_End;
	KbdDefs.down= sc_DownArrow;
	KbdDefs.downright= sc_PgDn;	
}
