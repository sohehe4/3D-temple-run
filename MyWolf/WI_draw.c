#include "WI_def.h"
#include "WI_act3.h"
#include "main.h"
#ifdef SDK_MOD
#include "assert.h"
#define Myassert(a)  assert(a)
#else
#define Myassert(a) 
#endif

/* C AsmRefresh() and related code
   originally from David Haslam -- dch@sirius.demon.co.uk */

#define ACTORSIZE	0x4000

static unsigned wallheight[MAXVIEWWIDTH];

/* refresh variables */
static int viewangle;

static unsigned tilehit;

static int xtile, ytile;
static int xtilestep, ytilestep;
static long xintercept, yintercept;

static unsigned postx;

static fixed focallength;
static fixed scale;
static long heightnumerator;
uint32 GameBottom;

static void AsmRefresh(void);
static void drawTop(void);
static void drawBottom(void);
static void drawMap(void);

 
static double radtoint;

// 添加外部变量声明


/*
 
= CalcProjection
 
*/

void CalcProjection(long focal)
{
	int     i;
	long    intang;
	double angle, tang, facedist;
	int     halfview;

	focallength = focal;
	facedist = focal+MINDIST;
	halfview = viewwidth/2;

	scale = halfview*facedist/(VIEWGLOBAL/2);

	heightnumerator = (TILEGLOBAL*scale)>>6;

	for (i = 0; i < halfview; i++)
	{
		tang = ((double)i)*VIEWGLOBAL/viewwidth/facedist;
		angle = atan(tang);
		intang = angle*radtoint;
		pixelangle[halfview-1-i] = intang;
		pixelangle[halfview+i] = -intang;
	}
}

 

/*
= 
= CalcRotate
 =
*/

static const int dirangle[9] = {0,ANGLES/8,2*ANGLES/8,3*ANGLES/8,4*ANGLES/8, 5*ANGLES/8,6*ANGLES/8,7*ANGLES/8,ANGLES};

static int CalcRotate(objtype *ob)
{
	int	angle,calc_viewangle;

	calc_viewangle = player->angle + (centerx - ob->viewx)/8;

	if (ob->obclass == rocketobj || ob->obclass == hrocketobj)
		angle =  (calc_viewangle-180)- ob->angle;
	else
		angle =  (calc_viewangle-180)- dirangle[ob->dir];

	angle+=ANGLES/16;
	while (angle>=ANGLES)
		angle-=ANGLES;
	while (angle<0)
		angle+=ANGLES;

	if (gamestates[ob->state].rotate == 2)
		return 4*(angle/(ANGLES/2));

	return angle/(ANGLES/8);
}

 

#define MAXVISABLE      64

typedef struct {
	int viewx;
	int viewheight;
	int shapenum;
} visobj_t;

static visobj_t vislist[MAXVISABLE], *visptr, *visstep, *farthest;

 

static int weaponscale[NUMWEAPONS] ;

/*
 
= WallRefresh
 
*/

static void WallRefresh(void)
{
	viewangle = player->angle;
	
	viewsin = sintable[viewangle];
	viewcos = costable[viewangle];
	
	// 神庙逃亡：焦点在玩家前方，第三人称视角
	// 注意：需要根据实际坐标系调整符号
	viewx = player->x + FixedByFrac(0x1800, viewcos);
	viewy = player->y - FixedByFrac(0x1800, viewsin);

	AsmRefresh();
}

 

#define	MAXVIEWHEIGHT	(MAXVIEWWIDTH/2)

static int spanstart[MAXVIEWHEIGHT/2];
static fixed basedist[MAXVIEWHEIGHT/2];
static byte planepics[8192];
static int halfheight = 0;
static byte *planeylookup[MAXVIEWHEIGHT/2];
static unsigned	mirrorofs[MAXVIEWHEIGHT/2];

static int mr_rowofs;
static int mr_count;
static unsigned short int mr_xstep;
static unsigned short int mr_ystep;
static unsigned short int mr_xfrac;
static unsigned short int mr_yfrac;
static byte *mr_dest;

 

static unsigned int Ceiling[1];

/*
= 
= ClearScreen
 =
*/

static void ClearScreen(void)
{
	//unsigned int ceiling = Ceiling[gamestate.episode*10+mapon] & 0xFF;
	unsigned int ceiling = Ceiling[0];
	unsigned int floor = 0x19;

	//mrc_drawRect(xoffset,yoffset,viewwidth,viewheight/2,DRAW_SKY_rgb);
	mrc_memset(bitmapInfo.p+viewwidth*yoffset,DRAW_SKY_rgb16,viewwidth*viewheight/2*sizeof(uint16));
	//VL_Bar(xoffset, yoffset, viewwidth, viewheight/2, ceiling);//设置为天空或者说天花板的颜色
	//mrc_drawRect(xoffset,yoffset + viewheight / 2,viewwidth,viewheight/2,DRAW_FLOOR_rgb);
	mrc_memset(bitmapInfo.p+viewwidth*(yoffset + viewheight / 2),DRAW_FLOOR_rgb16,viewwidth*viewheight/2*sizeof(uint16));
	//VL_Bar(xoffset, yoffset + viewheight / 2, viewwidth, viewheight/2, floor);//设置为地板的颜色。
}

 

/*
==== 
= ThreeDRefresh
 ====
*/

#ifndef DRAWCEIL
 /* #define DRAWCEIL */
#endif
 
void ThreeDRefresh(void)
{
	memset(spotvis, 0, sizeof(spotvis));
	
	ClearScreen();	

	WallRefresh();
	
	drawTop();
	
	
	DrawObstacles();	
	
	drawBottom();

	DrawPlayerSprite();

	if(gamestate.IsShowMap)
	{
		drawMap();
	}
	
	VW_UpdateScreen();
	
	frameon++;
}

 

static void ScaledDraw(uint16 *gfx, int count, uint16 *vid, unsigned int frac, unsigned int delta)
{
	while (count--) {
		*vid = gfx[frac >> 16];
		vid += vwidth;
		frac += delta;
	}
}

static void ScaleLine(unsigned int height, uint16 color, int x)
{
	unsigned int y, temp1, temp2, temp3;
	unsigned int count;
	
	if (height) 
	{
		if (height < viewheight) 
		{
			y = yoffset + (viewheight - height) / 2;			
			count = height;
			temp1 = y + height;
			temp2 = x + xoffset;
			count = y;
			temp3 = vwidth;
			
			// 绘制单色墙
			while(count < temp1)
			{
				if (count >= 0 && count < vheight)
				{
					*(bitmapInfo.p + count*temp3 + temp2) = color;
				}
				count++;
			}
			return;	
		} 
		else
		{
			// 高度超过屏幕，绘制全屏
			for (y = 0; y < viewheight; y++)
			{
				if ((yoffset+y) >= 0 && (yoffset+y) < vheight)
				{
					*(bitmapInfo.p + (yoffset+y)*vwidth + x + xoffset) = color;
				}
			}
		}
	}
}

 

/*
 
= CalcHeight
=
= Calculates the height of xintercept,yintercept from viewx,viewy
 
*/

static int CalcHeight(void)
{
	fixed gxt,gyt,nx,gx,gy;

	gx = xintercept - viewx;
	gxt = FixedByFrac(gx, viewcos);

	gy = yintercept - viewy;
	gyt = FixedByFrac(gy, viewsin);

	nx = gxt-gyt;

	if (nx < MINDIST)
		nx = MINDIST;
	
	return heightnumerator/(nx>>8);
}

static void ScalePost(int is_vert_wall)
{
	int height, distance_value;
	uint16 wall_color;
	
	// 计算墙高度>>2
	height = (wallheight[postx] & 0xFFF8) >> 2;
	
	if (height <= 0)
		return;
	
	// 根据距离确定墙颜色（近亮远暗）
	// wallheight[postx] 保存的是 CalcHeight() 的返回值
	// 这个值越大表示墙越远，越小表示墙越近
	distance_value = wallheight[postx];
	
	// 障碍物使用特殊颜色
	if (tilehit == 0x90)
	{
		// 障碍物统一用红色（简单处理）
		wall_color = 0xF800;  // 红色
	}
	else
	{
		// 墙壁颜色（根据"距离值"）
		// 注意：wallheight 值越小表示距离越近
		if (distance_value > 2000)  // 远距离
			wall_color = 0x52AA;  // 暗灰色
		else if (distance_value > 1000)  // 中距离
			wall_color = 0x7BEF;  // 灰色
		else  // 近距离
			wall_color = 0x9CF3;  // 亮灰色
	}
	
	// 绘制墙
	ScaleLine(height, wall_color, postx);
}

static void HitVertWall(void)
{
	unsigned texture;
	
	texture = (yintercept>>4)&0xfc0;
	
	if (xtilestep == -1) {
		texture = 0xfc0-texture;
		xintercept += TILEGLOBAL;
	}
	
	// 计算墙高度
	wallheight[postx] = CalcHeight();
	
	// 绘制墙（纯色）
	ScalePost(1);
}

static void HitHorizWall(void)
{
	unsigned texture;
	
	texture = (xintercept >> 4) & 0xfc0;
	
	if (ytilestep == -1)
		yintercept += TILEGLOBAL;
	else
		texture = 0xfc0 - texture;
		
	wallheight[postx] = CalcHeight();
	
	// 绘制墙（纯色）
	ScalePost(0);
}

#define DEG90	900
#define DEG180	1800
#define DEG270	2700
#define DEG360	3600

static int samex(int intercept, int tile)
{
	if (xtilestep > 0) {
		if ((intercept>>TILESHIFT) >= tile)
			return 0;
		else
			return 1;
	} else {
		if ((intercept>>TILESHIFT) <= tile)
			return 0;
		else
			return 1;
	}
}

static int samey(int intercept, int tile)
{
	if (ytilestep > 0) 
	{
		if ((intercept>>TILESHIFT) >= tile)
			return 0;
		else
			return 1;
	} 
	else 
	{
		if ((intercept>>TILESHIFT) <= tile)
			return 0;
		else
			return 1;
	}
}

static void GetPassTime(void)
{
	int32 curtime,timepass,temp;
	curtime=mrc_getUptime();
	timepass=curtime-gamestate.time_old;
	if(timepass<0 )
	{
		gamestate.time_old=curtime;
		return;
	}
	gamestate.time_old=curtime;
	temp=timepass/(1000*60);
	timepass=timepass%(1000*60);
	gamestate.time_minute+=temp;
	temp=timepass/(1000);
	timepass=timepass%(1000);	
	gamestate.time_second+=temp;
	temp=timepass/(100);
	gamestate.time_100milsec+=temp;
	temp=timepass%(100);
	temp=gamestate.time_old-temp;
	if(temp>0)
		gamestate.time_old=temp;

	//处理进位
	if(gamestate.time_100milsec>9)
	{
		gamestate.time_second++;
		gamestate.time_100milsec-=10;			
	}
	if(gamestate.time_second>59)
	{
		gamestate.time_minute++;
		gamestate.time_second-=60;			
	}			
}

// 顶部UI绘制（时间+标题+生命值）
static void drawTop(void) {
    char bufout[70], buftemp[20];
    char hp_buf[20];
    bufout[0] = 0;

    // 绘制顶部背景栏（上半部分：浅蓝色，下半部分：浅灰色边框）
    mrc_drawRect(0, 0, 240, DRAW_GameTop - 3, 173, 216, 230);  // 浅蓝色 RGB  
    mrc_drawRect(0, DRAW_GameTop - 3, 240, 3, 128, 128, 128);  // 深灰色 RGB   
    mrc_drawRect(120, 0, 1, DRAW_GameTop - 4, 128, 128, 128);  // 深灰色分隔线

    // 计算并显示游戏时间（分钟:秒:0.1秒）
    if (!gamestate.victoryflag) {
        GetPassTime();
    }
    mrc_sprintf(bufout, "%2d:%02d:%02d", 
            gamestate.time_minute,
            gamestate.time_second,
            gamestate.time_100milsec);

    // 绘制标题、时间、生命值
    mrc_drawText("神庙逃亡", 4, 3, 255, 255, 255, 0, MR_FONT_BIG);  // 标题（白色）
    mrc_drawText(bufout, 100, 3, 255, 255, 255, 0, MR_FONT_BIG);   // 时间（白色）
    mrc_sprintf(hp_buf, "HP:%d", gamestate.lives);
    mrc_drawText(hp_buf, 200, 3, 255, 0, 0, 0, MR_FONT_BIG);       // 生命值（红色）
}

// 底部UI绘制（功能提示栏）
static void drawBottom(void) {
    // 绘制底部背景栏（上半部分：浅灰色边框，下半部分：浅蓝色）
    mrc_drawRect(0, GameBottom, 240, 3, 128, 128, 128);  // 深灰色 RGB
    
    mrc_drawRect(0, GameBottom + 3, 240, 320 - GameBottom - 3, 173, 216, 230);  // 浅蓝色 RGB
    // 中间分隔
    mrc_drawRect(120, GameBottom + 3, 1, 320 - GameBottom - 3, 128, 128, 128);  // 深灰色分隔线

    mrc_drawText("暂停", 0, GameBottom + 10, 255, 255, 255, 0, MR_FONT_BIG);  // 右下角显示"暂停"
}



static void drawMap(void)
{
    int32 row, col, MazeHeight2, vwidth2;
    int playerCenterX, playerCenterY;      // 小人在小地图的中心位置
    int dirX, dirY;                        // 方向分量
    int endX, endY;                        // 方向指示点位置
    int mapX, mapY;                        // 地图坐标
    int mapSize = MAPSIZE << 1;            // 小地图大小（每个格子2像素）
    
    // 小地图位置计算
    MazeHeight2 = GameBottom - mapSize;
    vwidth2 = (vwidth >> 1) - MAPSIZE;
    
    // 清空小地图区域（绘制黑色背景）
    mrc_drawRect(vwidth2, MazeHeight2, mapSize, mapSize, 0, 0, 0);  // RGB: 黑色
    
    // 绘制墙壁
    for(row = MAPSIZE-1; row >= 0; row--)
    {
        for(col = MAPSIZE-1; col >= 0; col--)
        {
            if(actorat[row][col] & 0x80)
            {
                // 计算小地图坐标（Y轴翻转）
                mapX = vwidth2 + (col << 1);
                mapY = MazeHeight2 + ((MAPSIZE-1 - row) << 1);  // Y轴翻转
                
                // 使用 mrc_drawRect 绘制2x2的墙壁块（灰色）
                mrc_drawRect(mapX, mapY, 2, 2, 120, 120, 120);  // RGB: 灰色
            }
        }
    }
    
    // === 绘制带精确方向的小人 ===
    
    // 1. 计算小人在小地图上的中心位置
    // Y坐标需要翻转：使用 (MAPSIZE-1 - player->tiley)
    playerCenterX = vwidth2 + (player->tilex << 1);
    playerCenterY = MazeHeight2 + ((MAPSIZE-1 - player->tiley) << 1);  // Y轴翻转
    
    // 2. 绘制小人（白色方块，2x2像素）
    mrc_drawRect(playerCenterX, playerCenterY, 2, 2, 255, 255, 255);  // RGB: 白色
    
    // 3. 使用 costable 和 sintable 计算方向
    #define DIRECTION_LENGTH 5
    
    // 计算方向分量
    // 注意：小地图Y轴已经翻转，所以方向向量的Y分量也要取反
    dirX = (costable[player->angle] * DIRECTION_LENGTH) >> 16;
    dirY = -((-sintable[player->angle] * DIRECTION_LENGTH) >> 16);  // 取反，因为Y轴翻转了
    
    // 4. 计算终点坐标
    endX = playerCenterX + dirX;
    endY = playerCenterY + dirY;  // 注意：这里加dirY，因为Y轴已经翻转
    
    // 5. 绘制方向指示点（红色方块，2x2像素）
    mrc_drawRect(endX, endY, 2, 2, 255, 0, 0);  // RGB: 红色
    
    // 可选：绘制小地图边框（浅灰色）
    mrc_drawRect(vwidth2-1, MazeHeight2-1, mapSize+2, mapSize+2, 200, 200, 200);  // RGB: 浅灰色边框
}

void InitVariable_draw(void)
{
	radtoint = (double)FINEANGLES/2.0/PI;
	
	weaponscale[0] = SPR_KNIFEREADY;
	weaponscale[1] = SPR_PISTOLREADY;
	weaponscale[2] = SPR_MACHINEGUNREADY;
	weaponscale[3] = SPR_CHAINREADY;

	Ceiling[0]=0xfd8;
}

static void AsmRefresh(void)
{
	unsigned xpartialup, xpartialdown, ypartialup, ypartialdown;
	unsigned xpartial, ypartial;
	int angle;
	int midangle;
	int focaltx, focalty;
	int xstep, ystep;
	int tmptitle;
	int32 EnterTime,Leavetime,TimePassed,TimeTotal,FirstEnterTime;
	
	midangle = viewangle*(FINEANGLES/ANGLES);
	xpartialdown = (viewx&(TILEGLOBAL-1));
	xpartialup = TILEGLOBAL-xpartialdown;
	ypartialdown = (viewy&(TILEGLOBAL-1));
	ypartialup = TILEGLOBAL-ypartialdown;

	focaltx = viewx>>TILESHIFT;
	focalty = viewy>>TILESHIFT;
			
	for (postx = 0; postx < viewwidth; postx++) 
	{
				
		angle = midangle + pixelangle[postx];

		if (angle < 0) 
		{
			angle += FINEANGLES;
			goto entry360;
		} 
		else if (angle < DEG90) 
		{
		entry90:
			xtilestep = 1;
			ytilestep = -1;
			xstep = finetangent[DEG90-1-angle];
			ystep = -finetangent[angle];
			xpartial = xpartialup;
			ypartial = ypartialdown;
		} 
		else if (angle < DEG180) 
		{
			xtilestep = -1;
			ytilestep = -1;
			xstep = -finetangent[angle-DEG90];
			ystep = -finetangent[DEG180-1-angle];
			xpartial = xpartialdown;
			ypartial = ypartialdown;
		} 
		else if (angle < DEG270) 
		{
			xtilestep = -1;
			ytilestep = 1;
			xstep = -finetangent[DEG270-1-angle];
			ystep = finetangent[angle-DEG180];
			xpartial = xpartialdown;
			ypartial = ypartialup;
		} 
		else if (angle < DEG360) 
		{
		entry360:
			xtilestep = 1;
			ytilestep = 1;
			xstep = finetangent[angle-DEG270];
			ystep = finetangent[DEG360-1-angle];
			xpartial = xpartialup;
			ypartial = ypartialup;
		}
		else
		{
			angle -= FINEANGLES;
			goto entry90;
		}
		
		yintercept = viewy + FixedByFrac(xpartial, ystep);
		xtile = focaltx + xtilestep;
		xintercept = viewx + FixedByFrac(ypartial, xstep);
		ytile = focalty + ytilestep;

/* CORE LOOP */

#define TILE(n) ((n)>>TILESHIFT)

		/* check intersections with vertical walls */
vertcheck:
		if (!samey(yintercept, ytile))
			goto horizentry;
			
vertentry:
		tilehit = tilemap[xtile][TILE(yintercept)];
		
		if (tilehit)
		{
			xintercept = xtile << TILESHIFT;
			HitVertWall();
			
			continue;
		}
passvert:
		tmptitle=TILE(yintercept);
		spotvis[xtile][tmptitle] = 1;
		xtile += xtilestep;
		yintercept += ystep;
		goto vertcheck;
		
horizcheck:
		/* check intersections with horizontal walls */
		
		if (!samex(xintercept, xtile))
			goto vertentry;

horizentry:
		tilehit = tilemap[TILE(xintercept)][ytile];
		
		if (tilehit) 
		{
			yintercept = ytile << TILESHIFT;
			HitHorizWall();
			
			continue;
		}
passhoriz:
		tmptitle=TILE(xintercept);
		spotvis[tmptitle][ytile] = 1;
		ytile += ytilestep;
		xintercept += xstep;
		goto horizcheck;
	}
		
}