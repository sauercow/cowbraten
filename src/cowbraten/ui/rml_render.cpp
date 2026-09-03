#include "rml_render.h"

namespace cowui
{
    Rml::CompiledGeometryHandle RmlRenderInterface::CompileGeometry(
        Rml::Span<const Rml::Vertex>,
        Rml::Span<const int>)
    {
        return 0;
    }

    void RmlRenderInterface::RenderGeometry(
        Rml::CompiledGeometryHandle,
        Rml::Vector2f,
        Rml::TextureHandle)
    {
    }

    void RmlRenderInterface::ReleaseGeometry(
        Rml::CompiledGeometryHandle)
    {
    }

    Rml::TextureHandle RmlRenderInterface::LoadTexture(
        Rml::Vector2i &,
        const Rml::String &)
    {
        return 0;
    }

    Rml::TextureHandle RmlRenderInterface::GenerateTexture(
        Rml::Span<const Rml::byte>,
        Rml::Vector2i)
    {
        return 0;
    }

    void RmlRenderInterface::ReleaseTexture(
        Rml::TextureHandle)
    {
    }

    void RmlRenderInterface::EnableScissorRegion(bool)
    {
    }

    void RmlRenderInterface::SetScissorRegion(
        Rml::Rectanglei)
    {
    }

    void RmlRenderInterface::setviewport(int width, int height)
    {
        viewport_width = width > 0 ? width : 1;
        viewport_height = height > 0 ? height : 1;
    }
}
