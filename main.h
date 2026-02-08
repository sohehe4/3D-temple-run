#ifndef __MAIN_H__
#define __MAIN_H__

#include "mpc.h"
//3D引擎是MyWolf文件夹
#include "WI_def.h"

#define ROAD_WIDTH 6
#define ROAD_LENGTH 56  // 注意MAPSIZE定义了64

#define MAX_OBSTACLES 20 // 最大障碍数

// 障碍类型
typedef enum {
	OBSTACLE_STONE, // 石头（碰撞减血）
	OBSTACLE_HOLE  // 坑洞（碰撞减血）
} ObstacleType;

// 障碍结构体
typedef struct {
	int32 tilex, tiley; // 所在格子坐标
	ObstacleType type;
	boolean exists; // 是否存在
	mrpfileSt sprite; // 障碍图片
} Obstacle;

extern Obstacle g_obstacles[MAX_OBSTACLES];
extern boolean g_GameOver;
extern boolean g_is_jumping; // 是否跳跃中

extern void DrawPlayerSprite(void);
extern void DrawObstacles(void);

#endif

/*
地图长宽tile个
#define MAPSIZE		64

main.c使用引擎
void InitGame(int32 screen_w, int32 screen_h, int32 StartTileX, int32 StartTileY, int32 dir);
void PlayLoop (void);
void GameExit(void);//退出游戏前释放内存


tilemap数组

0x80   - 默认墙壁（内墙)
0x82   - 特殊墙壁（周边墙）
0x84   - 出口附近

99     - EXITTILE（出口）



actorat数组  // trymove()移动规则0x8开头的过不去

0x90   - 障碍物，不影响通行，会掉HP

0x80/0x82 - 墙壁
0x8000  - 活体
*/