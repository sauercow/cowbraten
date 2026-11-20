#pragma once

#include "rml_render.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/StringUtilities.h>

#include <algorithm>
#include <string>

namespace cowui
{
    // Synchronous loading presentation. Engine loading remains blocking, so
    // this document is explicitly updated and rendered from renderprogress().
    class RmlLoading final
    {
    public:
        bool init(Rml::Context *ctx)
        {
            if(!ctx) return false;
            document = ctx->LoadDocument(
                "data/cowbraten/ui/default/screens/loading.rml");
            if(!document) return false;

            root = document->GetElementById("cow-loading-root");
            caption_element = document->GetElementById("cow-loading-caption");
            progress_card = document->GetElementById("cow-loading-progress-card");
            progress_text = document->GetElementById("cow-loading-progress-text");
            progress_fill = document->GetElementById("cow-loading-progress-fill");
            map_card = document->GetElementById("cow-loading-map-card");
            mapshot_element = document->GetElementById("cow-loading-mapshot");
            mapshot_fallback = document->GetElementById("cow-loading-mapshot-fallback");
            map_name_element = document->GetElementById("cow-loading-map-name");
            map_info_element = document->GetElementById("cow-loading-map-info");

            if(!root || !caption_element || !progress_card || !progress_text ||
               !progress_fill || !map_card || !mapshot_element ||
               !mapshot_fallback || !map_name_element || !map_info_element)
            {
                shutdown();
                return false;
            }

            document->Hide();
            return true;
        }

        void shutdown()
        {
            if(document) document->Close();
            document = nullptr;
            root = caption_element = progress_card = progress_text = nullptr;
            progress_fill = map_card = mapshot_element = mapshot_fallback = nullptr;
            map_name_element = map_info_element = nullptr;
            caption.clear();
            mapshot.clear();
            map_name.clear();
            map_info.clear();
        }

        Rml::ElementDocument *getdocument() const { return document; }

        bool background(Rml::Context *ctx, RmlRenderInterface &renderer,
            const char *next_caption, const char *next_mapshot,
            const char *next_map_name, const char *next_map_info)
        {
            caption = next_caption ? next_caption : "";
            mapshot = next_mapshot ? next_mapshot : "";
            map_name = next_map_name ? next_map_name : "";
            map_info = next_map_info ? next_map_info : "";
            return present(ctx, renderer, false, false, 0.0f, nullptr);
        }

        bool progress(Rml::Context *ctx, RmlRenderInterface &renderer,
            float value, const char *text, bool full_background)
        {
            return present(ctx, renderer, true, !full_background, value, text);
        }

    private:
        static void settext(Rml::Element *element, const std::string &value)
        {
            if(!element) return;
            const Rml::String encoded = Rml::StringUtilities::EncodeRml(value);
            if(element->GetInnerRML() != encoded) element->SetInnerRML(encoded);
        }

        bool present(Rml::Context *ctx, RmlRenderInterface &renderer,
            bool show_progress, bool compact, float value, const char *text)
        {
            if(!ctx || !document || !renderer.ready()) return false;

            root->SetClass("cow-loading-compact", compact);
            caption_element->SetProperty("display", compact || caption.empty()
                ? "none" : "block");
            settext(caption_element, caption.empty() ? "LOADING" : caption);

            progress_card->SetProperty("display", show_progress ? "block" : "none");
            settext(progress_text, text && text[0] ? text : "Working...");
            const float percent = std::max(0.0f, std::min(value, 1.0f))*100.0f;
            progress_fill->SetProperty("width", Rml::CreateString("%.2f%%", percent));
            progress_fill->SetClass("cow-loading-indeterminate", value <= 0.0f);

            const bool have_map = !map_name.empty() || !map_info.empty() ||
                !mapshot.empty();
            map_card->SetProperty("display", !compact && have_map ? "flex" : "none");
            mapshot_element->SetProperty("display", mapshot.empty() ? "none" : "block");
            mapshot_fallback->SetProperty("display", mapshot.empty() ? "flex" : "none");
            if(!mapshot.empty())
            {
                const Rml::String resource = mapshot[0] == '?'
                    ? mapshot
                    : Rml::String("?cowbraten-resource:") + mapshot;
                mapshot_element->SetAttribute("src", resource);
            }
            settext(map_name_element, map_name);
            settext(map_info_element, map_info);

            document->Show();
            document->PullToFront();
            ctx->Update(false);
            renderer.beginframe();
            ctx->Render();
            renderer.endframe();
            document->Hide();
            return true;
        }

        Rml::ElementDocument *document = nullptr;
        Rml::Element *root = nullptr;
        Rml::Element *caption_element = nullptr;
        Rml::Element *progress_card = nullptr;
        Rml::Element *progress_text = nullptr;
        Rml::Element *progress_fill = nullptr;
        Rml::Element *map_card = nullptr;
        Rml::Element *mapshot_element = nullptr;
        Rml::Element *mapshot_fallback = nullptr;
        Rml::Element *map_name_element = nullptr;
        Rml::Element *map_info_element = nullptr;
        std::string caption, mapshot, map_name, map_info;
    };
}
