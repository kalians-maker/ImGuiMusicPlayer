#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/syscall.h>//包含了__NR_process_vm_readv对应的值，即270
#include <sys/mman.h>
#include <sys/uio.h>
#include <malloc.h>
#include <math.h>
#include <thread>
#include <iostream>
#include <sys/stat.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <locale>
#include <string>
#include <codecvt>
#include <dlfcn.h>
#include "TheColor.h"
// 添加系统调用号定义（Android ARM64）
#ifndef __NR_process_vm_readv
#define __NR_process_vm_readv 270
#endif

#ifndef __NR_process_vm_writev
#define __NR_process_vm_writev 271
#endif

// 定义这些常量
#define process_vm_readv_syscall __NR_process_vm_readv
#define process_vm_writev_syscall __NR_process_vm_writev
#define PI 3.141592653589793238

int 数组数量;
long int 数组入口;
long int 矩阵 = 88;
long int 模块地址;
long 自身结构;
long int 世界数组;
long int 数组跳板;
uintptr_t 自身坐标头;
static bool game_begin_bool = false;
bool game_text_bool = false;
bool game_text_bool2 = true;
float LineAns1 = 2.0f,LineAns2 = 2.0f;
int pid2 = -1;
float health_value = 1.0f;
int grenade_count;
int bullet_count;
bool Line_bool1 = true,Line_bool2 = true;
float matrix2[16];
ImColor LineForDread1 = ColorArray1[0];
ImColor LineForDread2 = ColorArray1[0];

// 自瞄相关变量定义
float XGX, XGY;  // 计算出的瞄准角度
float cx, cy, cz; // 与敌人的坐标差
float JD, XGX1, XGY1, JD1; // 角度和最佳角度
int zmclose = 1; // 自瞄开关
int fw = 1000; // 自瞄范围
int ab=0;
float wy;
float wx;
long int zmAddrx;

int getPID(const char *packageName)
{
	int id = -1;
	DIR *dir;
	FILE *fp;
	char filename[64];
	char cmdline[64];
	struct dirent *entry;
	dir = opendir("/proc");
	while ((entry = readdir(dir)) != NULL)
	{
		id = atoi(entry->d_name);
		if (id != 0)
		{
			sprintf(filename, "/proc/%d/cmdline", id);
			fp = fopen(filename, "r");
			if (fp)
			{
				fgets(cmdline, sizeof(cmdline), fp);
				fclose(fp);
				if (strcmp(packageName, cmdline) == 0)
				{
					return id;
				}
			}
		}
	}
	closedir(dir);
	return -1;
}
long getBss(int pid1,const char *name)
{
	FILE *fp;
	int cnt = 0;
	long start = 0;
	char tmp[256];
	char line[1024];
	char fname[128];
	sprintf(fname, "/proc/%d/maps", pid1);
	fp = fopen(fname, "r");
	if (!fp) {
		return 0; // 或者打印错误日志
	}
	while (fgets(tmp, sizeof(tmp), fp)) {
		if (cnt == 1) {
			if (strstr(tmp, "[anon:.bss]") != NULL) {
				sscanf(tmp, "%lx-%*lx", &start);
				break;
			} else {
				cnt = 0;
			}
		}
		if (strstr(tmp, name) != NULL) {
			cnt = 1;
		}
	}
	fclose(fp);
	return start;
}
ssize_t process_v2(pid_t __pid, const struct iovec *__local_iov, unsigned long __local_iov_count,const struct iovec *__remote_iov, unsigned long __remote_iov_count,unsigned long __flags, bool iswrite)
{
	return syscall((iswrite ? process_vm_writev_syscall : process_vm_readv_syscall), __pid,__local_iov, __local_iov_count, __remote_iov, __remote_iov_count, __flags);
}

bool pvm2(void *address, void *buffer, size_t size, bool iswrite,int pid)
{
	struct iovec local[1];
	struct iovec remote[1];
	local[0].iov_base = buffer;
	local[0].iov_len = size;
	remote[0].iov_base = address;
	remote[0].iov_len = size;
	if (pid < 0)
	{
		return false;
	}
	ssize_t bytes = process_v2(pid, local, 1, remote, 1, 0, iswrite);
	return bytes == size;
}
//读内存
bool vm_readv2(unsigned long address, void *buffer, size_t size,int pid)
{
	return pvm2(reinterpret_cast < void *>(address), buffer, size, false,pid);
}


// 写入内存
bool vm_writev2(unsigned long address, void *buffer, size_t size,int pid)
{
	return pvm2(reinterpret_cast < void *>(address), buffer, size, true,pid);
}


int getDword1(unsigned long addr)
{
	int var = 0;
	vm_readv2(addr, &var, 4,pid2);
	return (var);
}
float getFloat1(unsigned long addr)
{
	float var = 0;
	vm_readv2(addr, &var, 4,pid2);
	return (var);
}
long int getZZ(long int addr)
{
	long int var = 0;
	vm_readv2(addr, &var, sizeof(var),pid2);
	return var;
}

// 静态内联函数，性能更好，超级牛逼的方框，
static inline void DrawRectLines(ImDrawList* draw_list, float x, float top, float width, float bottom,ImU32 color, float thickness) {
    float x1 = x;
    float y1 = top;
    float x2 = x + width;
    float y2 = bottom;  
    draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y1), color, thickness);
    draw_list->AddLine(ImVec2(x2, y1), ImVec2(x2, y2), color, thickness);
    draw_list->AddLine(ImVec2(x2, y2), ImVec2(x1, y2), color, thickness);
    draw_list->AddLine(ImVec2(x1, y2), ImVec2(x1, y1), color, thickness);
}


//初始化数据
void DrawInt1() {

     pid2 = getPID("com.ShuiSha.FPS2");  
    if(pid2 < 0){
       
        game_text_bool = true;//文字提醒，未开游戏。
        
    }else{
              game_text_bool = false;            	       
              模块地址 = getBss(pid2,"libUE4.so");                
              世界数组 = getZZ(模块地址 + 0x29C900);
              自身结构 = getZZ(getZZ(getZZ(getZZ(模块地址 + 0x26CF58) + 0x8) + 0x270) + 0x110);              
              数组跳板 = getZZ(世界数组 + 0x30);              
              数组入口 = getZZ(数组跳板 + 0x98);                 
              自身坐标头 = getZZ(自身结构 + 0x130);                            
              数组数量 = getDword1(数组跳板 + 0x98 + 0x08);            
              矩阵 = getZZ(getZZ(模块地址 + 0x26CF58) + 0x20) + 0x280;               
              memset(matrix2, 0, sizeof(matrix2));//用零填充这个矩阵。   
              vm_readv2(矩阵, matrix2, 16 * 4,pid2);                           
              
              game_begin_bool = true;//这个变量为true时，绘制开启
            
    }
}
void 绘制遍历(){
   if(game_begin_bool){ 
            auto draw_list = ImGui::GetForegroundDrawList();
            float  the_px = displayInfo.width/2.0f;//屏幕中心x
            float the_py = displayInfo.height/2.0f;//  屏幕中心y   
                
            for( int i = 0;i < 数组数量;i++){                                                                 
                uintptr_t obj1 = getZZ(数组入口 + 8 * i);                                   
                uintptr_t head1 = getZZ(obj1 + 0x130);                                                              
                uintptr_t enemy_health_addr = getZZ(obj1 + 0x4C0) + 0xB0;//健康值地址
                float hp = getFloat1(enemy_health_addr);//获取健康值                             
                float 敌人x = getFloat1(head1 + 0x1D0);
                float 敌人y = getFloat1(head1 + 0x1D4);
                float 敌人z = getFloat1(head1 + 0x1D8); 
                float self_x = getFloat1(自身坐标头 + 0x1D0);
                float self_y = getFloat1(自身坐标头 + 0x1D4);
                float self_z = getFloat1(自身坐标头 + 0x1D8);         
                                                            
                float camera = matrix2[3] * 敌人x  + matrix2[7] * 敌人y + matrix2[11] * 敌人z + matrix2[15];                
                float r_x = the_px + (matrix2[0] * 敌人x  + matrix2[4] * 敌人y + matrix2[8] * 敌人z + matrix2[12]) / camera * the_px;
                float r_y = the_py - (matrix2[1] * 敌人x  + matrix2[5] * 敌人y + matrix2[9] * (敌人z - 5) + matrix2[13]) / camera * the_py;
                float r_w = the_py - (matrix2[1] * 敌人x  + matrix2[5] * 敌人y + matrix2[9] * (敌人z + 205)+ matrix2[13]) / camera * the_py;                
                               
                float X = r_x - (r_y - r_w) / 4.0f;
                float Y = r_y;
                float W = (r_y - r_w) / 2.0f;
                float top = Y - W;
                float bottom = Y + W;
                float mid = X + W/2;
                float left = mid -(W/2);
                float right = mid +(W/2);
                
                //各种过滤
                if (obj1 == 0) continue;
                if (head1 == 0) continue;                
                if (getFloat1(obj1 + 0x22C) != 64.0f) continue;                               
                if (hp <= 0.0f) continue;//健康值小于等于零忽略。                                              
                if(敌人x == 0&& 敌人y == 0&& 敌人z == 0) continue;//循环跳过。
                if (camera <= 0.0f) continue;
                if(敌人x == self_x && 敌人y ==self_y && 敌人z == self_z) continue;                                                                                                                                                   
                if(W < 0.0f) continue;             
                
                      
                    if(Line_bool1)               
                        draw_list->AddLine(ImVec2(the_px, 15), ImVec2(r_x, top), LineForDread1, LineAns1);
                    if(Line_bool2)    
                       // draw_list->AddRect(ImVec2(X, top), ImVec2(X + W, bottom), ImColor(0, 255, 35), 0.0f, LineAns2);
                        DrawRectLines(draw_list, X, top, W, bottom, LineForDread2, LineAns2);
                                            
                     // 自瞄计算核心逻辑（在遍历敌人循环中）
                   float zxjl = sqrt(pow(r_x - the_px, 2) + pow(r_y - the_py, 2));
                  
                    if (zxjl<=fw) {  // 敌人在自瞄范围内
                        // 获取视角内存地址
                        zmAddrx = getZZ(getZZ(getZZ(模块地址 + 0xAD9A000 + 0x276080) + 0x30) + 0x250) + 0x6F0;
                        wx = getFloat1(zmAddrx);     // 当前X轴视角
                        wy = getFloat1(zmAddrx + 0x4); // 当前Y轴视角
  
                        // 计算坐标差
                        cx = 敌人x - self_x;  // 敌人X - 自己X
                        cy = 敌人y - self_y;  // 敌人Y - 自己Y
                        cz = 敌人z - self_z;  // 敌人Z - 自己Z
                        
                        // 计算需要瞄准的角度（数学公式）
                        XGX = (float)atan2(cy, cx) * 180 / PI;  // 水平角度
                        XGY = (float)atan2(cz, sqrt((cx * cx) + (cy * cy))) * 180 / PI + 0.4; // 垂直角度
                        
                        // 角度规范化（0-360度）
                        if (XGX > -180 && XGX < 0) {
                            XGX = XGX + 360;
                        }
                        
                        // 计算角度差，选择最近的目标
                        JD = wx - XGX;
                        JD = sqrt(JD * JD);  // 取绝对值
                        
                        if (ab == 0) {  // 初始化
                            ab = 1;
                            JD1 = JD + 1;
                        }
                        
                        // 选择角度差最小的敌人（最接近准心的）
                        if (JD < JD1 && ab == 1) {
                            JD1 = JD;
                            XGX1 = XGX;  // 保存最佳水平角度
                            XGY1 = XGY;  // 保存最佳垂直角度
                            ab = 1;
                        }
                        
                        // 如果自瞄开关打开，写入角度数据
                        if (zmclose == 1) {
                          //  WriteFloat(zmAddrx, XGY1);    // 写入Y轴（垂直）角度
                            vm_writev2(zmAddrx, &XGY1, sizeof(int),pid2);
                         //   WriteFloat(zmAddrx + 0x4, XGX1); // 写入X轴（水平）角度
                            vm_writev2(zmAddrx+ 0x4, &XGX1, sizeof(int),pid2);
                        }
                    }//敌人在视角内分号结束        
                        
                        
                        
                        
                        
                        
                        
                        
                        
            }//遍历内容结束分号        
   }    
}
void InfiniteAmmo(){
    uintptr_t bullet_addr = getZZ(自身结构 + 0x750) + 0x1440;
    
    vm_writev2(bullet_addr, &bullet_count, sizeof(int),pid2);
}
void UnlimitedGrenades(){
    uintptr_t grenade_addr = getZZ(自身结构 + 0x1440) + 0x4;
   
    vm_writev2(grenade_addr, &grenade_count, sizeof(int),pid2);
}
void UnlimitedHealth(){
    uintptr_t self_health_addr = getZZ(自身结构 + 0x4C0) + 0xB0;
  
    vm_writev2(self_health_addr, &health_value, sizeof(float),pid2);
}



// //传送相关函数
// void WriteSelfPosition(float x, float y, float z) {
    // if (自身结构 == 0) return;
    // uintptr_t head = getPtr64(自身结构 + 0x130);
    // if (head == 0) return;
    // vm_writev(head + 0x1D0, &x, sizeof(float));
    // vm_writev(head + 0x1D4, &y, sizeof(float));
    // vm_writev(head + 0x1D8, &z, sizeof(float));
// }












































































