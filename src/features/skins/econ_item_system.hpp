
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace features::skins {

    enum class ItemCategory : uint8_t {
        Weapons,
        Knives,
        Gloves,
        Agents
    };

    struct ItemDef {
        int16_t def_index = 0;
        std::string name;
        std::string display_name;
        ItemCategory category = ItemCategory::Weapons;
        int team = 0; 
    };

    struct PaintKit {
        int id = 0;
        std::string name;
        std::string display_name;
        uint8_t rarity = 1;
    };

    class EconItemSystem {
    public:
        static EconItemSystem& get();

        void initialize();
        bool parse_from_game();
        void build_skin_index();

        const std::vector<ItemDef>& get_items() const { return items; }
        const std::vector<PaintKit>& get_paint_kits() const { return paint_kits; }
        std::vector<const PaintKit*> get_paint_kits_for_item(int16_t def_index);

        const ItemDef* find_item(int16_t def_index) const;
        const PaintKit* find_paint_kit(int id) const;

        std::vector<const ItemDef*> get_items_by_category(ItemCategory cat) const;

    private:
        std::vector<ItemDef> items;
        std::vector<PaintKit> paint_kits;
        std::unordered_map<int16_t, size_t> item_map;
        std::unordered_map<int, size_t> paint_kit_map;
        std::unordered_map<int16_t, std::vector<int>> item_skins_map;
        bool skin_index_built = false;
    };

}

