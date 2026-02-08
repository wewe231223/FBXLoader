#pragma once

#include <string>

namespace asset
{
    class Texture2D
    {
    public:
        Texture2D() = default;
        ~Texture2D();

        Texture2D(const Texture2D&) = delete;
        Texture2D& operator=(const Texture2D&) = delete;

        Texture2D(Texture2D&& other) noexcept;
        Texture2D& operator=(Texture2D&& other) noexcept;

        bool LoadFromFile(const std::string& path, bool srgb = false);
        void Bind(unsigned int unit) const;

        unsigned int Id() const;

    private:
        unsigned int m_id{0};
        int m_width{0};
        int m_height{0};
        int m_channels{0};
    };
} // namespace gfx
