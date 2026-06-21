// 必须在包含 miniaudio.h 之前定义此宏，代表在当前源文件中生成 miniaudio 的全部底层实现代码
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "MusicManager.h"
#include <mutex>       // 引入互斥锁，用于解决 UI 线程与底层音频渲染线程的并发冲突
#include <dirent.h>    // 引入 POSIX 标准目录操作库，用于扫描手机文件夹
#include <cstring>

namespace Music {
    // ==========================================
    // 静态全局变量定义（通过 static 限制作用域，仅限本文件访问）
    // ==========================================
    static ma_engine g_AudioEngine;       // miniaudio核心：音频引擎对象（相当于虚拟声卡、混音台）
    static ma_sound g_CurrentSound;       // miniaudio核心：音轨/声音对象（相当于当前磁带）
    
    static bool g_IsEngineInit = false;   // 标记音频引擎是否已经成功初始化
    static bool g_IsSoundLoaded = false;  // 标记当前是否有歌曲成功载入到了 g_CurrentSound 中
    static std::string g_CurrentTitle = "未在播放"; // 存储当前正在播放的歌曲名称
    
    // 关键核心：互斥锁。Android UI/绘图是在主线程，而 miniaudio 的声音解码和写入设备是在一条高优先级的底层音频独立线程。
    // 当你在 UI 上点击切歌（主线程修改 g_CurrentSound），底层线程可能刚好在读取它，不加锁会导致程序随机崩溃。
    static std::mutex g_AudioMutex;

    static std::string g_CurrentFolderPath = ""; // 当前正在扫描的文件夹绝对路径
    static std::vector<std::string> g_SongFiles; // 存放扫描出来的歌曲文件名集合（歌单列表）

    // ==========================================
    // 核心功能实现
    // ==========================================

    bool Init() {
        // 自动管理互斥锁：进入函数自动上锁，函数执行完毕（遇到 return 或结束大括号）自动解锁
        std::lock_guard<std::mutex> lock(g_AudioMutex);
        
        // 如果已经初始化过了，无需重复操作，直接返回成功
        if (g_IsEngineInit) return true;

        // 生成一套 miniaudio 默认的高性能引擎配置参数
        ma_engine_config config = ma_engine_config_init();
        
        // 初始化引擎。在 Android 平台上，miniaudio 会自动按顺序尝试 AAudio 和 OpenSL ES 这两种底层低延迟驱动
        if (ma_engine_init(&config, &g_AudioEngine) != MA_SUCCESS) {
            return false; // 初始化失败（可能由于音频设备被系统其他应用独占）
        }
        
        g_IsEngineInit = true; // 标记初始化成功
        return true;
    }

    void Shutdown() {
        std::lock_guard<std::mutex> lock(g_AudioMutex);
        
        // 如果当前有歌曲挂载，先将其卸载，回收占用的音频流内存
        if (g_IsSoundLoaded) {
            ma_sound_uninit(&g_CurrentSound);
            g_IsSoundLoaded = false;
        }
        
        // 销毁并关闭底层的音频引擎，释放硬件设备控制权
        if (g_IsEngineInit) {
            ma_engine_uninit(&g_AudioEngine);
            g_IsEngineInit = false;
        }
    }

    bool PlayFile(const std::string& filePath) {
        std::lock_guard<std::mutex> lock(g_AudioMutex);
        if (!g_IsEngineInit) return false; // 引擎都没开，拒绝播放

        // 【安全机制】如果要播新歌，必须先将上一首正在播放的歌曲安全地从引擎中卸载
        if (g_IsSoundLoaded) {
            ma_sound_uninit(&g_CurrentSound);
            g_IsSoundLoaded = false;
        }

        // 初始化并加载新歌曲文件
        // 核心参数 MA_SOUND_FLAG_STREAM：开启“流式加载”。
        // 原理：不把整首几兆甚至几十兆的 MP3 全部一次性解压到 RAM，而是边播放边从手机闪存里读一小段解码一小段。
        // 这对 Android 悬浮窗应用极为重要，可以避免因内存占用过高被系统强杀（OOM）。
        ma_result result = ma_sound_init_from_file(
            &g_AudioEngine, 
            filePath.c_str(), 
            MA_SOUND_FLAG_STREAM, 
            NULL, 
            NULL, 
            &g_CurrentSound
        );

        if (result != MA_SUCCESS) {
            g_CurrentTitle = "播放失败";
            return false; // 文件路径错误、损坏或格式不支持
        }

        g_IsSoundLoaded = true; // 歌曲挂载成功
        
        // 从长长的绝对路径中截取最后的“文件名.mp3”用来当作 UI 显示的标题
        size_t lastSlash = filePath.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            g_CurrentTitle = filePath.substr(lastSlash + 1);
        } else {
            g_CurrentTitle = filePath;
        }

        // 让该音轨开始发出声音
        ma_sound_start(&g_CurrentSound);
        return true;
    }

    void TogglePlay() {
        std::lock_guard<std::mutex> lock(g_AudioMutex);
        if (!g_IsSoundLoaded) return; // 没载入歌曲，按了也没反应

        // 检查当前状态，如果正在响则暂停，如果暂停了则继续
        if (ma_sound_is_playing(&g_CurrentSound)) {
            ma_sound_stop(&g_CurrentSound); // 停止发出声音（相当于暂停，磁头不会归零）
        } else {
            ma_sound_start(&g_CurrentSound); // 继续播放
        }
    }

    void SetVolume(float volume) {
        std::lock_guard<std::mutex> lock(g_AudioMutex);
        if (!g_IsSoundLoaded) return;
        
        // 调节当前歌曲的音量大小
        ma_sound_set_volume(&g_CurrentSound, volume);
    }

    void SetProgress(float pct) {
        std::lock_guard<std::mutex> lock(g_AudioMutex);
        if (!g_IsSoundLoaded) return;

        // 注意：音频世界里没有“秒”或“毫秒”概念，底层的最小时间计量单位叫 PCM 采样帧 (Frames)
        ma_uint64 totalFrames = 0;
        // 获取这首歌曲总共包含多少个采样帧
        ma_sound_get_length_in_pcm_frames(&g_CurrentSound, &totalFrames);
        
        // 目标帧 = 总帧数 * 目标百分比
        ma_uint64 targetFrame = (ma_uint64)(totalFrames * pct);
        // 调用快进/快退 API，直接把播放器的读取游标移到目标帧位置
        ma_sound_seek_to_pcm_frame(&g_CurrentSound, targetFrame);
    }

    float GetProgress() {
        std::lock_guard<std::mutex> lock(g_AudioMutex);
        if (!g_IsSoundLoaded) return 0.0f;

        ma_uint64 totalFrames = 0;
        ma_sound_get_length_in_pcm_frames(&g_CurrentSound, &totalFrames);
        if (totalFrames == 0) return 0.0f; // 防御性编程，避免除以 0 导致程序崩溃

        ma_uint64 currentFrame = 0;
        // 获取当前播放到第几个采样帧了（即播放进度游标位置）
        ma_sound_get_cursor_in_pcm_frames(&g_CurrentSound, &currentFrame);

        // 返回比例：当前帧 / 总帧数
        return (float)currentFrame / (float)totalFrames;
    }

    void GetTimeStr(char* buffer, int maxLen) {
        std::lock_guard<std::mutex> lock(g_AudioMutex);
        // 如果没歌或者缓冲区给得太小，直接填入 00:00 占位，防止越界崩溃
        if (!g_IsSoundLoaded || maxLen < 15) {
            snprintf(buffer, maxLen, "00:00 / 00:00");
            return;
        }

        // 获取音频采样率（例如标准 CD 音质通常是 44100Hz，代表一秒包含 44100 个采样帧）
        ma_uint32 sampleRate = ma_engine_get_sample_rate(&g_AudioEngine);
        
        ma_uint64 totalFrames = 0;
        ma_sound_get_length_in_pcm_frames(&g_CurrentSound, &totalFrames);
        
        ma_uint64 currentFrame = 0;
        ma_sound_get_cursor_in_pcm_frames(&g_CurrentSound, &currentFrame);
        
        float lengthInSeconds = 0.0f;
        float currentInSeconds = 0.0f;
        
        // 核心公式：秒数 = 总帧数 / 采样率
        if (sampleRate > 0) {
            lengthInSeconds = (float)totalFrames / sampleRate;
            currentInSeconds = (float)currentFrame / sampleRate;
        }

        // 分别拆出 分钟数 和 秒数
        int curMin = (int)currentInSeconds / 60;
        int curSec = (int)currentInSeconds % 60;
        int lenMin = (int)lengthInSeconds / 60;
        int lenSec = (int)lengthInSeconds % 60;

        // 安全地拼装成类似 "03:14 / 04:45" 的可读文本放入 buffer 中
        snprintf(buffer, maxLen, "%02d:%02d / %02d:%02d", curMin, curSec, lenMin, lenSec);
    }

    bool IsPlaying() {
        std::lock_guard<std::mutex> lock(g_AudioMutex);
        if (!g_IsSoundLoaded) return false;
        // 直接调用 miniaudio 底层查询接口
        return ma_sound_is_playing(&g_CurrentSound);
    }

    const char* GetCurrentTitle() {
        // 只是单纯的返回一个已经分配好内存的静态字符串地址，不需要加锁
        return g_CurrentTitle.c_str();
    }

    void ScanMusicFolder(const std::string& folderPath) {
        std::lock_guard<std::mutex> lock(g_AudioMutex);
        g_CurrentFolderPath = folderPath;
        
        // 严谨规范：如果用户填写的路径末尾没有加斜杠，自动帮他补上斜杠，方便后面做路径拼接
        if (g_CurrentFolderPath.back() != '/') {
            g_CurrentFolderPath += "/";
        }

        g_SongFiles.clear(); // 清空上次遗留的旧歌单
        
        // 使用 C 语言标准底层的目录检索机制（Android NDK 完美支持）
        DIR* dir = opendir(folderPath.c_str());
        if (!dir) return; // 文件夹不存在或者没有读写权限，直接退出

        struct dirent* entry;
        // 循环读取该文件夹下的每一个文件/子目录项
        while ((entry = readdir(dir)) != nullptr) {
            // 通过 strstr 过滤出带有常见音频格式后缀的文件
            if (strstr(entry->d_name, ".mp3") || 
                strstr(entry->d_name, ".flac") || 
                strstr(entry->d_name, ".wav") ||
                strstr(entry->d_name, ".ogg")) {
                // 将符合标准的文件名塞入我们的全局歌单 vector 中
                g_SongFiles.push_back(entry->d_name);
            }
        }
        closedir(dir); // 检索完毕，必须手动关闭目录流，防止句柄泄露
    }

    const std::vector<std::string>& GetSongList() {
        return g_SongFiles; // 返回只读容器引用
    }

    bool PlayAtIndex(int index) {
        std::string fullPath;
        
        // 先局部上锁做边界检查并提取出完整的歌曲绝对路径
        {
            std::lock_guard<std::mutex> lock(g_AudioMutex);
            if (index < 0 || index >= (int)g_SongFiles.size()) return false; // 索引非法越界防御
            
            // 拼接出完整路径（如："/sdcard/Music/" + "MySong.mp3"）
            fullPath = g_CurrentFolderPath + g_SongFiles[index];
        }
        
        // 直接调用上面的 PlayFile 播放该绝对路径歌曲（PlayFile 内部自身带有锁，故在锁外侧调用）
        return PlayFile(fullPath);
    }
}
