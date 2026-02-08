#include "WI_def.h"
#include "mpc.h"
/*
=============================================================
德军总部3D               WOLFENSTEIN 3-D
					  An Id Software production
						   by John Carmack
1992年德军总部3D能在 12MHz 的 386 上运行
==============================================================
*/

#define FOCALLENGTH     0x5800		/* in global coordinates */

char str[80], str2[20];

int viewwidth, viewheight;
int viewwidthwin, viewheightwin; /* for borders */
int xoffset, yoffset;
int vwidth, vheight; /* size of screen */
int viewsize;

int centerx;
int shootdelta;			/* pixels away from centerx a target can be */

boolean startgame,loadedgame;
int mouseadjustment;

long frameon;
long lasttimecount;
fixed viewsin, viewcos;
fixed viewx, viewy;		/* the focal point */
int pixelangle[MAXVIEWWIDTH];
long finetangent[FINEANGLES/4];//0到90度的正切值。精确到0.1度，即[0,900)。
int horizwall[MAXWALLTILES], vertwall[MAXWALLTILES];

char configname[13] = "config.";

fixed sintable[ANGLES+ANGLES/4+1], *costable;//360度的正弦值。

//uint16 MazeWidth,MazeHeight;
//byte *pMap;
//uint16 *pPic[8];//图片缓冲，色深16bit
mr_bitmapSt bitmapInfo;


boolean		ingame;
gametype	gamestate;

long		spearx,speary;
unsigned	spearangle;
boolean		spearflag;


void InitVariable_game(void)
{
	//初始化game相关变量。
}



/*
========================
=
= FixedByFrac (FixedMul)
=
= multiply two 16/16 bit, 2's complement fixed point numbers
=
========================
*/
#ifdef __SYMBIAN32__
	fixed FixedByFrac(fixed a, fixed b)
	{
		int ret;

		// multiply the values -> r6:r7
		asm volatile ( "SMULL r6, r7, %0, %1" : : 
					   "r"(a), "r"(b) : "r6", "r7" );
		// shift away the lowest 16 bits of result precision 
		asm volatile ( "MOV r6, r6, LSR #16" : : : "r6" );
		// or the halfwords together
		asm volatile ( "ORR r6, r6, r7, LSL #16" : : : "r6" );
		// store in memory for return
		asm volatile ( "STR r6, %0" : "=m"(ret) );
 
		return ret;
	}
#else
	#ifdef NOASM
		#if defined(__INTEL_COMPILER)
			typedef int64 int64_t;
		#endif
		#if defined(__ARM7_COMPILER)
			typedef __int64 int64_t;
		#endif
		fixed FixedByFrac(fixed a, fixed b)
		{
			int64_t ra = a;
			int64_t rb = b;
			int64_t r;
			
			r = ra * rb;
			r >>= TILESHIFT;
			return (fixed)r;
		}
	#else
	    #define FixedByFrac(x, y) \
        __extension__  \
        ({ unsigned long z; \
         asm("imull %2; shrdl $16, %%edx, %%eax" : "=a" (z) : "a" (x), "q" (y) : "%edx"); \
         z; \
        })
	#endif
#endif


/*
=====================
=
= CalcTics
=计算两个帧之间相隔的的滴答数。
=大致上是秒数(通常是零点零几秒)乘以70。
=如果滴答数大于10，则限制为10。
=====================
*/

void CalcTics(void)
{
	int newtime;
	int ticcount;
	
	ticcount = 0 + 1; /* 35 Hz */
	
	/* glossing over a bug in the deathcam code.. */
	newtime = mrc_getUptime();
	if ((newtime - lasttimecount) < 0)
		lasttimecount=newtime;
		
	do {
		newtime = mrc_getUptime();
		tics = (newtime - lasttimecount)/20;//除以20是调节速度。
		if(tics<0)
			tics=-tics;
	} while (tics <= ticcount);
	
	lasttimecount = newtime;
	
	if (tics > MAXTICS)
	  tics = MAXTICS;
	//tics%=MAXTICS;
}


static double radtoint ;//radtoint = (double)FINEANGLES/2.0/PI;单位是0.1度/弧度

void BuildTables(void) {
    int i;
    
    double tang;
	long sin_scaled;

/* calculate fine tangents */

	finetangent[0] = 0;
	for (i = 1; i < FINEANGLES/8; i++) { // 1 到45度
		tang = tan((double)i/radtoint);// 1到45度对应的正切值。
		finetangent[i] =(long) (tang*TILEGLOBAL);
		finetangent[FINEANGLES/4-1-i] =(long) (TILEGLOBAL/tang);// 45度到90度用近似计算?

	}
	
	/* fight off asymptotic behaviour at 90 degrees */
	finetangent[FINEANGLES/4-1] = finetangent[FINEANGLES/4-2]+1;
    
    /* 使用 mycos/mysin 生成 sintable 和 costable */
    for (i = 0; i <= ANGLES + ANGLES/4; i++) {
        
        sin_scaled = mysin(i);
        sintable[i] = GLOBAL1 / 4096 * sin_scaled;

    }
    
}

/*
= SetupWalls
=
= 将图块数值映射到缩放图片
*/

void SetupWalls(void)
{
	int i;

	for (i=1;i<MAXWALLTILES;i++)
	{
		horizwall[i]=(i-1)*2;
		vertwall[i]=(i-1)*2+1;
	}
}



void NewViewSize(int width)
{

	//if (width > 12)
	//	width = 12;
	if (width < 4)
		width = 4;	
	
	if (width==12) 
	{
		viewwidthwin = 176;
		viewheightwin = 208;
		viewsize = 12;//这个12是什么含义呢?

		viewwidth = 176;
		viewheight = 208;

		centerx = viewwidth/2-1;
		shootdelta = viewwidth/10;

		yoffset = 0;
		xoffset = 0;
	} else 
	{
		if ((width*16) > vwidth)
			width = vwidth / 16;
		
		if ((width*16*HEIGHTRATIO) > (vheight - 40/*vheight/208*/))
			width = (vheight - 40/*vheight/208*/)/8;
		
		viewwidthwin = width*16/*176/vwidth*/;
		viewheightwin =(int) (width*16*HEIGHTRATIO)/*176/vwidth*/;
		viewsize = width/*176/vwidth*/;
		
		viewwidth = width*16;
		viewheight =(int) (width*16*HEIGHTRATIO);
		if (viewheight%2) {
			viewheight+=1;
			viewheightwin+=1;
	}
	
	centerx = viewwidth/2-1;
	shootdelta = viewwidth/10;
	
	//yoffset = (vheight-STATUSLINES/*vheight/208*/-viewheight)/2;
	//xoffset = (vwidth-viewwidth)/2
	yoffset = 0;
	xoffset = 0;
	}
	yoffset=DRAW_GameTop;
	GameBottom=DRAW_GameTop+viewheight;
//
// calculate trace angles and projection constants
//
	CalcProjection(FOCALLENGTH);

}


/*
= InitGame
=
= Load a few things right away
*/

void InitVariable_main(void)
{
	radtoint = (double)FINEANGLES/2.0/PI;
	costable = sintable+(ANGLES/4);
}

// InitGame
void InitGame(int32 screen_w, int32 screen_h, int32 StartTileX, int32 StartTileY, int32 dir) {
    //MM_Startup(); 
	//页面文件
	//PM_Startup();
	//Cache缓冲文件
	//CA_Startup();
    // 初始化屏幕尺寸
    vwidth = screen_w;
    vheight = screen_h;
    
    gamestate.time_old=mrc_getUptime();
	gamestate.lives = 3;  //玩家初始生命3条
	gamestate.victoryflag = false;

    //获取屏幕缓冲地址方面信息
	mrc_bitmapGetInfo(30,&bitmapInfo);	
    
    InitVariable_play();
 	InitVariable_draw();
 	InitVariable_main();
 	InitVariable_act3();
 	InitVariable_Vi_comm();

	
	VW_Startup();
	//输入设备初始化
	IN_Startup();
	memset (buttonstate,0,sizeof(buttonstate));	
	//声音初始化
	//SD_Startup();
	//用户管理初始化
	//US_Startup();
    /*
    //平面地图瓦片偏移表
	for (i = 0;i < MAPSIZE; i++)
	{
		farmapylookup[i] = i*64;
	}
    */
    BuildTables();
    SetupWalls();
    NewViewSize(15); // 初始化视图尺寸
    
    lasttimecount = 0;	
	frameon = 0;
	anglefrac = 0;
    
    // 初始化玩家状态
    player = NULL;
    objfreelist = NULL;
    lastobj = NULL;
    memset(actorat, 0, sizeof(actorat));
    memset(tilemap, 0, sizeof(tilemap));
    
    InitActorList();
    SpawnPlayer(StartTileX, StartTileY, dir);
    
    //还需要给tilemap、actorat表赋值
}



void GameExit(void)//退出游戏前释放内存。
{
	//释放图片缓冲区内存空间
	//清除gamestate中保存的游戏状态。
	IN_Shutdown();//关闭键盘，清除键盘信息。
	mrc_memset(&gamestate,0,sizeof(gamestate));
}


/*目前未引用


//= 扫描信息平面
//= 生成所有角色并标记特殊地点


void ScanInfoPlane(void)
{
//这个函数功能是扫描活动物体平面，
//创建玩家。这里为了方便起见，在固定的位置创建一个玩家。
	if( (gamestate.player_tilex==0) && (gamestate.player_tiley==0) )
		SpawnPlayer(1,1, 1);
	else
		SpawnPlayer(gamestate.player_tilex,gamestate.player_tiley, 1);
}

void SetupGameLevel(void)
{
	int x,y;
	int32 len;
	
	if (!loadedgame) {
		gamestate.TimeCount = 0;
	}


	// load the level 
	//CA_CacheMap(gamestate.mapon+10*gamestate.episode);
	
	//if ((mapheaderseg[mapon]->width != 64) || (mapheaderseg[mapon]->height != 64))
	//	Quit("Map not 64*64!");

	//mapon -= gamestate.episode*10;
	
	memset(tilemap, 0, sizeof(tilemap));
	memset(actorat, 0, sizeof(actorat));

// copy the wall data to a data segment array 	
	//map = mapsegs[0];
	for (y = 0; y < MazeHeight; y++)
	{
		for (x = 0; x < MazeWidth; x++) 
		{
			if(*(pMap+y*MazeWidth+x)==0)
			{
				tilemap[y][x] = 0x80;
				actorat[y][x] = 0x80;
			}		
		}
	}

	for (y = 1; y < MazeHeight-1; y++)
	{
		for (x = 1; x < MazeWidth-1; x++) 
		{
			if(tilemap[y][x]&0x80)//是墙。
			{
				//if( (tilemap[y][x+1] &0x80) &&(tilemap[y][x-1] &0x80))//X轴水平方向的中间砖块
				//	continue;
				//tilemap[y][x] = 0x81;
				//actorat[y][x] = 0x81;	
				if( (tilemap[y][x+1] &0x80) ||(tilemap[y][x-1] &0x80))//X轴水平方向的尽头砖块，置为0x81
					continue;

				tilemap[y][x] = 0x82;//垂直X轴方向的砖块
				actorat[y][x] = 0x82;				
				//if((tilemap[y+1][x] &0x80) &&(tilemap[y-1][x] &0x80))
				//	continue;
				//tilemap[y][x] = 0x83;
				//actorat[y][x] = 0x83;
			}
		}
	}

	tilemap[MazeHeight-2][MazeWidth-1]=0x84;//
	tilemap[MazeHeight-1][MazeWidth-2]=0x84;//
	
	actorat[MazeHeight-2][MazeWidth-2]=EXITTILE;//
	
//	mrc_memset(pPic,0,sizeof(pPic));
//	pPic[0]=mrc_readFileFromMrp("pic_wall_0.bmp",&len,0);	//0x80
//	pPic[1]=mrc_readFileFromMrp("pic_wall_1.bmp",&len,0);	//0x81
//	pPic[2]=mrc_readFileFromMrp("pic_wall_2.bmp",&len,0);	//0x82
//	pPic[3]=mrc_readFileFromMrp("pic_wall_3.bmp",&len,0);	//0x83,end of maze.
//    pPic[4]=mrc_readFileFromMrp("pic_wall_4.bmp",&len,0);	//0x84 end
    
	//pPic[5]=mrc_readFileFromMrp("pic_good_1.bmp",&len,0);
	//pPic[6]=mrc_readFileFromMrp("pic_good_2.bmp",&len,0);
	//pPic[7]=mrc_readFileFromMrp("pic_good_3.bmp",&len,0);		//0x8F
	
		
//	for(y=0;y<8;y++)
//	{
//				tilemap[0][y] = 0x80;
//			 	 actorat[0][y] = 0x80;	

//				tilemap[8][y] = 0x80;
//			 	 actorat[8][y] = 0x80;		

//				tilemap[y][0] = 0x80;
//			 	 actorat[y][0] = 0x80;			

//				tilemap[y][7] = 0x80;
//			 	 actorat[y][7] = 0x80;					 	 
//	}

//	tilemap[5][0] = 0x80;
// 	 actorat[5][0] = 0x80;	
//	tilemap[5][1] = 0x80;
// 	 actorat[5][1] = 0x80;		
//	tilemap[5][2] = 0x80;
// 	 actorat[5][2] = 0x80;			
//	tilemap[5][3] = 0x80;
// 	 actorat[5][3] = 0x80;			 	 

		
	InitActorList();	// start spawning things with a clean slate 
	//InitDoorList();
	//InitStaticList();
	// spawn doors 
	// spawn actors 
	ScanInfoPlane();
}
*/