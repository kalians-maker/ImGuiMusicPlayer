#include "draw.h"
#include "My_font/zh_Font.h"
#include "MusicManager.h"  
#include "imgui.h"
#include "imgui_internal.h"
#include <cmath>
#include <vector>
#include <string>
#include <fstream>   // 引入文件流，用于加载并解析本地 LRC 歌词
#include <algorithm> // 引入算法库用于歌词时间轴安全排序
#include "WenFeng.h"
// ==========================================
// 1. 原项目既有的全局变量与结构
// ==========================================
int abs_ScreenX = 0, abs_ScreenY = 0;
bool permeate_record = false;      
bool permeate_record_ini = false;  
struct Last_ImRect LastCoordinate = {0, 0, 0, 0};
std::unique_ptr<AndroidImgui> graphics;
ANativeWindow *window = NULL; 
android::ANativeWindowCreator::DisplayInfo displayInfo;
ImGuiWindow *g_window = NULL;      
int native_window_screen_x = 0, native_window_screen_y = 0;
ImFont* zh_font = NULL;            
ImFont* zh_font1 = NULL;           
float FontSize = 30.0f;            
TextureInfo my_avatar_texture2 = {0};
// ==========================================
// 2. 赛博内核：大一统智能形态过渡状态机
// ==========================================
enum PlayerState { STATE_ISLAND, STATE_PLAYER };
static PlayerState s_CurrentState       = STATE_PLAYER; 
static bool        s_AudioInitialized   = false;
static bool        s_IsTransitioning    = false; 

// 丝滑形变实时物理参数
static float s_CurX = 100.0f, s_CurY = 150.0f;        
static float s_CurW = 360.0f, s_CurH = 500.0f;        
static float s_ContentAlpha = 1.0f;                  

// 记忆化存储：完美锁存用户手动拖拽拉伸后的个性化长宽与坐标
static float s_UserX = -1.0f,  s_UserY = 120.0f;
static float s_UserW = 360.0f, s_UserH = 540.0f; 

static int   s_SelectedSongIndex = -1;

// 播放控制状态属性
static bool  s_LoopMode         = false; // 循环播放模式
static bool  s_RandomMode       = false; // 随机播放模式
static bool  s_ShowLyrics       = false; // 歌词全局可见性控制锁（默认隐藏）

// ==========================================
// 3. 高级本地 LRC 歌词精密解析引擎
// ==========================================
struct LyricLine {
    float time; 
    std::string text;
};
static std::vector<LyricLine> s_ParsedLyrics;
static std::string s_LastSongTitle = "";
static std::string s_ActiveLyricText = "BoxMusic 极客舱准备就绪";

void DynamicLoadLRC(const std::string& songTitle) {
    if (songTitle == s_LastSongTitle) return; 
    s_LastSongTitle = songTitle;
    s_ParsedLyrics.clear();

    if (songTitle == "未在播放" || songTitle == "播放失败") {
        s_ActiveLyricText = "等待音轨序列载入...";
        return;
    }

    std::string lrcFileName = songTitle;
    size_t dotPos = lrcFileName.find_last_of(".");
    if (dotPos != std::string::npos) {
        lrcFileName = lrcFileName.substr(0, dotPos) + ".lrc";
    } else {
        lrcFileName += ".lrc";
    }

    std::string fullLrcPath = "/storage/emulated/0/Music/BoxMusic/" + lrcFileName;
    std::ifstream file(fullLrcPath);
    if (!file.is_open()) {
        s_ActiveLyricText = "未检测到同步 LRC 歌词";
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.size() >= 8 && line[0] == '[') {
            int min = 0, sec = 0;
            if (sscanf(line.c_str(), "[%d:%d", &min, &sec) == 2) {
                float totalSeconds = min * 60.0f + sec;
                size_t closeBracket = line.find(']');
                if (closeBracket != std::string::npos) {
                    std::string txt = line.substr(closeBracket + 1);
                    if (!txt.empty() && txt != "\r" && txt != "\n") {
                        s_ParsedLyrics.push_back({totalSeconds, txt});
                    }
                }
            }
        }
    }
    file.close();

    std::sort(s_ParsedLyrics.begin(), s_ParsedLyrics.end(), [](const LyricLine& a, const LyricLine& b) {
        return a.time < b.time;
    });
}

void RefreshLyricStreaming() {
    char timeBuf[64];
    Music::GetTimeStr(timeBuf, sizeof(timeBuf));
    int curMin = 0, curSec = 0;
    sscanf(timeBuf, "%d:%d", &curMin, &curSec);
    float currentSeconds = curMin * 60.0f + curSec;

    if (s_ParsedLyrics.empty()) return;
    s_ActiveLyricText = s_ParsedLyrics[0].text;
    for (const auto& item : s_ParsedLyrics) {
        if (currentSeconds >= item.time) {
            s_ActiveLyricText = item.text;
        } else {
            break;
        }
    }
}

void CyberAnimateFloat(float& current, float target, float speed, float dt) {
    current += (target - current) * speed * dt;
    if (fabsf(current - target) < 0.1f) current = target;
}

void DrawCyberVisualizer(ImDrawList* drawList, ImVec2 startPos, float width, float maxHeight, float time, bool isPlaying) {
    const int barCount = 20;           
    float barWidth = width / barCount;
    float spacing = 1.5f;              

    for (int i = 0; i < barCount; i++) {
        float targetHeight = 0.0f;
        if (isPlaying) {
            float waveA = sinf(time * 9.5f + i * 0.6f) * 0.5f + 0.5f;
            float waveB = cosf(time * 13.0f - i * 0.9f) * 0.3f + 0.2f;
            targetHeight = (waveA + waveB) * maxHeight;
            if (targetHeight < 4.0f) targetHeight = 4.0f;
        } else {
            targetHeight = 2.5f; 
        }

        static float currentHeights[barCount] = { 0 };
        currentHeights[i] += (targetHeight - currentHeights[i]) * 14.0f * ImGui::GetIO().DeltaTime;

        ImVec2 pMin = ImVec2(startPos.x + i * barWidth + spacing, startPos.y - currentHeights[i]);
        ImVec2 pMax = ImVec2(startPos.x + (i + 1) * barWidth, startPos.y);

        ImColor barColor = ImColor::HSV(0.52f + (float)i / barCount * 0.22f, 0.9f, 0.95f);
        drawList->AddRectFilled(pMin, pMax, barColor, 3.0f, ImDrawFlags_RoundCornersTop);
    }
}

// ==========================================
// 3. 原项目既有的基础配置函数
// ==========================================
bool M_Android_LoadFont(float SizePixels) {
    ImGuiIO &io = ImGui::GetIO();    
    io.Fonts->Clear();
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.SizePixels = SizePixels;
    config.OversampleH = 1;
    ::zh_font = io.Fonts->AddFontFromMemoryTTF((void *)OPPOSans_H, OPPOSans_H_size, 0.0f, &config, io.Fonts->GetGlyphRangesChineseFull());   
  
    io.Fonts->AddFontDefault();
    return zh_font != nullptr;
}

void init_My_drawdata() {
    M_Android_LoadFont(FontSize);
    ImGui::GetStyle().ScaleAllSizes(2.0f);
}

void screen_config() {
    ::displayInfo = android::ANativeWindowCreator::GetDisplayInfo();
}

void drawBegin() {
    if (::permeate_record_ini) {
        LastCoordinate.Pos_x = ::g_window->Pos.x;
        LastCoordinate.Pos_y = ::g_window->Pos.y;
        LastCoordinate.Size_x = ::g_window->Size.x;
        LastCoordinate.Size_y = ::g_window->Size.y;

        graphics->Shutdown();
        android::ANativeWindowCreator::Destroy(::window);
        ::window = android::ANativeWindowCreator::Create("simple_window", native_window_screen_x, native_window_screen_y, permeate_record);
        graphics->Init_Render(::window, native_window_screen_x, native_window_screen_y);
        ::init_My_drawdata();
        permeate_record_ini = false;
    }

    static uint32_t orientation = -1;
    ::screen_config(); 
    if (orientation != displayInfo.orientation) {
        orientation = displayInfo.orientation;
        Touch::setOrientation(displayInfo.orientation);
        if (g_window != NULL) {
            g_window->Pos.x = 100;
            g_window->Pos.y = 125;        
        }
    }
}

// ==========================================
// 5. 大一统全能操控中心实现
// ==========================================
void Layout_tick_UI(bool *main_thread_flag) {    
    My_Vector2 WindowPos[10];
    My_Vector2 WindowSize[10];
    int WindowCount = 0;

    if (!s_AudioInitialized) {
        if (Music::Init()) {
            Music::ScanMusicFolder("/storage/emulated/0/Music/BoxMusic");
            s_AudioInitialized = true;
        }
    }

    float dt = ImGui::GetIO().DeltaTime;
    float runTime = (float)ImGui::GetTime();

    ::screen_config();
    float screenW = (float)::displayInfo.width;
    float screenH = (float)::displayInfo.height;

    if (s_UserX < 0 && screenW > 0) {
        s_UserX = (screenW - s_UserW) * 0.5f;
    }

    static bool s_TrackWasPlayingLastFrame = false;
    bool isTrackPlayingNow = Music::IsPlaying();
    float trackProgressCheck = Music::GetProgress();

    if (s_SelectedSongIndex != -1 && s_TrackWasPlayingLastFrame && !isTrackPlayingNow && trackProgressCheck >= 0.95f) {
        const auto& globalPlaylist = Music::GetSongList();
        if (!globalPlaylist.empty()) {
            if (s_LoopMode) {
                Music::PlayAtIndex(s_SelectedSongIndex);
            } else if (s_RandomMode) {
                s_SelectedSongIndex = rand() % globalPlaylist.size();
                Music::PlayAtIndex(s_SelectedSongIndex);
            } else {
                if (s_SelectedSongIndex < (int)globalPlaylist.size() - 1) {
                    s_SelectedSongIndex++;
                    Music::PlayAtIndex(s_SelectedSongIndex);
                } else {
                    s_SelectedSongIndex = -1; 
                }
            }
        }
    }
    s_TrackWasPlayingLastFrame = isTrackPlayingNow;

    DynamicLoadLRC(Music::GetCurrentTitle());
    RefreshLyricStreaming();

    if (s_ShowLyrics) {
        ImDrawList* fgDrawList = ImGui::GetForegroundDrawList();
        ImVec2 lyricSize = ImGui::CalcTextSize(s_ActiveLyricText.c_str());
        ImVec2 lyricPos = ImVec2((screenW - lyricSize.x) * 0.5f, 105.0f);
        
        fgDrawList->AddText(ImVec2(lyricPos.x + 2, lyricPos.y + 2), ImColor(5, 5, 5, 230), s_ActiveLyricText.c_str()); 
        fgDrawList->AddText(lyricPos, ImColor(255, 235, 0, 255), s_ActiveLyricText.c_str()); 
    }

    // ─── B. 形变锁与坐标对齐驱动 ───
    float targetX, targetY, targetW, targetH, targetAlpha;
    if (s_CurrentState == STATE_ISLAND) {
        targetW = 260.0f;
        targetH = 48.0f;
        targetX = (screenW - targetW) * 0.5f; 
        targetY = 35.0f; 
        targetAlpha = 0.0f;
    } else {
        targetW = s_UserW;
        targetH = s_UserH;
        targetX = s_UserX;
        targetY = s_UserY;
        targetAlpha = 1.0f;
    }

    if (s_IsTransitioning) {
        CyberAnimateFloat(s_CurX, targetX, 14.0f, dt);
        CyberAnimateFloat(s_CurY, targetY, 14.0f, dt);
        CyberAnimateFloat(s_CurW, targetW, 14.0f, dt);
        CyberAnimateFloat(s_CurH, targetH, 14.0f, dt);
        CyberAnimateFloat(s_ContentAlpha, targetAlpha, 15.0f, dt);

        ImGui::SetNextWindowPos(ImVec2(s_CurX, s_CurY));
        ImGui::SetNextWindowSize(ImVec2(s_CurW, s_CurH));

        if (fabsf(s_CurW - targetW) < 0.5f && fabsf(s_CurH - targetH) < 0.5f && fabsf(s_CurX - targetX) < 0.5f) {
            s_IsTransitioning = false; 
        }
    } else {
        if (s_CurrentState == STATE_ISLAND) {
            ImGui::SetNextWindowPos(ImVec2(targetX, targetY));
            ImGui::SetNextWindowSize(ImVec2(targetW, targetH));
        } else {
            ImGui::SetNextWindowPos(ImVec2(s_UserX, s_UserY), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(s_UserW, s_UserH), ImGuiCond_Once);
        }
    }

    // ─── C. 渲染大一统面板 ───
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, s_CurrentState == STATE_ISLAND ? 24.0f : 14.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    
 //彻底消除边框、阴影和手柄残留的白线
ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

// 新增：调整大小手柄及其悬停/激活状态设为全透明（这是白线的常见来源）
ImGui::PushStyleColor(ImGuiCol_ResizeGrip, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::GetStyle().WindowBorderSize = 0.0f; // 彻底去掉窗口边框
    
    ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
    if (s_CurrentState == STATE_ISLAND || s_IsTransitioning) {
        winFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    }

    ImGui::Begin("CyberMusicSpace##Unified", nullptr, winFlags);
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();

    if (s_CurrentState == STATE_PLAYER && !s_IsTransitioning) {
        s_UserX = winPos.x;
        s_UserY = winPos.y;
        s_UserW = winSize.x;
        s_UserH = winSize.y;
    }

    drawList->AddRectFilled(winPos, ImVec2(winPos.x + winSize.x, winPos.y + winSize.y), ImColor(ImVec4(0.08f, 0.08f, 0.08f, 1.0f)), ImGui::GetStyle().WindowRounding);

    // ─── 形态 1：苹果同款高级灵动岛形态 (Marquee Mini Mode) ───
    if (s_CurrentState == STATE_ISLAND || s_ContentAlpha < 0.15f) {
        if (ImGui::InvisibleButton("IslandHotZone", ImVec2(winSize.x, winSize.y))) {
            s_CurrentState = STATE_PLAYER; 
            s_IsTransitioning = true; 
        }

        std::string songTitle = Music::GetCurrentTitle();
        if (songTitle == "未在播放") songTitle = "BoxMusic 灵动空间舱";
        ImVec2 ts = ImGui::CalcTextSize(songTitle.c_str());
        
        float maxAvailableW = winSize.x - 30.0f; 
        ImVec2 tPos = ImVec2(winPos.x + 16.0f, winPos.y + (winSize.y - ImGui::GetTextLineHeight()) * 0.5f);
        
        drawList->PushClipRect(ImVec2(winPos.x + 12.0f, winPos.y), ImVec2(winPos.x + winSize.x - 12.0f, winPos.y + winSize.y), true);
        if (ts.x > maxAvailableW) {
            float totalScrollLoop = ts.x + 50.0f; 
            float textOffset = fmodf(runTime * 45.0f, totalScrollLoop); 
            
            drawList->AddText(ImVec2(tPos.x - textOffset, tPos.y), ImColor(0, 255, 230, 255), songTitle.c_str());
            drawList->AddText(ImVec2(tPos.x - textOffset + totalScrollLoop, tPos.y), ImColor(0, 255, 230, 255), songTitle.c_str());
        } else {
            drawList->AddText(tPos, ImColor(0, 255, 230, 255), songTitle.c_str());
        }
        drawList->PopClipRect();
    } 
    // ─── 形态 2：全尺寸极客播放器形态 (Pro Capabilities Mode) ───
    else {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, s_ContentAlpha);

        // 1. 左上角红绿灯三点系统
        float radius = 8.5f; 
        ImVec2 dotR = ImVec2(winPos.x + 25.0f, winPos.y + 25.0f);
        ImVec2 dotY = ImVec2(winPos.x + 48.0f, winPos.y + 25.0f);
        ImVec2 dotG = ImVec2(winPos.x + 71.0f, winPos.y + 25.0f);
        drawList->AddCircleFilled(dotR, radius, ImColor(255, 95, 87, 255));
        drawList->AddCircleFilled(dotY, radius, ImColor(255, 189, 46, 255));
        drawList->AddCircleFilled(dotG, radius, ImColor(39, 201, 63, 255));

        if (ImGui::IsMouseClicked(0)) {
            ImVec2 mPos = ImGui::GetIO().MousePos;
            float dist = sqrtf(powf(mPos.x - dotY.x, 2) + powf(mPos.y - dotY.y, 2));
            if (dist <= 70.0f) {
                s_CurrentState = STATE_ISLAND;
                s_IsTransitioning = true; 
            }
        }

        // 2. 经典的高级头部标题栏
        const char* mainTitle = "Hi-Res 音乐播放器";
        ImVec2 mainTitleSize = ImGui::CalcTextSize(mainTitle);
        drawList->AddText(ImVec2(winPos.x + (winSize.x - mainTitleSize.x) * 0.5f, winPos.y + 15.0f), ImColor(0, 255, 240, 255), mainTitle);

        // 3. 显著增大标题文字与下方波浪动画之间的垂直间距
        ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
        ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
        float visPadding = 15.0f;
        ImVec2 visStart = ImVec2(winPos.x + visPadding, winPos.y + 155.0f); 
        float dynamicVisWidth = winSize.x - (visPadding * 2.0f);
        DrawCyberVisualizer(drawList, visStart, dynamicVisWidth, 50.0f, runTime, Music::IsPlaying());
        ImGui::Dummy(ImVec2(20.0f, 50.0f));

        // 4. 曲目名称状态读条
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), "CURRENT AUDIO TRACK:");
        ImGui::TextColored(ImColor(0, 255, 240, 255), "%s", Music::GetCurrentTitle());

        // 5. 时间精密进度条
        char timeBuf[64];
        Music::GetTimeStr(timeBuf, sizeof(timeBuf));
        
        float songPct = Music::GetProgress();
        ImGui::PushItemWidth(-1.0f);
        if (ImGui::SliderFloat("##CyberSlider", &songPct, 0.0f, 1.0f, timeBuf)) {
            Music::SetProgress(songPct);
        }
        ImGui::PopItemWidth();
        ImGui::Spacing();

        // 6. 动态文字长自适应 5 按键均分等差矩阵控制台
        float framePaddingX = ImGui::GetStyle().FramePadding.x * 2.0f;
        float w1 = ImGui::CalcTextSize("循环播放").x + framePaddingX;
        float w2 = ImGui::CalcTextSize("随机播放").x + framePaddingX;
        float w3 = ImGui::CalcTextSize("上一首").x + framePaddingX;
        float w4 = ImGui::CalcTextSize(Music::IsPlaying() ? "暂停" : "播放").x + framePaddingX;
        float w5 = ImGui::CalcTextSize("下一首").x + framePaddingX;
        
        float totalButtonsWidth = w1 + w2 + w3 + w4 + w5;
        float outerPadding = 15.0f;
        float dynamicGap = (winSize.x - (outerPadding * 2.0f) - totalButtonsWidth) / 4.0f;
        if (dynamicGap < 4.0f) dynamicGap = 4.0f; 

        // 按键 1：循环播放
        ImGui::SetCursorPosX(outerPadding);
        if (s_LoopMode) ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0, 180, 255, 255));
        if (ImGui::Button("循环播放##Pro", ImVec2(w1, 0.0f))) {
            s_LoopMode = !s_LoopMode;
            if (s_LoopMode) s_RandomMode = false;
        }
        if (s_LoopMode) ImGui::PopStyleColor();
        ImGui::SameLine(0.0f, 0.0f);

        // 按键 2：随机播放
        ImGui::SetCursorPosX(outerPadding + w1 + dynamicGap);
        if (s_RandomMode) ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0, 180, 255, 255));
        if (ImGui::Button("随机播放##Pro", ImVec2(w2, 0.0f))) {
            s_RandomMode = !s_RandomMode;
            if (s_RandomMode) s_LoopMode = false;
        }
        if (s_RandomMode) ImGui::PopStyleColor();
        ImGui::SameLine(0.0f, 0.0f);

        // 按键 3：上一首
        ImGui::SetCursorPosX(outerPadding + w1 + dynamicGap + w2 + dynamicGap);
        if (ImGui::Button("上一首##Pro", ImVec2(w3, 0.0f))) {
            if (s_SelectedSongIndex > 0) {
                s_SelectedSongIndex--;
                Music::PlayAtIndex(s_SelectedSongIndex);
            }
        }
        ImGui::SameLine(0.0f, 0.0f);

        // 按键 4：暂停 / 播放
        ImGui::SetCursorPosX(outerPadding + w1 + dynamicGap + w2 + dynamicGap + w3 + dynamicGap);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(255, 0, 128, 255)); 
        if (ImGui::Button(Music::IsPlaying() ? "暂停##Pro" : "播放##Pro", ImVec2(w4, 0.0f))) {
            Music::TogglePlay();
        }
        ImGui::PopStyleColor();
        ImGui::SameLine(0.0f, 0.0f);

        // 按键 5：下一首
        ImGui::SetCursorPosX(outerPadding + w1 + dynamicGap + w2 + dynamicGap + w3 + dynamicGap + w4 + dynamicGap);
        if (ImGui::Button("下一首##Pro", ImVec2(w5, 0.0f))) {
            if (s_SelectedSongIndex >= 0 && s_SelectedSongIndex < (int)Music::GetSongList().size() - 1) {
                s_SelectedSongIndex++;
                Music::PlayAtIndex(s_SelectedSongIndex);
            }
        }

        // 7. 舱载音轨数据中心
        ImGui::Spacing(); ImGui::Spacing();
        ImGui::Spacing();
        
        if (ImGui::CollapsingHeader("音乐列表##Folder", 0)) {
            // 【核心修复】根据要求，将右侧小滑块的物理尺寸由 18.0f 再次调粗至 40.0f 极致丰满厚重
            ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 40.0f);       
            ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 14.0f);   
            ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.06f, 0.07f, 0.09f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, (ImVec4)ImColor(255, 0, 128, 240));       
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, (ImVec4)ImColor(255, 60, 160, 255));
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, (ImVec4)ImColor(0, 255, 230, 255));  

            float listHeight = winSize.y - (ImGui::GetCursorPosY() - winPos.y) - 185.0f;
            if (listHeight < 65.0f) listHeight = 65.0f;

            ImGui::BeginChild("##CyberScrollList", ImVec2(-1.0f, listHeight), true, ImGuiWindowFlags_NavFlattened);
            const auto& listData = Music::GetSongList();
            if (listData.empty()) {
                ImGui::Text(" 目录内未搜寻到任何音频数据序列...");
            } else {
                for (int i = 0; i < (int)listData.size(); i++) {
                    bool isCurrent = (s_SelectedSongIndex == i);
                    
                    if (isCurrent) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.9f, 1.0f));
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    }
                    
                    if (ImGui::Selectable(listData[i].c_str(), isCurrent, 0, ImVec2(0, 32.0f))) {
                        s_SelectedSongIndex = i;
                        Music::PlayAtIndex(i);
                    }
                    
                    ImGui::PopStyleColor();
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(2); 
        }
     
        // 8. 原厂过录制保护功能区
        if (ImGui::CollapsingHeader("录制穿透开关")) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
            if (ImGui::Checkbox("开启过录制保护", &::permeate_record)) {
                ::permeate_record_ini = true;
                my_avatar_texture2.DS = 0;
            }
            ImGui::PopStyleColor(); // 恢复默认颜色
            ImGui::SameLine();
            if (::permeate_record) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "状态: 录制穿透已开启");
            } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "状态: 录制穿透已关闭");
            }
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::PopStyleColor(); // 恢复默认颜色
        }
        if (ImGui::CollapsingHeader("作者&声明")) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
            if (my_avatar_texture2.DS == 0)
                    my_avatar_texture2 = graphics->LoadTextureFromMemory((void*)img_dataWenFeng, sizeof(img_dataWenFeng));
            ImGui::Image((ImTextureID)my_avatar_texture2.DS, ImVec2(150.0f, 150.0f));
            ImGui::SameLine();
            {
            ImGui::Text("作者：梁文文  ");            
            ImGui::Text("联系邮箱：kalivct08@hotmail.com ");
            }
            ImGui::Text("<<<<<<<<<<<<<<< 声明 >>>>>>>>>>>>>>>");
            ImGui::SetWindowFontScale(0.9f);
            ImGui::Text(" 渲染:Vulkan | ImGui:1.90.1 | Music Miniaudio");
            ImGui::Text("[+] 欢迎使用我们小而巧的Music播放工具,当前版本1.00.10.14");
            ImGui::Text("[+] 你应该在 /storage/emulated/0/Music/BoxMusic 里面存放你的音频和lrc歌词");            
            ImGui::Text("[+] 我打算将本项目设为开源项目，禁止二改后进行圈钱行为");
            ImGui::Text("[+] 祝您使用愉快~");
            
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor(); // 恢复默认颜色
        }

        // 9. 歌词全局双路流线控制器
        ImGui::Spacing();
        float lyricBtnHalfW = (winSize.x - 40.0f) * 0.5f;
        if (ImGui::Button("显示歌词##LyricAct", ImVec2(lyricBtnHalfW, 0.0f))) {
            s_ShowLyrics = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("隐藏歌词##LyricAct", ImVec2(lyricBtnHalfW, 0.0f))) {
            s_ShowLyrics = false;
        }

        // 10. 原厂退出项目大按钮
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(180, 0, 30, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(230, 0, 40, 255));
        if (ImGui::Button("TERMINATE SYSTEM / 安全退出项目", ImVec2(winSize.x - 30.0f, 0.0f))) {
            exit(0); 
        }
        ImGui::PopStyleColor(2);

        ImGui::PopStyleVar(); 
    }

    // ─── D. 统一向底层触控拦截拓扑登记汇报 ───
    if (WindowCount < 10) {
        WindowPos[WindowCount] = My_Vector2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);
        WindowSize[WindowCount] = My_Vector2(ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
        WindowCount++;
    }

    g_window = ImGui::GetCurrentWindow(); 
    ImGui::End();
    
    // 清理 PushStyleColor 的 Border 变量
    ImGui::PopStyleColor(7); // 因为我们一共 Push 了 7 个颜色（原有2个 + 新增5个）
ImGui::PopStyleVar(2);   // 弹出2个 Var

    Touch::SetTouchObstacle(WindowPos, WindowSize, WindowCount);
}
