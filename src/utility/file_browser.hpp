#pragma once
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx12.h"

#include <filesystem>

namespace moonlight
{

enum AssetFileType
{
    Unknown = 0,
    MOF = 1,
    BVH = 2
};

class AssetFileBrowser
{
public:

    AssetFileBrowser();
    AssetFileBrowser(const char* starting_directory);

    char* display(AssetFileType& asset_file_type);

private:

    std::filesystem::directory_entry m_current_dir;
};

}