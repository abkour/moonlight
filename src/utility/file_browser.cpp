#include "file_browser.hpp"
#include <Windows.h>

namespace moonlight
{

namespace fs = std::filesystem;

AssetFileBrowser::AssetFileBrowser()
{
    m_current_dir = fs::directory_entry(fs::current_path());
}

AssetFileBrowser::AssetFileBrowser(const char* starting_directory)
{
    m_current_dir = fs::directory_entry(fs::path(starting_directory));
}

char* AssetFileBrowser::display(AssetFileType& asset_file_type)
{
    int n = 0;
    static int selection = -1;
    static char str_buffer[512];
    ImGuiStyle* style = &ImGui::GetStyle();

    wcstombs(str_buffer, (wchar_t*)fs::path(m_current_dir).c_str(), 512);

    ImGui::Begin("File Browser");
    
    ImGui::BeginChild(
        ImGui::GetID((void*)(intptr_t)0),
        ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y),
        true, 
        ImGuiWindowFlags_MenuBar
    );

    if (ImGui::BeginMenuBar())
    {
        ImGui::PushButtonRepeat(true);
        if (ImGui::ArrowButton("##left", ImGuiDir_Left))
        {
            auto& p = m_current_dir.path();
            auto parent_path = p.parent_path();
            fs::directory_entry parent_dir(parent_path);
            if (parent_dir.is_directory())
            {
                m_current_dir = parent_dir;
            }
        }
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

        ImGui::PopButtonRepeat();
        ImGui::SameLine();
        ImGui::Text(str_buffer);
        ImGui::EndMenuBar();
    }

    // Display folders
    style->Colors[ImGuiCol_Text] = ImVec4(1.f, 1.f, 0.f, 1.f);
    for (auto const& dir_entry : fs::directory_iterator(m_current_dir))
    {
        if (dir_entry.is_directory())
        {
            wcstombs(str_buffer, (wchar_t*)dir_entry.path().filename().c_str(), 512);
            if (ImGui::Selectable(str_buffer, selection == n))
            {
                selection = n;
                m_current_dir = dir_entry;
            }
            ++n;
        }
    }

    // Display .wof files and return when one is selcted
    style->Colors[ImGuiCol_Text] = ImVec4(0.f, 1.f, 0.f, 1.f);
    for (auto const& dir_entry : fs::directory_iterator(m_current_dir))
    {
        auto extension = dir_entry.path().extension();

        if (extension == L".mof")
        {
            asset_file_type = MOF;
        }
        else if (extension == L".bvh")
        {
            asset_file_type = BVH;
        }
        else
        {
            asset_file_type = Unknown;
        }

        if (asset_file_type != Unknown)
        {
            wcstombs(str_buffer, (wchar_t*)dir_entry.path().filename().c_str(), 512);
            if (ImGui::Selectable(str_buffer, selection == n))
            {
                selection = n;
                wcstombs(str_buffer, (wchar_t*)dir_entry.path().c_str(), 512);
                
                ImGui::EndChild();
                ImGui::End();

                return str_buffer;
            }
            ++n;
        }
    }

    // Display files
    style->Colors[ImGuiCol_Text] = ImVec4(1.f, 1.f, 1.f, 1.f);
    for (auto const& dir_entry : fs::directory_iterator(m_current_dir))
    {
        auto& p = dir_entry.path();
        if (!dir_entry.is_directory() && dir_entry.path().extension() != L".mof")
        {
            wcstombs(str_buffer, (wchar_t*)p.filename().c_str(), 512);
            if (ImGui::Selectable(str_buffer, selection == n))
            {
                selection = n;
            }
            ++n;
        }
    }

    ImGui::EndChild();
    ImGui::End();
    
    return nullptr;
}

}