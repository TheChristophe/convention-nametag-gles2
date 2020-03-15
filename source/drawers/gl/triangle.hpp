#ifndef DRAWERS_GL_DECORATION_HPP
#define DRAWERS_GL_DECORATION_HPP

#include "drawers/gl/drawable.hpp"
#include "wrappers/shader.hpp"

#include <glm/glm.hpp>

#include <memory>

namespace Drawers::GL {
    /**
     * @brief OpenGL drawing class for background pictures (triangles, ...).
     * 
     */
    class Triangle : public Drawable {
        public:
        Triangle(std::shared_ptr<Wrappers::Shader> shader);

        void Draw(float time) final;

        void MoveTo(glm::vec2 to);

        private:
        std::shared_ptr<Wrappers::Shader> _shader;

        GLuint _vbo;

        GLuint _posLoc;
        GLuint _luminLoc;

        glm::vec2 _at;
    };
} // namespace Drawers::GL

#endif