#include "draw.h"
#include "My_font/zh_Font.h"
int abs_ScreenX = 0, abs_ScreenY = 0;
bool permeate_record = false;//如果为true则为录制穿透已开启标志
bool permeate_record_ini = false;//录制穿透关开布尔变量
struct Last_ImRect LastCoordinate = {0, 0, 0, 0};
std::unique_ptr<AndroidImgui> graphics;
ANativeWindow *window = NULL; 
android::ANativeWindowCreator::DisplayInfo displayInfo;
ImGuiWindow *g_window = NULL;//初始化窗口信息指针
int native_window_screen_x = 0, native_window_screen_y = 0;
ImFont* zh_font = NULL;//初始化字体指针
ImFont* zh_font1 = NULL;//初始化字体指针
float FontSize = 30.0f; //字体大小
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
    screen_config();
    if (orientation != displayInfo.orientation) {
        orientation = displayInfo.orientation;
        Touch::setOrientation(displayInfo.orientation);
        if (g_window != NULL) {
            g_window->Pos.x = 100;
            g_window->Pos.y = 125;        
        }
    }
}

void Layout_tick_UI(bool *main_thread_flag) {    
    // 分配空间用于收集当前帧渲染的窗口范围
    My_Vector2 WindowPos[10];
    My_Vector2 WindowSize[10];
    int WindowCount = 0;
    
    
        ImGui::Begin("悬浮窗", main_thread_flag);        
        // 如果上次有记录位置和大小，恢复它们
        if (::permeate_record_ini) {
            ImGui::SetWindowPos({LastCoordinate.Pos_x, LastCoordinate.Pos_y});
            ImGui::SetWindowSize({LastCoordinate.Size_x, LastCoordinate.Size_y});
            permeate_record_ini = false;   
        }
        
        // 只有一个功能：过录制
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "录制穿透开关");
        if (ImGui::Checkbox("过录制", &::permeate_record)) {
            ::permeate_record_ini = true;
        }
        
        // 显示状态
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        if (::permeate_record) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "状态: 录制穿透已开启");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "状态: 录制穿透已关闭");
        }
        

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);// 显示FPS信息
         
        ImGui::Text("这是一个文字 ");
        ImGui::Text("关注一下我呗 ");
        ImGui::Text("[+] 抖音号:ACoier0409cpp");
        ImGui::Text("[+] 哔哩哔哩:UID:3546556374452457");
        

        // 捕捉收集本悬浮窗的物理坐标边界
        if (WindowCount < 10) {
            WindowPos[WindowCount] = My_Vector2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);
            WindowSize[WindowCount] = My_Vector2(ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
            WindowCount++;
        }      
        g_window = ImGui::GetCurrentWindow();// 保存当前窗口指针用于位置恢复
        ImGui::End();     
    
        Touch::SetTouchObstacle(WindowPos, WindowSize, WindowCount);// 将本帧所有收集到的窗口数据同步到底层驱动拦截链中
}
