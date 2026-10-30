#include "StateMainMenu.hpp"
#include "Global.hpp"
#include "Window.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>

StateMainMenu::StateMainMenu(std::shared_ptr<Connection> conn)
    : State(STATE_MAIN_MENU), server_conn(std::move(conn)), db("world_data.db") {}

void StateMainMenu::init() {
    db.init();
    global.window->set_grabbed(false);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(global.window->get_handle(), true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void StateMainMenu::destroy() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void StateMainMenu::update() {
    auto *wnd = global.window.get();
    if (wnd->get_keyboard()->keys[GLFW_KEY_ESCAPE].pressed) {
        wnd->set_should_close(true);
    }
}

void StateMainMenu::render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiIO &io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - 300.0f) * 0.5f, (io.DisplaySize.y - 250.0f) * 0.5f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300.0f, 250.0f));

    if (ImGui::Begin("Voxel Game - Authentication", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Username:");
        ImGui::InputText("##username", username_buf, sizeof(username_buf));

        ImGui::Spacing();
        ImGui::Text("Password:");
        ImGui::InputText("##password", password_buf, sizeof(password_buf), ImGuiInputTextFlags_Password);

        ImGui::Spacing();
        if (!error_message.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", error_message.c_str());
        }

        ImGui::Spacing();
        if (ImGui::Button("Sign In", ImVec2(130.0f, 30.0f))) {
            std::string user(username_buf);
            std::string pass(password_buf);

            if (user.empty() || pass.empty()) {
                error_message = "Fields cannot be empty!";
            } else if (db.verify_user(user, pass, authenticated_player)) {
                error_message = "";
                switch_to_game = true;
            } else {
                error_message = "Invalid username or password!";
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Sign Up", ImVec2(130.0f, 30.0f))) {
            std::string user(username_buf);
            std::string pass(password_buf);

            uint32_t new_id = 0;
            if (user.empty() || pass.empty()) {
                error_message = "Fields cannot be empty!";
            } else if (db.register_user(user, pass, new_id)) {
                authenticated_player.id = new_id;
                authenticated_player.username = user;
                authenticated_player.password_hash = pass;
                error_message = "";
                switch_to_game = true;
            } else {
                error_message = "Username already taken!";
            }
        }
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
