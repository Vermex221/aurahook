// Created by Valorr19
// elements.h

#pragma once
#include <string>
#include <memory>
#include <imgui.h>

class c_elements
{
public:

    struct 
    {
        std::string name{ "Kitty.cc" };
        ImVec2 size{ 760, 560 };
        float rounding{ 10 };
    } window;
};

inline std::unique_ptr<c_elements> elements = std::make_unique<c_elements>();
