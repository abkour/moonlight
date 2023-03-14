#pragma once
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx12.h"

#include <filesystem>

namespace moonlight
{

class AssetFileBrowser
{
public:

    AssetFileBrowser();
    AssetFileBrowser(const char* starting_directory);

    char* display();

private:

    std::filesystem::directory_entry m_current_dir;
};

}