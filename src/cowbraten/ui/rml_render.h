#ifndef COWBRATEN_UI_RML_RENDER_H
#define COWBRATEN_UI_RML_RENDER_H

#include <RmlUi/Core/RenderInterface.h>

namespace cowui
{
    class RmlRenderInterface final : public Rml::RenderInterface
    {
    public:
        RmlRenderInterface() = default;
        ~RmlRenderInterface() override = default;

        bool initgl();
        void shutdowngl();

        void beforeglreset();
        void afterglreset();

        void beginframe();
        void endframe();

        bool ready() const { return gl_ready; }

        Rml::CompiledGeometryHandle CompileGeometry(
            Rml::Span<const Rml::Vertex> vertices,
            Rml::Span<const int> indices) override;

        void RenderGeometry(
            Rml::CompiledGeometryHandle geometry,
            Rml::Vector2f translation,
            Rml::TextureHandle texture) override;

        void ReleaseGeometry(
            Rml::CompiledGeometryHandle geometry) override;

        Rml::TextureHandle LoadTexture(
            Rml::Vector2i &texture_dimensions,
            const Rml::String &source) override;

        Rml::TextureHandle GenerateTexture(
            Rml::Span<const Rml::byte> source,
            Rml::Vector2i source_dimensions) override;

        void ReleaseTexture(
            Rml::TextureHandle texture) override;

        void EnableScissorRegion(bool enable) override;

        void SetScissorRegion(
            Rml::Rectanglei region) override;

        void setviewport(int width, int height);

    private:
        struct Geometry;
        struct Texture;

        bool createsystemresources();
        void deletesystemresources();

        bool uploadgeometry(Geometry *geometry);
        void releasegpu(Geometry *geometry);

        bool uploadtexture(Texture *texture);
        void releasegputexture(Texture *texture);

        Geometry *geometries = nullptr;
        Texture *textures = nullptr;

        unsigned int program = 0;

        int uniform_viewport = -1;
        int uniform_translation = -1;
        int uniform_texture = -1;
        int uniform_use_texture = -1;

        int viewport_width = 1;
        int viewport_height = 1;

        bool gl_ready = false;

        struct SavedState
        {
            bool valid = false;

            bool blend = false;
            bool depth_test = false;
            bool cull_face = false;
            bool scissor_test = false;

            int program = 0;
            int vao = 0;
            int array_buffer = 0;
            int element_array_buffer = 0;

            int active_texture = 0;
            int texture_binding_2d = 0;

            int blend_src_rgb = 0;
            int blend_dst_rgb = 0;
            int blend_src_alpha = 0;
            int blend_dst_alpha = 0;

            int viewport[4] = {0, 0, 0, 0};
            int scissor_box[4] = {0, 0, 0, 0};
        };

        SavedState saved_state;
    };
}

#endif
