//=================================================
// This Collision Class Contains AABB + Sphere + OBB
//=================================================
#pragma once

#include <pch.h>
#include <SimpleMath.h>
#include <algorithm>
#include <float.h>
#include <MyLib/Utilities/Ray.h>

namespace MyLib
{
    using namespace DirectX::SimpleMath;

    //====================================
    // Sphere Collider
    //====================================
    class SphereCollider
    {
    private:
        Vector3 m_center;
        float   m_radius;

    public:
        SphereCollider() : m_center(0, 0, 0), m_radius(1.0f) {}
        SphereCollider(const Vector3& center, float radius)
            : m_center(center), m_radius(radius) {}

        void SetCenter(const Vector3& c) { m_center = c; }
        void SetRadius(float r) { m_radius = r; }

        Vector3 GetCenter() const { return m_center; }
        float   GetRadius() const { return m_radius; }
    };


    //====================================
    // AABB Box Collider
    //====================================
    class BoxCollider3D
    {
    private:
        Vector3 m_center;
        Vector3 m_halfSize;

    public:
        BoxCollider3D()
            : m_center(0, 0, 0), m_halfSize(0.5f, 0.5f, 0.5f) {}

        BoxCollider3D(const Vector3& center, const Vector3& half)
            : m_center(center), m_halfSize(half) {}

        void SetCenter(const Vector3& c) { m_center = c; }
        void SetHalfSize(const Vector3& h) { m_halfSize = h; }

        Vector3 GetCenter()   const { return m_center; }
        Vector3 GetHalfSize() const { return m_halfSize; }
    };


    //====================================
    // OBB Collider (rotatable box)
    //====================================
    class OBBCollider
    {
    private:
        Vector3 m_center;
        Vector3 m_halfSize;
        Matrix  m_orientation;

    public:
        OBBCollider();
        OBBCollider(const Vector3& center,
            const Vector3& halfSize,
            const Matrix& orientation);

        void SetCenter(const Vector3& c) { m_center = c; }
        void SetHalfSize(const Vector3& h) { m_halfSize = h; }
        void SetOrientation(const Matrix& m) { m_orientation = m; }

        Vector3 GetCenter() const { return m_center; }
        Vector3 GetHalfSize() const { return m_halfSize; }
        Matrix  GetOrientation() const { return m_orientation; }
    };


    //====================================
    // Collision tests
    //====================================
    bool IsHit(const SphereCollider& a, const SphereCollider& b);
    bool IsHit(const BoxCollider3D& a, const BoxCollider3D& b);

    bool IsHit(const Ray& ray, const SphereCollider& sphere, float* outDistance = nullptr);
    bool IsHit(const Ray& ray, const BoxCollider3D& box, float* outDistance = nullptr);

    Vector3 CalculateOverlap(const BoxCollider3D& a, const BoxCollider3D& b);

    // OBB tests
    bool IsHit(const OBBCollider& a, const OBBCollider& b);
    bool IsHit(const Ray& ray, const OBBCollider& obb, float* outDistance = nullptr);
}