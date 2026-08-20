//
//  window_util.hpp
//
//  Created by Fons Kuijk on 14-02-19.
//

#ifndef window_util_hpp
#define window_util_hpp
#pragma once

#define GLFW_INCLUDE_GLU

#include "GLFW/glfw3.h"
#include <iostream>
#include <functional>

// Struct for managing rotation of pointcloud view
struct glfw_state {
    glfw_state() : yaw(0.0), pitch(0.0), last_x(0.0), last_y(0.0), ml(false), offset(0.f) {}
    double yaw;
    double pitch;
    double last_x;
    double last_y;
    bool ml;
    float offset;
};

class window_util {
public:
    std::function<void(bool)>           on_left_mouse = [](bool) {};
    std::function<void(bool)>           on_right_mouse = [](bool) {};
    std::function<void(double, double)> on_mouse_scroll = [](double, double) {};
    std::function<void(double, double)> on_mouse_move = [](double, double) {};
    std::function<void(int)>            on_character = [](int) {};

    window_util(int width, int height, const char* title);
    ~window_util();

    operator bool();
    operator GLFWwindow*();

    glfw_state* app_state();
    float width() const;
    float height() const;
    void prepare_gl(float x, float y, float z, float pointSize);
    void cleanup_gl();

private:
  GLFWwindow* win;
  glfw_state _appstate;
  int _width, _height;
};
#endif //window_util_hpp
