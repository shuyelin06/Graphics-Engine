#include "ImGui.h"

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace ImGuiHelper {
struct ImGuiNode {
    std::string name;
    std::unordered_map<std::string, ImGuiNode> children;
    std::function<void(void)> callback = nullptr;
    bool active;
};

static ImGuiNode root = ImGuiNode();
static std::vector<ImGuiNode*> activeNodes = std::vector<ImGuiNode*>();

static std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> tokens;
    std::string token;
    std::istream_iterator<char> it;
    std::stringstream ss(path);

    while (std::getline(ss, token, '/')) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

void registerImGuiCallback(const std::string& path,
                           std::function<void(void)> callback) {
#if defined(IMGUI_ENABLED)
    std::vector<std::string> delim = splitPath(path);
    ImGuiNode* node = &root;

    for (const std::string& key : delim) {
        node = &(node->children[key]);
        node->name = key;
    }

    node->callback = callback;
#endif
}

static void traverseNodeHierarchy(ImGuiNode* node) {
#if defined(IMGUI_ENABLED)
    for (auto& [name, child] : node->children) {
        if (child.callback != nullptr) {
            // It's an end-item, render as a selectable MenuItem
            if (ImGui::MenuItem(name.c_str())) {
                if (!child.active) {
                    child.active = true;
                    activeNodes.push_back(&child);
                }
            }
        } else {
            // It has children, render as a cascading sub-menu
            if (ImGui::BeginMenu(name.c_str())) {
                traverseNodeHierarchy(&child);
                ImGui::EndMenu();
            }
        }
    }
#endif
}

void renderImGui() {
#if defined(IMGUI_ENABLED)
    traverseNodeHierarchy(&root);

    std::vector<ImGuiNode*>::iterator iter = activeNodes.begin();
    for (auto iter = activeNodes.begin(); iter != activeNodes.end();) {
        bool remove = false;

        ImGuiNode* node = *iter;
        if (node->active) {
            if (ImGui::Begin(node->name.c_str(), &node->active,
                             ImGuiWindowFlags_NoCollapse)) {
                node->callback(); // Execute the assigned callback
            }
            ImGui::End();
        } else {
            remove = true;
        }

        if (remove) {
            iter = activeNodes.erase(iter);
        } else {
            ++iter;
        }
    }
#endif
}

} // namespace ImGuiHelper