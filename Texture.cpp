#include "Texture.h"

#include <glad/glad.h>
#include <cstdint>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace asset
{
    Texture2D::~Texture2D()
    {
        if (m_id != 0)
        {
            glDeleteTextures(1, &m_id);
            m_id = 0;
        }
    }

    Texture2D::Texture2D(Texture2D&& other) noexcept
        : m_id(other.m_id)
        , m_width(other.m_width)
        , m_height(other.m_height)
        , m_channels(other.m_channels)
    {
        other.m_id = 0;
        other.m_width = 0;
        other.m_height = 0;
        other.m_channels = 0;
    }

    Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
    {
        if (this != &other)
        {
            if (m_id != 0)
            {
                glDeleteTextures(1, &m_id);
            }
            m_id = other.m_id;
            m_width = other.m_width;
            m_height = other.m_height;
            m_channels = other.m_channels;

            other.m_id = 0;
            other.m_width = 0;
            other.m_height = 0;
            other.m_channels = 0;
        }
        return *this;
    }

    bool Texture2D::LoadFromFile(const std::string& path, bool srgb)
    {
        stbi_set_flip_vertically_on_load(1);

        unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &m_channels, 0);
        if (!data)
        {
            return false;
        }

        GLenum srcFormat = GL_RGBA;
        GLenum internalFormat = GL_RGBA8;

        if (m_channels == 1)
        {
            srcFormat = GL_RED;
            internalFormat = GL_R8;
        }
        else if (m_channels == 3)
        {
            srcFormat = GL_RGB;
            internalFormat = srgb ? GL_SRGB8 : GL_RGB8;
        }
        else if (m_channels == 4)
        {
            srcFormat = GL_RGBA;
            internalFormat = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        }

        if (m_id == 0)
        {
            glGenTextures(1, &m_id);
        }

        glBindTexture(GL_TEXTURE_2D, m_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, srcFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(data);
        return true;
    }

    void Texture2D::Bind(unsigned int unit) const
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_id);
    }

    unsigned int Texture2D::Id() const
    {
        return m_id;
    }
} // namespace gfx
