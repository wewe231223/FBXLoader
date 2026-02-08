#include "NumericTypes.h"

namespace asset {
    Vec2::Vec2()
        : mX{ 0.0f },
		mY{ 0.0f } {
    }

    Vec2::Vec2(float X, float Y)
        : mX{ X },
		mY{ Y } {
    }

    Vec3::Vec3()
        : mX{ 0.0f },
		mY{ 0.0f },
		mZ{ 0.0f } {
    }

    Vec3::Vec3(float X, float Y, float Z)
        : mX{ X },
		mY{ Y },
		mZ{ Z } {
    }

    Vec4::Vec4()
        : mX{ 0.0f },
		mY{ 0.0f },
		mZ{ 0.0f },
		mW{ 0.0f } {
    }

    Vec4::Vec4(float X, float Y, float Z, float W)
        : mX{ X },
		mY{ Y },
		mZ{ Z },
		mW{ W } {
    }

    UVec4::UVec4()
        : mX{ 0 },
		mY{ 0 },
		mZ{ 0 },
		mW{ 0 } {
    }

    UVec4::UVec4(std::uint32_t X, std::uint32_t Y, std::uint32_t Z, std::uint32_t W)
        : mX{ X },
		mY{ Y },
		mZ{ Z },
		mW{ W } {
    }

    Mat4::Mat4()
        : mValue{} {
    }

    Mat4::Mat4(float Diagonal)
        : mValue{} {
        mValue[0][0] = Diagonal;
        mValue[1][1] = Diagonal;
        mValue[2][2] = Diagonal;
        mValue[3][3] = Diagonal;
    }

    float* Mat4::Data() {
        return &mValue[0][0];
    }

    const float* Mat4::Data() const {
        return &mValue[0][0];
    }
}
