#include <SDL.h>
#include <SDL_opengl.h>

#include "glexts.h"
#include "rml_render.h"

#include <RmlUi/Core/Vertex.h>

#include <cstddef>
#include <climits>
#include <cstdio>
#include <new>

extern int glslversion;

namespace gle
{
    extern void forcedisable();
}

namespace cowui
{
    namespace
    {
        constexpr GLuint ATTR_POSITION = 0;
        constexpr GLuint ATTR_COLOR = 1;

        static bool canusevao()
        {
            return glGenVertexArrays_ &&
                   glBindVertexArray_ &&
                   glDeleteVertexArrays_;
        }

        static GLuint compileshader(GLenum type, const char *source)
        {
            GLuint shader = glCreateShader_(type);
            if(!shader) return 0;

            glShaderSource_(shader, 1, &source, nullptr);
            glCompileShader_(shader);

            GLint status = GL_FALSE;
            glGetShaderiv_(shader, GL_COMPILE_STATUS, &status);

            if(status != GL_TRUE)
            {
                char log[4096] = {0};
                GLsizei length = 0;

                glGetShaderInfoLog_(
                    shader,
                    sizeof(log) - 1,
                    &length,
                    log
                );

                std::fprintf(
                    stderr,
                    "Cowbraten UI shader compilation failed: %s\n",
                    log
                );

                glDeleteShader_(shader);
                return 0;
            }

            return shader;
        }

        static GLuint createprogram()
        {
            static const char *vertex_330 =
                "#version 330 core\n"
                "in vec2 a_position;\n"
                "in vec4 a_color;\n"
                "uniform vec2 u_viewport;\n"
                "uniform vec2 u_translation;\n"
                "out vec4 v_color;\n"
                "void main()\n"
                "{\n"
                "    vec2 position = a_position + u_translation;\n"
                "    vec2 ndc = vec2(\n"
                "        position.x / u_viewport.x * 2.0 - 1.0,\n"
                "        1.0 - position.y / u_viewport.y * 2.0\n"
                "    );\n"
                "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
                "    v_color = a_color;\n"
                "}\n";

            static const char *fragment_330 =
                "#version 330 core\n"
                "in vec4 v_color;\n"
                "layout(location = 0) out vec4 frag_color;\n"
                "void main()\n"
                "{\n"
                "    frag_color = v_color;\n"
                "}\n";

            static const char *vertex_150 =
                "#version 150\n"
                "in vec2 a_position;\n"
                "in vec4 a_color;\n"
                "uniform vec2 u_viewport;\n"
                "uniform vec2 u_translation;\n"
                "out vec4 v_color;\n"
                "void main()\n"
                "{\n"
                "    vec2 position = a_position + u_translation;\n"
                "    vec2 ndc = vec2(\n"
                "        position.x / u_viewport.x * 2.0 - 1.0,\n"
                "        1.0 - position.y / u_viewport.y * 2.0\n"
                "    );\n"
                "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
                "    v_color = a_color;\n"
                "}\n";

            static const char *fragment_150 =
                "#version 150\n"
                "in vec4 v_color;\n"
                "out vec4 frag_color;\n"
                "void main()\n"
                "{\n"
                "    frag_color = v_color;\n"
                "}\n";

            static const char *vertex_130 =
                "#version 130\n"
                "in vec2 a_position;\n"
                "in vec4 a_color;\n"
                "uniform vec2 u_viewport;\n"
                "uniform vec2 u_translation;\n"
                "out vec4 v_color;\n"
                "void main()\n"
                "{\n"
                "    vec2 position = a_position + u_translation;\n"
                "    vec2 ndc = vec2(\n"
                "        position.x / u_viewport.x * 2.0 - 1.0,\n"
                "        1.0 - position.y / u_viewport.y * 2.0\n"
                "    );\n"
                "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
                "    v_color = a_color;\n"
                "}\n";

            static const char *fragment_130 =
                "#version 130\n"
                "in vec4 v_color;\n"
                "out vec4 frag_color;\n"
                "void main()\n"
                "{\n"
                "    frag_color = v_color;\n"
                "}\n";

            static const char *vertex_120 =
                "#version 120\n"
                "attribute vec2 a_position;\n"
                "attribute vec4 a_color;\n"
                "uniform vec2 u_viewport;\n"
                "uniform vec2 u_translation;\n"
                "varying vec4 v_color;\n"
                "void main()\n"
                "{\n"
                "    vec2 position = a_position + u_translation;\n"
                "    vec2 ndc = vec2(\n"
                "        position.x / u_viewport.x * 2.0 - 1.0,\n"
                "        1.0 - position.y / u_viewport.y * 2.0\n"
                "    );\n"
                "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
                "    v_color = a_color;\n"
                "}\n";

            static const char *fragment_120 =
                "#version 120\n"
                "varying vec4 v_color;\n"
                "void main()\n"
                "{\n"
                "    gl_FragColor = v_color;\n"
                "}\n";

            const char *vertex_source = nullptr;
            const char *fragment_source = nullptr;

            if(glslversion >= 330)
            {
                vertex_source = vertex_330;
                fragment_source = fragment_330;
            }
            else if(glslversion >= 150)
            {
                vertex_source = vertex_150;
                fragment_source = fragment_150;
            }
            else if(glslversion >= 130)
            {
                vertex_source = vertex_130;
                fragment_source = fragment_130;
            }
            else
            {
                vertex_source = vertex_120;
                fragment_source = fragment_120;
            }

            GLuint vertex_shader =
                compileshader(GL_VERTEX_SHADER, vertex_source);

            if(!vertex_shader) return 0;

            GLuint fragment_shader =
                compileshader(GL_FRAGMENT_SHADER, fragment_source);

            if(!fragment_shader)
            {
                glDeleteShader_(vertex_shader);
                return 0;
            }

            GLuint result = glCreateProgram_();

            if(!result)
            {
                glDeleteShader_(vertex_shader);
                glDeleteShader_(fragment_shader);
                return 0;
            }

            glAttachShader_(result, vertex_shader);
            glAttachShader_(result, fragment_shader);

            glBindAttribLocation_(
                result,
                ATTR_POSITION,
                "a_position"
            );

            glBindAttribLocation_(
                result,
                ATTR_COLOR,
                "a_color"
            );

            if(glBindFragDataLocation_)
                glBindFragDataLocation_(result, 0, "frag_color");

            glLinkProgram_(result);

            glDeleteShader_(vertex_shader);
            glDeleteShader_(fragment_shader);

            GLint status = GL_FALSE;
            glGetProgramiv_(result, GL_LINK_STATUS, &status);

            if(status != GL_TRUE)
            {
                char log[4096] = {0};
                GLsizei length = 0;

                glGetProgramInfoLog_(
                    result,
                    sizeof(log) - 1,
                    &length,
                    log
                );

                std::fprintf(
                    stderr,
                    "Cowbraten UI shader link failed: %s\n",
                    log
                );

                glDeleteProgram_(result);
                return 0;
            }

            return result;
        }
    }

    struct RmlRenderInterface::Geometry
    {
        const Rml::Vertex *vertices = nullptr;
        const int *indices = nullptr;

        int vertex_count = 0;
        int index_count = 0;

        GLuint vbo = 0;
        GLuint ibo = 0;
        GLuint vao = 0;

        Geometry *next = nullptr;
    };

    bool RmlRenderInterface::initgl()
    {
        if(gl_ready) return true;

        if(!createsystemresources())
            return false;

        gl_ready = true;

        for(Geometry *geometry = geometries;
            geometry;
            geometry = geometry->next)
        {
            if(!uploadgeometry(geometry))
            {
                std::fprintf(
                    stderr,
                    "Cowbraten UI failed to restore geometry\n"
                );

                beforeglreset();
                return false;
            }
        }

        return true;
    }

    void RmlRenderInterface::shutdowngl()
    {
        if(!gl_ready && !program) return;

        for(Geometry *geometry = geometries;
            geometry;
            geometry = geometry->next)
        {
            releasegpu(geometry);
        }

        deletesystemresources();
        gl_ready = false;
    }

    void RmlRenderInterface::beforeglreset()
    {
        for(Geometry *geometry = geometries;
            geometry;
            geometry = geometry->next)
        {
            releasegpu(geometry);
        }

        deletesystemresources();

        gl_ready = false;
        saved_state.valid = false;
    }

    void RmlRenderInterface::afterglreset()
    {
        if(!initgl())
        {
            std::fprintf(
                stderr,
                "Cowbraten UI OpenGL resources could not be restored\n"
            );
        }
    }

    bool RmlRenderInterface::createsystemresources()
    {
        program = createprogram();

        if(!program)
            return false;

        uniform_viewport =
            glGetUniformLocation_(program, "u_viewport");

        uniform_translation =
            glGetUniformLocation_(program, "u_translation");

        if(uniform_viewport < 0 || uniform_translation < 0)
        {
            std::fprintf(
                stderr,
                "Cowbraten UI shader uniforms could not be located\n"
            );

            deletesystemresources();
            return false;
        }

        return true;
    }

    void RmlRenderInterface::deletesystemresources()
    {
        if(program)
        {
            glDeleteProgram_(program);
            program = 0;
        }

        uniform_viewport = -1;
        uniform_translation = -1;
    }

    bool RmlRenderInterface::uploadgeometry(Geometry *geometry)
    {
        if(!geometry ||
           !geometry->vertices ||
           !geometry->indices ||
           geometry->vertex_count <= 0 ||
           geometry->index_count <= 0)
        {
            return false;
        }

        releasegpu(geometry);

        GLint old_array_buffer = 0;
        GLint old_element_buffer = 0;
        GLint old_vao = 0;

        glGetIntegerv(
            GL_ARRAY_BUFFER_BINDING,
            &old_array_buffer
        );

        glGetIntegerv(
            GL_ELEMENT_ARRAY_BUFFER_BINDING,
            &old_element_buffer
        );

        const bool use_vao = canusevao();

        if(use_vao)
        {
            glGetIntegerv(
                GL_VERTEX_ARRAY_BINDING,
                &old_vao
            );

            glGenVertexArrays_(1, &geometry->vao);
            glBindVertexArray_(geometry->vao);
        }

        glGenBuffers_(1, &geometry->vbo);
        glGenBuffers_(1, &geometry->ibo);

        if(!geometry->vbo || !geometry->ibo)
        {
            releasegpu(geometry);

            if(use_vao)
                glBindVertexArray_(old_vao);

            glBindBuffer_(
                GL_ARRAY_BUFFER,
                old_array_buffer
            );

            glBindBuffer_(
                GL_ELEMENT_ARRAY_BUFFER,
                old_element_buffer
            );

            return false;
        }

        glBindBuffer_(
            GL_ARRAY_BUFFER,
            geometry->vbo
        );

        glBufferData_(
            GL_ARRAY_BUFFER,
            GLsizeiptr(
                sizeof(Rml::Vertex) *
                geometry->vertex_count
            ),
            geometry->vertices,
            GL_STATIC_DRAW
        );

        glBindBuffer_(
            GL_ELEMENT_ARRAY_BUFFER,
            geometry->ibo
        );

        glBufferData_(
            GL_ELEMENT_ARRAY_BUFFER,
            GLsizeiptr(
                sizeof(int) *
                geometry->index_count
            ),
            geometry->indices,
            GL_STATIC_DRAW
        );

        if(use_vao)
        {
            glEnableVertexAttribArray_(ATTR_POSITION);
            glEnableVertexAttribArray_(ATTR_COLOR);

            glVertexAttribPointer_(
                ATTR_POSITION,
                2,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Rml::Vertex),
                reinterpret_cast<const void *>(
                    offsetof(Rml::Vertex, position)
                )
            );

            glVertexAttribPointer_(
                ATTR_COLOR,
                4,
                GL_UNSIGNED_BYTE,
                GL_TRUE,
                sizeof(Rml::Vertex),
                reinterpret_cast<const void *>(
                    offsetof(Rml::Vertex, colour)
                )
            );

            glBindVertexArray_(old_vao);
        }

        glBindBuffer_(
            GL_ARRAY_BUFFER,
            old_array_buffer
        );

        glBindBuffer_(
            GL_ELEMENT_ARRAY_BUFFER,
            old_element_buffer
        );

        return true;
    }

    void RmlRenderInterface::releasegpu(Geometry *geometry)
    {
        if(!geometry) return;

        if(geometry->vao && glDeleteVertexArrays_)
        {
            glDeleteVertexArrays_(
                1,
                &geometry->vao
            );

            geometry->vao = 0;
        }

        if(geometry->vbo)
        {
            glDeleteBuffers_(
                1,
                &geometry->vbo
            );

            geometry->vbo = 0;
        }

        if(geometry->ibo)
        {
            glDeleteBuffers_(
                1,
                &geometry->ibo
            );

            geometry->ibo = 0;
        }
    }

    Rml::CompiledGeometryHandle
    RmlRenderInterface::CompileGeometry(
        Rml::Span<const Rml::Vertex> vertices,
        Rml::Span<const int> indices)
    {
        if(vertices.size() == 0 ||
           indices.size() == 0 ||
           vertices.size() > INT_MAX ||
           indices.size() > INT_MAX)
        {
            return 0;
        }

        Geometry *geometry =
            new (std::nothrow) Geometry;

        if(!geometry)
            return 0;

        geometry->vertices = vertices.data();
        geometry->indices = indices.data();

        geometry->vertex_count =
            int(vertices.size());

        geometry->index_count =
            int(indices.size());

        geometry->next = geometries;
        geometries = geometry;

        if(gl_ready && !uploadgeometry(geometry))
        {
            geometries = geometry->next;
            delete geometry;
            return 0;
        }

        return reinterpret_cast<
            Rml::CompiledGeometryHandle
        >(geometry);
    }

    void RmlRenderInterface::RenderGeometry(
        Rml::CompiledGeometryHandle handle,
        Rml::Vector2f translation,
        Rml::TextureHandle texture)
    {
        if(!gl_ready || !handle)
            return;

        // Textured geometry is added in the next renderer stage.
        if(texture)
            return;

        Geometry *geometry =
            reinterpret_cast<Geometry *>(handle);

        if(!geometry->vbo || !geometry->ibo)
            return;

        glUniform2f_(
            uniform_translation,
            translation.x,
            translation.y
        );

        if(geometry->vao)
        {
            glBindVertexArray_(geometry->vao);
        }
        else
        {
            gle::forcedisable();

            glBindBuffer_(
                GL_ARRAY_BUFFER,
                geometry->vbo
            );

            glBindBuffer_(
                GL_ELEMENT_ARRAY_BUFFER,
                geometry->ibo
            );

            glEnableVertexAttribArray_(ATTR_POSITION);
            glEnableVertexAttribArray_(ATTR_COLOR);

            glVertexAttribPointer_(
                ATTR_POSITION,
                2,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Rml::Vertex),
                reinterpret_cast<const void *>(
                    offsetof(Rml::Vertex, position)
                )
            );

            glVertexAttribPointer_(
                ATTR_COLOR,
                4,
                GL_UNSIGNED_BYTE,
                GL_TRUE,
                sizeof(Rml::Vertex),
                reinterpret_cast<const void *>(
                    offsetof(Rml::Vertex, colour)
                )
            );
        }

        glDrawElements(
            GL_TRIANGLES,
            geometry->index_count,
            GL_UNSIGNED_INT,
            nullptr
        );

        if(!geometry->vao)
        {
            glDisableVertexAttribArray_(ATTR_POSITION);
            glDisableVertexAttribArray_(ATTR_COLOR);
            gle::forcedisable();
        }
    }

    void RmlRenderInterface::ReleaseGeometry(
        Rml::CompiledGeometryHandle handle)
    {
        if(!handle) return;

        Geometry *geometry =
            reinterpret_cast<Geometry *>(handle);

        Geometry **current = &geometries;

        while(*current)
        {
            if(*current == geometry)
            {
                *current = geometry->next;
                break;
            }

            current = &((*current)->next);
        }

        if(gl_ready)
            releasegpu(geometry);

        delete geometry;
    }

    Rml::TextureHandle RmlRenderInterface::LoadTexture(
        Rml::Vector2i &texture_dimensions,
        const Rml::String &)
    {
        texture_dimensions = Rml::Vector2i(0, 0);
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

    void RmlRenderInterface::EnableScissorRegion(bool enable)
    {
        if(enable)
            glEnable(GL_SCISSOR_TEST);
        else
            glDisable(GL_SCISSOR_TEST);
    }

    void RmlRenderInterface::SetScissorRegion(
        Rml::Rectanglei region)
    {
        int width = region.Width();
        int height = region.Height();

        if(width < 0) width = 0;
        if(height < 0) height = 0;

        glScissor(
            region.Left(),
            viewport_height - region.Bottom(),
            width,
            height
        );
    }

    void RmlRenderInterface::setviewport(
        int width,
        int height)
    {
        viewport_width =
            width > 0 ? width : 1;

        viewport_height =
            height > 0 ? height : 1;
    }

    void RmlRenderInterface::beginframe()
    {
        if(!gl_ready || saved_state.valid)
            return;

        saved_state.valid = true;

        saved_state.blend =
            glIsEnabled(GL_BLEND) == GL_TRUE;

        saved_state.depth_test =
            glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;

        saved_state.cull_face =
            glIsEnabled(GL_CULL_FACE) == GL_TRUE;

        saved_state.scissor_test =
            glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;

        glGetIntegerv(
            GL_CURRENT_PROGRAM,
            &saved_state.program
        );

        glGetIntegerv(
            GL_ARRAY_BUFFER_BINDING,
            &saved_state.array_buffer
        );

        glGetIntegerv(
            GL_ELEMENT_ARRAY_BUFFER_BINDING,
            &saved_state.element_array_buffer
        );

        if(canusevao())
        {
            glGetIntegerv(
                GL_VERTEX_ARRAY_BINDING,
                &saved_state.vao
            );
        }

        glGetIntegerv(
            GL_BLEND_SRC_RGB,
            &saved_state.blend_src_rgb
        );

        glGetIntegerv(
            GL_BLEND_DST_RGB,
            &saved_state.blend_dst_rgb
        );

        glGetIntegerv(
            GL_BLEND_SRC_ALPHA,
            &saved_state.blend_src_alpha
        );

        glGetIntegerv(
            GL_BLEND_DST_ALPHA,
            &saved_state.blend_dst_alpha
        );

        glGetIntegerv(
            GL_VIEWPORT,
            saved_state.viewport
        );

        glGetIntegerv(
            GL_SCISSOR_BOX,
            saved_state.scissor_box
        );

        glViewport(
            0,
            0,
            viewport_width,
            viewport_height
        );

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_SCISSOR_TEST);

        glEnable(GL_BLEND);

        if(glBlendFuncSeparate_)
        {
            glBlendFuncSeparate_(
                GL_ONE,
                GL_ONE_MINUS_SRC_ALPHA,
                GL_ONE,
                GL_ONE_MINUS_SRC_ALPHA
            );
        }
        else
        {
            glBlendFunc(
                GL_ONE,
                GL_ONE_MINUS_SRC_ALPHA
            );
        }

        glUseProgram_(program);

        glUniform2f_(
            uniform_viewport,
            float(viewport_width),
            float(viewport_height)
        );

        glUniform2f_(
            uniform_translation,
            0.0f,
            0.0f
        );
    }

    void RmlRenderInterface::endframe()
    {
        if(!saved_state.valid)
            return;

        if(canusevao())
        {
            glBindVertexArray_(
                saved_state.vao
            );
        }
        else
        {
            gle::forcedisable();
        }

        glBindBuffer_(
            GL_ARRAY_BUFFER,
            saved_state.array_buffer
        );

        glBindBuffer_(
            GL_ELEMENT_ARRAY_BUFFER,
            saved_state.element_array_buffer
        );

        glUseProgram_(
            saved_state.program
        );

        if(glBlendFuncSeparate_)
        {
            glBlendFuncSeparate_(
                saved_state.blend_src_rgb,
                saved_state.blend_dst_rgb,
                saved_state.blend_src_alpha,
                saved_state.blend_dst_alpha
            );
        }
        else
        {
            glBlendFunc(
                saved_state.blend_src_rgb,
                saved_state.blend_dst_rgb
            );
        }

        if(saved_state.blend)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);

        if(saved_state.depth_test)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);

        if(saved_state.cull_face)
            glEnable(GL_CULL_FACE);
        else
            glDisable(GL_CULL_FACE);

        glScissor(
            saved_state.scissor_box[0],
            saved_state.scissor_box[1],
            saved_state.scissor_box[2],
            saved_state.scissor_box[3]
        );

        if(saved_state.scissor_test)
            glEnable(GL_SCISSOR_TEST);
        else
            glDisable(GL_SCISSOR_TEST);

        glViewport(
            saved_state.viewport[0],
            saved_state.viewport[1],
            saved_state.viewport[2],
            saved_state.viewport[3]
        );

        saved_state.valid = false;
    }
}
