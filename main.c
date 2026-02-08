#include <math.h>
#include "mrc_base.h"
//#include "mrc_win.h"
//#include "mrc_menu.h"
//#include "mrc_text.h"
//#include "mrc_aux.h"
//#include "mrc_bmp.h"
#include "mpc.h"
#include "fontread.h"
#include "mrc_graphics.h"
//引擎
#include "WI_def.h"
#include "Vi_comm.h"
#include "main.h"
/* 全局变量定义（遵循功能机规则：不初始化，统一在mrc_init中赋值） */

enum {
WIN_MENU,
WIN_GAME,
WIN_GAMEOVER
};

/* 神庙逃亡核心变量 */


Obstacle g_obstacles[MAX_OBSTACLES];
boolean g_GameOver;
boolean g_is_jumping; // 是否跳跃中

uint8 g_RoadMap[MAPSIZE][MAPSIZE];
int32 g_StartTileX, g_StartTileY;  //可初始主角、生成道路、actorat[][]终点
int32 currentWindow;
int32 main_timer;
mrpfileSt player_sprite[4]; // 4帧奔跑动画
mrpfileSt obstacle_stone;
mrpfileSt obstacle_hole;
int32 player_sprite_idx; // 动画帧索引
int32 player_sprite_width; // 小人宽度
int32 player_sprite_height; // 小人高度
int32 g_jump_cnt; // 跳跃计时（持续10帧）


/* 函数声明（集中前置声明，避免隐式声明警告） */

void white_checkAiTimer(int32 data);

void GameOver_GoAgain(int32 data);

void white_gameOverTimer(int32 data);

void GameFunc_exit(void);
static void white_KeyHandler(int32 type, int32 p1, int32 p2);
void Game_start(void);

void GenerateTempleRoad(void);


void switchwindow(int32 win)
{
    currentWindow = win;
    
    if (currentWindow == WIN_GAME)
    {
        //定时器已经绘制页面了
    } else if (currentWindow == WIN_GAMEOVER)
    {
        mrc_EffSetCon(0,0,SCRW,SCRH,128,128,128);
        if (gamestate.victoryflag) //胜利
        {
            _drawText("游戏界面", 10, 10,  0, 0, 0, 0, 1);
        } else {
            //失败提示
            _drawText("游戏失败", 10, 10,  0, 0, 0, 0, 1);
        }
        mrc_refreshScreen(0, 0, 240, 320);
    }
}

void DrawObstacles(void) {
    int32 i, x, y;
    for (i = 0; i < MAX_OBSTACLES; i++) {
        if (g_obstacles[i].exists) {
            // 转换格子坐标到屏幕坐标（适配3D视角偏移）
            x = (g_obstacles[i].tilex - player->tilex) * 16 + 120;
            y = (g_obstacles[i].tiley - player->tiley) * 16 + 180;
            
            // 屏幕内才绘制
            if (x >= 0 && x < 240 - 16 && y >= 0 && y < 320 - 32) {
				// 障碍图片尺寸16x16
                mrc_bitmapShowEx(
                    g_obstacles[i].sprite.bitmap, x, y, 16, 16, 16, BM_TRANSPARENT,0, 0);
            }
        }
    }
}


// 在3D场景绘制后叠加小人（屏幕底部中央）
void DrawPlayerSprite(void) {
    int32 x, y;
    static int32 frame_cnt = 0;
    // 屏幕底部中央位置（适配240x320分辨率）
    x = (240 - player_sprite_width) / 2;
    y = 320 - player_sprite_height - 10; // 距底部10像素
    
    if (g_is_jumping) {
         y -= 10;
     }
    
    // 绘制当前帧动画
    mrc_bitmapShowEx(
        player_sprite[player_sprite_idx].bitmap,
        x, y,
        player_sprite_width, // 源图宽度
        player_sprite_width, // 绘制宽度
        player_sprite_height, // 绘制高度
        BM_TRANSPARENT, // 透明模式（忽略图片左上角透明色）
        0, 0 // 源图起始坐标
    );
    
    // 动画帧切换（每2帧切换一次，避免过快）    
    frame_cnt++;
    if (frame_cnt >= 2) {
        player_sprite_idx = (player_sprite_idx + 1) % 4;
        frame_cnt = 0;
    }
}


/* 神庙逃亡道路生成 */
void GenerateTempleRoad(void) {
//    uint8 x, y;
    int32 i, x, y;        
    
    // 定义四个角的坐标
    int32 left = g_StartTileX;
    int32 right = g_StartTileX + ROAD_LENGTH - 1;
    int32 bottom = g_StartTileY;
    int32 top = g_StartTileY + ROAD_LENGTH - 1;
    
    mrc_memset(g_RoadMap, 0, MAPSIZE * MAPSIZE * sizeof(uint8));
    
    // 1. 下边（从起点向右）
    for ( x = left; x <= right; x++) {
        for ( y = bottom; y < bottom + ROAD_WIDTH; y++) {
            g_RoadMap[y][x] = 1;
        }
    }
    
    // 2. 右边（从下向右上）
    for ( y = bottom; y <= top; y++) {
        for ( x = right - ROAD_WIDTH + 1; x <= right; x++) {
            g_RoadMap[y][x] = 1;
        }
    }
    
    // 3. 上边（从右上向左）
    for ( x = right; x >= left; x--) {
        for ( y = top - ROAD_WIDTH + 1; y <= top; y++) {
            g_RoadMap[y][x] = 1;
        }
    }
    
    // 4. 左边（从上向左下，回到起点）
    for ( y = top; y >= bottom; y--) {
        for ( x = left; x < left + ROAD_WIDTH; x++) {
            g_RoadMap[y][x] = 1;
        }
    }

    /* 随机外侧物、景观 */
    for (y = 1; y < MAPSIZE - 1; y++) {
        for (x = 1; x < MAPSIZE - 1; x++) {
            if (g_RoadMap[y][x] == 0 && (mrc_rand() % 5) < 3) {
                g_RoadMap[y][x] = 0x82;
            }
        }
    }
    
   
    //先把玩家从起点到终点隔开
    for(i = 0; i < 5; i++)
    {   
        g_RoadMap[g_StartTileY+7][g_StartTileX+i] = EXITTILE;
        g_RoadMap[g_StartTileY+6][g_StartTileX+i] = 0;
    }
        
    /* 同步到tilemap ，过不去的地方给actorat赋0x8开头的*/
    for (y = 0; y < MAPSIZE; y++) {
        for (x = 0; x < MAPSIZE; x++) {
            if (g_RoadMap[y][x] == 1) {
                tilemap[y][x] = 0;
            } else if (g_RoadMap[y][x] == 0x82) {
                tilemap[y][x] = 0x82;
                actorat[y][x] = 0x82;
            } else if (g_RoadMap[y][x] == EXITTILE) {
                tilemap[y][x] = EXITTILE;
            } else {
                tilemap[y][x] = 0x80;
                actorat[y][x] = 0x80;
            }
        }
    }
    
      
    
    // 初始化障碍
    
    mrc_memset(g_obstacles, 0, sizeof(g_obstacles));
    
    // 随机生成10个障碍（道路内随机位置）
    for (i = 0; i < 10; i++) {
        // 随机道路内格子（避开起点和转弯点）
        do {
            x = g_StartTileX + (mrc_rand() % ROAD_LENGTH);
            y = g_StartTileY + (mrc_rand() % ROAD_LENGTH);
        } while (
            (g_RoadMap[y][x] == EXITTILE) || // 避开起点
            (x > g_StartTileX + ROAD_LENGTH - 5 && y < g_StartTileY + 5) || // 避开转弯点
            (x < g_StartTileX + 5 && y < g_StartTileY + 5)
        );
        
        g_obstacles[i].tilex = x;
        g_obstacles[i].tiley = y;
        g_obstacles[i].exists = TRUE;
        
        // 随机障碍类型
        if (mrc_rand() % 2 == 0) {
            g_obstacles[i].type = OBSTACLE_STONE;
            g_obstacles[i].sprite = obstacle_stone;
        } else {
            g_obstacles[i].type = OBSTACLE_HOLE;
            g_obstacles[i].sprite = obstacle_hole;            
        }
        
        // 标记障碍位置（不影响道路通行，仅用于碰撞检测）
        actorat[y][x] = 0x90; // 自定义标记（非墙壁/道路）
    }
}


void GameFunc_exit() {

    if (main_timer) {
        mrc_timerStop(main_timer);
    }   
    GameExit();
}


void GameOver_GoAgain(int32 data) {

    GameFunc_exit();    
    mrc_clearScreen(0, 0, 0);   
    Game_start();
}

void white_gameOverTimer(int32 data) {
    GameFunc_exit();
    switchwindow(WIN_GAMEOVER);
}


void white_checkAiTimer(int32 data) {
    // 跳跃状态更新
    if (g_is_jumping) {
        g_jump_cnt++;
        // 跳跃持续10帧（约0.3秒）
        if (g_jump_cnt >= 10) {
            g_is_jumping = FALSE;
            g_jump_cnt = 0;
        }
    }
    
    PlayLoop();
    
    // 原有定时器逻辑...
    if (!g_GameOver) {
        mrc_timerStart(main_timer, 33, 0, white_checkAiTimer, 0);
    } else {
        mrc_timerStart(main_timer, 500, 0, white_gameOverTimer, 0);
    }
}




static void white_KeyHandler(int32 type, int32 p1, int32 p2) {
    int32 ItemType = 0, ItemCol = 0, ItemRow = 0;
    if (g_GameOver) return;

    if (type == MR_KEY_PRESS) {
        lasttimecount = mrc_getUptime();
        switch (p1) {
            case MR_KEY_UP:
                Keyboard[sc_UpArrow] = true;
                break;
            case MR_KEY_LEFT:
                Keyboard[sc_LeftArrow] = true;
                break;
            case MR_KEY_DOWN:
                Keyboard[sc_DownArrow] = true;
                break;
            case MR_KEY_RIGHT:
                Keyboard[sc_RightArrow] = true;
                break;
            case MR_KEY_SOFTLEFT:
                gamestate.IsShowMap = !gamestate.IsShowMap;
                break;
            case MR_KEY_SELECT:
                if (!g_is_jumping) {
                    g_is_jumping = TRUE;
                    g_jump_cnt = 0;
                }
                break;
            case MR_KEY_SOFTRIGHT:
//                IN_StartAck();                
                GameFunc_exit();
                mrc_exit();
                break;
            default:
                break;
        }
    }

    if (type == MR_KEY_RELEASE) {
        switch (p1) {
            case MR_KEY_UP:
                Keyboard[sc_UpArrow] = false;
                break;
            case MR_KEY_LEFT:
                Keyboard[sc_LeftArrow] = false;
                break;
            case MR_KEY_DOWN:
                Keyboard[sc_DownArrow] = false;
                break;
            case MR_KEY_RIGHT:
                Keyboard[sc_RightArrow] = false;
                break;
            case MR_KEY_SOFTLEFT:
                Keyboard[sc_Enter] = false;
                break;
            case MR_KEY_SELECT:
                Keyboard[sc_Enter] = false;
                break;
            case MR_KEY_SOFTRIGHT:
                break;
            default:
                break;
        }
    }

    if (type == MR_MOUSE_DOWN) {
		//white_MouseToItem(SCREENSTATUS_GAME, p1, p2, &ItemType, &ItemCol, &ItemRow) < 0
        
    }
}



void Game_start() {
    // 原有初始化逻辑...
    switchwindow(WIN_GAME);
    
	g_StartTileX = 3;
	g_StartTileY = 3;
    g_GameOver = FALSE;

	g_is_jumping = FALSE;
	g_jump_cnt = 0;
	player_sprite_idx = 0;

    //屏幕宽、高、玩家坐标X、Y、玩家面朝向(0、1、2、3)分别代表右、下、左、上
    InitGame(240, 320, g_StartTileX, g_StartTileY, 1);   
    GenerateTempleRoad();  //生成tilemap、actorat	
       
    mrc_timerStart(main_timer, 33, 0, white_checkAiTimer, 0);
}


/* 应用入口函数 */
int32 mrc_init(void) {
	mpc_init();
	player_sprite_width = 32; // 小人宽度
	player_sprite_height = 64; // 小人高度
    // 加载小人奔跑动画帧（4帧循环）
    loadmrpfile("player_run_0.bmp", &player_sprite[0]);
    loadmrpfile("player_run_1.bmp", &player_sprite[1]);
    loadmrpfile("player_run_2.bmp", &player_sprite[2]);
    loadmrpfile("player_run_3.bmp", &player_sprite[3]);
    loadmrpfile("stone.bmp", &obstacle_stone);
    loadmrpfile("hole.bmp", &obstacle_hole);
    
    mrc_clearScreen(0, 0, 0);

    main_timer = mrc_timerCreate();

    Game_start();
    return MR_SUCCESS;
}

int32 mrc_event(int32 code, int32 param0, int32 param1) {
    
    if (currentWindow == WIN_GAME)
    {
        white_KeyHandler(code, param0, param1);
    } else if (currentWindow == WIN_GAMEOVER) 
    {
        if (code == MR_KEY_DOWN)
        {
            if (param0 == MR_KEY_SOFTLEFT)
            {
            } else if (param0 == MR_KEY_SOFTRIGHT)
            {                
                mrc_exit();
            }
        }
        
    } else if (currentWindow == WIN_MENU)
    {
        if (code == MR_KEY_DOWN)
        {
            if (param0 == MR_KEY_SOFTLEFT)
            {
            } else if (param0 == MR_KEY_SOFTRIGHT)
            {                
                mrc_exit();
            }
        }
    }
    return MR_SUCCESS;
}

int32 mrc_pause(void) {
    return 0;
}

int32 mrc_resume(void) {
    return 0;
}

int32 mrc_exitApp(void) {
    if (main_timer) {
        mrc_timerStop(main_timer);
        mrc_timerDelete(main_timer);
    }
    GameFunc_exit();
    freemrpfile(&player_sprite[0]);
    freemrpfile(&player_sprite[1]);
    freemrpfile(&player_sprite[2]);
    freemrpfile(&player_sprite[3]);
    freemrpfile(&obstacle_stone);
    freemrpfile(&obstacle_hole);
	mpc_exit();
    return 0;
}

int32 mrc_extRecvAppEventEx(void) {
    return 0;
}

int32 mrc_extRecvAppEvent(void) {
    return 0;
}

