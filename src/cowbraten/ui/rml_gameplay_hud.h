#pragma once

#include <RmlUi/Core.h>
#include "gameplay_hud_snapshot.h"

// Gameplay status dock: presentation only. fps.cpp is the single source of
// health, armour, ammo, selected weapon, spectator/death and quad state.
namespace cowui
{
    class RmlGameplayHud final
    {
    public:
        bool init(Rml::Context *ctx)
        {
            if(!ctx) return false;
            context = ctx;
            document = ctx->LoadDocument(
                "data/cowbraten/ui/default/screens/gameplay_hud.rml");
            if(!document) { context = nullptr; return false; }
            root = document->GetElementById("cow-status-root");
            ammo_bar_root = document->GetElementById("cow-ammo-bar-root");
            health_value = document->GetElementById("cow-status-health-value");
            health_max = document->GetElementById("cow-status-health-max");
            armour_value = document->GetElementById("cow-status-armour-value");
            ammo_value = document->GetElementById("cow-status-ammo-value");
            armour_card = document->GetElementById("cow-status-armour");
            ammo_card = document->GetElementById("cow-status-ammo");
            health_card = document->GetElementById("cow-status-health");
            for(int i = 0; i < 8; ++i)
                weapon_images[i] = document->GetElementById(
                    Rml::CreateString("cow-status-gun-%d", i));
            secondary_up_group = document->GetElementById(
                "cow-status-secondary-up");
            secondary_down_group = document->GetElementById(
                "cow-status-secondary-down");
            secondary_cycle_group = document->GetElementById(
                "cow-status-secondary-cycle");
            for(int i = 0; i < GameplayHudSnapshot::SecondaryUpSlots; ++i)
                secondary_up[i] = document->GetElementById(
                    Rml::CreateString("cow-status-up-%d", i));
            for(int i = 0; i < GameplayHudSnapshot::SecondaryDownSlots; ++i)
                secondary_down[i] = document->GetElementById(
                    Rml::CreateString("cow-status-down-%d", i));
            for(int i = 0; i < GameplayHudSnapshot::SecondaryCycleSlots; ++i)
                secondary_cycle[i] = document->GetElementById(
                    Rml::CreateString("cow-status-cycle-%d", i));
            for(int i = 0; i < GameplayHudSnapshot::AmmoBarSlots; ++i)
            {
                ammo_bar_items[i] = document->GetElementById(
                    Rml::CreateString("cow-ammo-bar-item-%d", i));
                ammo_bar_counts[i] = document->GetElementById(
                    Rml::CreateString("cow-ammo-bar-count-%d", i));
            }
            quad_card = document->GetElementById("cow-status-quad");
            recording_indicator = document->GetElementById(
                "cow-demo-recording");
            mode_card = document->GetElementById("cow-status-mode");
            mode_icon = document->GetElementById("cow-status-mode-icon");
            mode_count = document->GetElementById("cow-status-mode-count");
            const bool valid = root && health_value && health_max &&
                armour_value && ammo_value && armour_card && ammo_card &&
                health_card && quad_card && recording_indicator && mode_card &&
                mode_icon && mode_count &&
                ammo_bar_root && secondary_up_group &&
                secondary_down_group && secondary_cycle_group;
            bool all_weapons = true;
            for(int i = 0; i < 8; ++i)
                all_weapons = all_weapons && weapon_images[i];
            for(int i = 0; i < GameplayHudSnapshot::SecondaryUpSlots; ++i)
                all_weapons = all_weapons && secondary_up[i];
            for(int i = 0; i < GameplayHudSnapshot::SecondaryDownSlots; ++i)
                all_weapons = all_weapons && secondary_down[i];
            for(int i = 0; i < GameplayHudSnapshot::SecondaryCycleSlots; ++i)
                all_weapons = all_weapons && secondary_cycle[i];
            for(int i = 0; i < GameplayHudSnapshot::AmmoBarSlots; ++i)
                all_weapons = all_weapons && ammo_bar_items[i] &&
                    ammo_bar_counts[i];
            if(!valid || !all_weapons)
            {
                shutdown(ctx);
                return false;
            }
            document->SetProperty("font-family", "Lato");
            root->SetProperty("display", "none");
            ammo_bar_root->SetProperty("display", "none");
            document->Show();
            return true;
        }

        void sync(const GameplayHudSnapshot &next)
        {
            snapshot = next;
            have_snapshot = true;
        }

        void update(bool allow_overlay)
        {
            if(!document || !root) return;
            if(have_applied_snapshot &&
               applied_had_snapshot == have_snapshot &&
               applied_allow_overlay == allow_overlay &&
               applied_snapshot == snapshot)
                return;

            applied_snapshot = snapshot;
            applied_had_snapshot = have_snapshot;
            applied_allow_overlay = allow_overlay;
            have_applied_snapshot = true;
            const bool status_shown = allow_overlay && have_snapshot &&
                snapshot.enabled && snapshot.show;
            const bool ammo_bar_shown = status_shown && snapshot.alive &&
                snapshot.show_ammo_bar;
            const bool recording_shown = allow_overlay && have_snapshot &&
                snapshot.enabled && snapshot.demo_recording;
            shown = status_shown || ammo_bar_shown || recording_shown;
            root->SetProperty("display", status_shown ? "flex" : "none");
            ammo_bar_root->SetProperty("display", ammo_bar_shown ? "block" : "none");
            recording_indicator->SetProperty("display",
                recording_shown ? "block" : "none");
            recording_indicator->SetProperty("opacity",
                snapshot.demo_recording_bright ? "1" : "0.47");
            if(!status_shown) return;

            // In Sauer's virtual HUD, statusdockcenter(w,h) is expressed in
            // (1800*w/h) units. Center the complete RmlUi row at that exact
            // normalized X location without using percentage transforms.
            root->SetProperty("left", Rml::CreateString("%.3f%%",
                (snapshot.x - 0.5f)*100.0f));
            root->SetProperty("top", Rml::CreateString("%.3f%%",
                snapshot.y*100.0f));

            // Scale from the RmlUi context height, not hard-coded 1080p.
            // The cards' dimensions use em relative to this base.
            const int height = context ? context->GetDimensions().y : 1080;
            const float font_px = (height > 0 ? height : 1080)*0.020f;
            root->SetProperty("font-size", Rml::CreateString("%.2fpx", font_px));

            settext(health_value, Rml::CreateString("%d", snapshot.health));
            settext(health_max, Rml::CreateString("/%d", snapshot.max_health));
            health_max->SetProperty("display", snapshot.alive &&
                snapshot.max_health > 100 ? "inline" : "none");
            settext(armour_value, Rml::CreateString("%d", snapshot.armour));
            settext(ammo_value, Rml::CreateString("%d", snapshot.ammo));

            armour_card->SetProperty("display", snapshot.alive ? "flex" : "none");
            ammo_card->SetProperty("display", snapshot.alive ? "flex" : "none");
            quad_card->SetProperty("display", snapshot.alive && snapshot.quad
                ? "flex" : "none");
            const bool mode_shown = snapshot.alive && snapshot.mode_icon > 0;
            mode_card->SetProperty("display", mode_shown ? "flex" : "none");
            if(mode_shown)
            {
                static const char *resources[] =
                {
                    "", "../images/hud/v1/flag_blue.png",
                    "../images/hud/v1/flag_red.png",
                    "../images/hud/v1/flag_neutral.png",
                    "../images/hud/v1/suicide.png"
                };
                const int icon = snapshot.mode_icon >= 1 &&
                    snapshot.mode_icon <= 4 ? snapshot.mode_icon : 0;
                mode_icon->SetAttribute("src", resources[icon]);
                mode_card->SetClass("cow-mode-blue", icon == 1);
                mode_card->SetClass("cow-mode-red", icon == 2);
                mode_card->SetClass("cow-mode-neutral", icon == 3);
                mode_card->SetClass("cow-mode-token", icon == 4);
                mode_count->SetProperty("display",
                    snapshot.mode_count > 0 ? "block" : "none");
                settext(mode_count, snapshot.mode_count > 0
                    ? Rml::CreateString("%d", snapshot.mode_count) : "");
            }

            health_card->SetClass("cow-status-warning", snapshot.health_tone == 1);
            health_card->SetClass("cow-status-critical", snapshot.health_tone == 2);
            health_card->SetClass("cow-status-boosted", snapshot.health_tone == 3);
            armour_card->SetClass("cow-status-empty", snapshot.armour == 0);
            // Sauer enums: A_BLUE=0, A_GREEN=1, A_YELLOW=2.
            armour_card->SetClass("cow-armour-blue", snapshot.armour_type == 0);
            armour_card->SetClass("cow-armour-green", snapshot.armour_type == 1);
            armour_card->SetClass("cow-armour-yellow", snapshot.armour_type == 2);
            ammo_card->SetClass("cow-status-warning", snapshot.ammo_tone == 1);
            ammo_card->SetClass("cow-status-critical", snapshot.ammo_tone == 2);

            const int active_gun = snapshot.gun >= 0 && snapshot.gun <= 6
                ? snapshot.gun : 7;
            for(int i = 0; i < 8; ++i)
                weapon_images[i]->SetProperty("display",
                    active_gun == i ? "inline-block" : "none");

            const bool secondary = snapshot.alive &&
                snapshot.show_secondary_ammo;
            secondary_up_group->SetProperty("display", secondary ? "flex" : "none");
            secondary_down_group->SetProperty("display", secondary ? "flex" : "none");
            secondary_cycle_group->SetProperty("display", secondary ? "flex" : "none");
            for(int i = 0; i < GameplayHudSnapshot::SecondaryUpSlots; ++i)
                setweapon(secondary_up[i], snapshot.secondary_up[i], secondary);
            for(int i = 0; i < GameplayHudSnapshot::SecondaryDownSlots; ++i)
                setweapon(secondary_down[i], snapshot.secondary_down[i], secondary);
            for(int i = 0; i < GameplayHudSnapshot::SecondaryCycleSlots; ++i)
                setweapon(secondary_cycle[i], snapshot.secondary_cycle[i], secondary);

            if(ammo_bar_shown) updateammobar();
        }

        bool visible() const { return shown; }
        bool ready() const { return document != nullptr; }

        void shutdown(Rml::Context *)
        {
            if(document) document->Close();
            document = nullptr;
            context = nullptr;
            root = ammo_bar_root = health_value = health_max = armour_value =
                ammo_value = nullptr;
            armour_card = ammo_card = health_card = quad_card = nullptr;
            recording_indicator = nullptr;
            mode_card = mode_icon = mode_count = nullptr;
            secondary_up_group = secondary_down_group =
                secondary_cycle_group = nullptr;
            for(int i = 0; i < 8; ++i) weapon_images[i] = nullptr;
            for(int i = 0; i < GameplayHudSnapshot::SecondaryUpSlots; ++i)
                secondary_up[i] = nullptr;
            for(int i = 0; i < GameplayHudSnapshot::SecondaryDownSlots; ++i)
                secondary_down[i] = nullptr;
            for(int i = 0; i < GameplayHudSnapshot::SecondaryCycleSlots; ++i)
                secondary_cycle[i] = nullptr;
            for(int i = 0; i < GameplayHudSnapshot::AmmoBarSlots; ++i)
                ammo_bar_items[i] = ammo_bar_counts[i] = nullptr;
            shown = false;
            have_snapshot = false;
            have_applied_snapshot = false;
        }

    private:
        static void settext(Rml::Element *element, const Rml::String &text)
        {
            if(element && element->GetInnerRML() != text)
                element->SetInnerRML(text);
        }

        static void setweapon(Rml::Element *element, int gun, bool enabled)
        {
            static const char *resources[] =
            {
                "../images/hud/v1/fist.png",
                "../images/hud/v1/shotgun.png",
                "../images/hud/v1/chaingun.png",
                "../images/hud/v1/rocket_launcher.png",
                "../images/hud/v1/rifle.png",
                "../images/hud/v1/grenade_launcher.png",
                "../images/hud/v1/pistol.png"
            };
            const bool visible = enabled && gun >= 0 && gun < 7;
            element->SetProperty("display", visible ? "block" : "none");
            if(visible) element->SetAttribute("src", resources[gun]);
        }

        void updateammobar()
        {
            const float icon = snapshot.ammo_bar_icon_size;
            const float label_height = snapshot.ammo_bar_label_size*1.25f;
            const float distance = icon*0.5f + snapshot.ammo_bar_label_gap +
                label_height*0.5f;
            for(int i = 0; i < GameplayHudSnapshot::AmmoBarSlots; ++i)
            {
                Rml::Element *item = ammo_bar_items[i];
                Rml::Element *count = ammo_bar_counts[i];
                const bool visible = snapshot.ammo_bar_visible[i];
                item->SetProperty("display", visible ? "block" : "none");
                if(!visible) continue;
                item->SetProperty("left", Rml::CreateString("%.2fpx",
                    snapshot.ammo_bar_x[i] - icon*0.5f));
                item->SetProperty("top", Rml::CreateString("%.2fpx",
                    snapshot.ammo_bar_y[i] - icon*0.5f));
                item->SetProperty("width", Rml::CreateString("%.2fpx", icon));
                item->SetProperty("height", Rml::CreateString("%.2fpx", icon));
                item->SetClass("cow-ammo-bar-warning",
                    snapshot.ammo_bar_tone[i] == 1);
                item->SetClass("cow-ammo-bar-critical",
                    snapshot.ammo_bar_tone[i] == 2);
                item->SetClass("cow-ammo-bar-empty",
                    snapshot.ammo_bar_count[i] == 0);
                settext(count, Rml::CreateString("%d",
                    snapshot.ammo_bar_count[i]));
                count->SetProperty("font-size", Rml::CreateString("%.2fpx",
                    snapshot.ammo_bar_label_size));
                count->SetProperty("width", Rml::CreateString("%.2fpx", icon*2.0f));
                count->SetProperty("height", Rml::CreateString("%.2fpx", label_height));
                if(snapshot.ammo_bar_horizontal)
                {
                    count->SetProperty("left", Rml::CreateString("%.2fpx", -icon*0.5f));
                    const float center = icon*0.5f +
                        (snapshot.ammo_bar_flip ? -distance : distance);
                    count->SetProperty("top", Rml::CreateString("%.2fpx",
                        center - label_height*0.5f));
                }
                else
                {
                    const float center = icon*0.5f +
                        (snapshot.ammo_bar_flip ? -distance : distance);
                    count->SetProperty("left", Rml::CreateString("%.2fpx",
                        center - icon));
                    count->SetProperty("top", Rml::CreateString("%.2fpx",
                        icon*0.5f - label_height*0.5f));
                }
            }
        }

        Rml::Context *context = nullptr;
        Rml::ElementDocument *document = nullptr;
        Rml::Element *root = nullptr;
        Rml::Element *ammo_bar_root = nullptr;
        Rml::Element *health_value = nullptr, *health_max = nullptr;
        Rml::Element *armour_value = nullptr, *ammo_value = nullptr;
        Rml::Element *health_card = nullptr, *armour_card = nullptr;
        Rml::Element *ammo_card = nullptr;
        Rml::Element *quad_card = nullptr;
        Rml::Element *recording_indicator = nullptr;
        Rml::Element *mode_card = nullptr, *mode_icon = nullptr;
        Rml::Element *mode_count = nullptr;
        Rml::Element *secondary_up_group = nullptr;
        Rml::Element *secondary_down_group = nullptr;
        Rml::Element *secondary_cycle_group = nullptr;
        GameplayHudSnapshot snapshot;
        GameplayHudSnapshot applied_snapshot;
        bool have_snapshot = false;
        bool have_applied_snapshot = false;
        bool applied_had_snapshot = false;
        bool applied_allow_overlay = false;
        bool shown = false;
        Rml::Element *weapon_images[8] = {};
        Rml::Element *secondary_up[GameplayHudSnapshot::SecondaryUpSlots] = {};
        Rml::Element *secondary_down[GameplayHudSnapshot::SecondaryDownSlots] = {};
        Rml::Element *secondary_cycle[GameplayHudSnapshot::SecondaryCycleSlots] = {};
        Rml::Element *ammo_bar_items[GameplayHudSnapshot::AmmoBarSlots] = {};
        Rml::Element *ammo_bar_counts[GameplayHudSnapshot::AmmoBarSlots] = {};
    };
}
