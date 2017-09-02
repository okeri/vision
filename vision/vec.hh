#pragma once

template <typename T>
struct Vec2 {
    T x;
    T y;
    inline Vec2 operator*(Vec2 v) {
        x *= v.x;
        y *= v.y;
        return *this;
    }
    inline Vec2 operator*(T v) {
        x *= v;
        y *= v;
        return *this;
    }
    inline Vec2 operator+(Vec2 v) {
        x += v.x;
        y += v.y;
        return *this;
    }
    inline Vec2 operator+(T v) {
        x += v;
        y += v;
        return *this;
    }
};

template <typename T>
struct Vec3 : public Vec2<T> {
    T z;
    inline Vec3 operator*(T v) {
        Vec2<T>::operator*(v);
        z *= v;
        return *this;
    }
    inline Vec3 operator*(Vec3 v) {
        Vec2<T>::operator*(v);
        z *= v.z;
        return *this;
    }
    inline Vec3 operator+(T v) {
        Vec2<T>::operator+(v);
        z += v;
        return *this;
    }
    inline Vec3 operator+(Vec3 v) {
        Vec2<T>::operator+(v);
        z += v.z;
        return *this;
    }
};

using Vec2i = Vec2<int>;
using Vec3i = Vec3<int>;
