#include "rage.hpp"
#include "../../widgets/custom_widgets.hpp"
#include "../../gui/gui/gui.hpp"

namespace features::rage {

    void render() {
        float full_w = ImGui::GetContentRegionAvail().x;
        float col_w = (full_w - 6.0f) * 0.5f;

        ImGui::BeginGroup();
        ImGui::EndGroup();

        ImGui::SameLine(0.0f, 6.0f);

        ImGui::BeginGroup();
        ImGui::EndGroup();
    }

}
