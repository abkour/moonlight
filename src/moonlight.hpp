#pragma once
#include "application.hpp"
#include "config.hpp"

namespace moonlight {

class MoonLight {
public:
    MoonLight() = delete;
    MoonLight(Config config);
private:
    std::unique_ptr<Application> m_application;
};

}