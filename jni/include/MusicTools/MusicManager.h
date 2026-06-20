#pragma once
#include <string>
#include <vector>

// 使用单独的命名空间 Music，避免与全局或其他第三方库的函数名发生冲突（高内聚、低耦合设计）
namespace Music {
    
    // ==========================================
    // 一、 生命周期管理函数
    // ==========================================
    
    /**
     * @brief 初始化音频引擎
     * @return true 初始化成功，false 初始化失败
     * @note 整个程序运行期间只需要在入口处调用一次（例如应用启动时）
     */
    bool Init();

    /**
     * @brief 彻底关闭音频引擎并释放所有相关资源
     * @note 应该在程序准备退出、关闭悬浮窗销毁上下文时调用，防止内存泄漏或音频硬件死锁
     */
    void Shutdown();
    
    // ==========================================
    // 二、 播放状态控制接口
    // ==========================================

    /**
     * @brief 根据传入的绝对路径播放指定音频文件
     * @param filePath 音乐文件的完整路径（如 "/storage/emulated/0/Music/test.mp3"）
     * @return true 成功打开并开始播放，false 文件不存在或格式不支持
     */
    bool PlayFile(const std::string& filePath);

    /**
     * @brief 切换播放/暂停状态（俗称：一键暂停或继续）
     * @note 如果当前正在放歌，则暂停；如果处于暂停状态，则继续播放
     */
    void TogglePlay();

    /**
     * @brief 设置当前音轨的音量
     * @param volume 音量大小，范围限定在 0.0f（完全静音） 到 1.0f（最大原音音量） 之间
     */
    void SetVolume(float volume);

    /**
     * @brief 强制跳转/拖动歌曲进度
     * @param pct 进度百分比，范围在 0.0f（歌曲开头） 到 1.0f（歌曲结尾） 之间
     * @note 对应 ImGui 里的 Slider 拖拽更新
     */
    void SetProgress(float pct);
    
    // ==========================================
    // 三、 状态与时间数据获取接口（供 ImGui 渲染调用）
    // ==========================================

    /**
     * @brief 获取当前歌曲的播放进度百分比
     * @return 返回 0.0f 到 1.0f 之间的浮点数，直接传递给 ImGui::SliderFloat 或 ProgressBar 显示
     */
    float GetProgress();

    /**
     * @brief 获取格式化好的时间字符串
     * @param buffer 接收输出的字符数组指针
     * @param maxLen 缓冲区最大长度，防止内存越界
     * @note 输出格式示例: "01:45 / 04:12"，非常方便直接用 ImGui::Text 渲染
     */
    void GetTimeStr(char* buffer, int maxLen);

    /**
     * @brief 查询当前是否正在响着歌
     * @return true 正在播放，false 处于暂停、停止或未加载歌曲状态
     */
    bool IsPlaying();

    /**
     * @brief 获取当前播放歌曲的纯文本名称
     * @return 返回当前播放的文件名字符串指针，未放歌时返回 "未在播放"
     */
    const char* GetCurrentTitle();

    // ==========================================
    // 四、 歌单管理与本地文件检索接口
    // ==========================================

    /**
     * @brief 扫描指定的本地文件夹，筛选出所有音频文件并建立临时歌单
     * @param folderPath 目标文件夹的绝对路径
     */
    void ScanMusicFolder(const std::string& folderPath);

    /**
     * @brief 获取已检索到的歌曲文件名列表
     * @return 返回一个包含所有筛选出的音乐文件名的只读 vector 引用
     */
    const std::vector<std::string>& GetSongList();

    /**
     * @brief 播放当前建立的歌单中对应索引位置的歌曲
     * @param index 歌曲在歌单 vector 中的下标
     * @return true 切换并成功播放，false 索引越界或播放失败
     */
    bool PlayAtIndex(int index);
}
