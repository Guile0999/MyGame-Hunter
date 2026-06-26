#include "pch.h"
#include "Collision.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace MyLib
{
    //=========================
    // OBB Constructors
    //=========================
    OBBCollider::OBBCollider()
        : m_center(0, 0, 0),
        m_halfSize(0.5f, 0.5f, 0.5f),
        m_orientation(Matrix::Identity)
    {}

    OBBCollider::OBBCollider(const Vector3& center,
        const Vector3& halfSize,
        const Matrix& orientation)
        : m_center(center),
        m_halfSize(halfSize),
        m_orientation(orientation)
    {}


    //====================================
    // Sphere vs Sphere
    //====================================
    bool IsHit(const SphereCollider& a, const SphereCollider& b)
    {
        Vector3 d = a.GetCenter() - b.GetCenter();
        float distSq = d.LengthSquared();
        float r = a.GetRadius() + b.GetRadius();
        return distSq <= r * r;
    }


    //====================================
    // AABB vs AABB
    //====================================
    bool IsHit(const BoxCollider3D& a, const BoxCollider3D& b)
    {
        Vector3 aMin = a.GetCenter() - a.GetHalfSize();
        Vector3 aMax = a.GetCenter() + a.GetHalfSize();
        Vector3 bMin = b.GetCenter() - b.GetHalfSize();
        Vector3 bMax = b.GetCenter() + b.GetHalfSize();

        return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
            (aMin.y <= bMax.y && aMax.y >= bMin.y) &&
            (aMin.z <= bMax.z && aMax.z >= bMin.z);
    }


    //====================================
    // Ray vs Sphere
    //====================================
    bool IsHit(const Ray& ray, const SphereCollider& sphere, float* outDistance)
    {
        Vector3 m = ray.origin - sphere.GetCenter();
        float b = m.Dot(ray.direction);
        float c = m.Dot(m) - sphere.GetRadius() * sphere.GetRadius();
        if (c > 0.0f && b > 0.0f) return false;

        float disc = b * b - c;
        if (disc < 0.0f) return false;

        if (outDistance)
            *outDistance = -b - sqrtf(disc);

        return true;
    }


    //====================================
    // Ray vs AABB
    //====================================
    bool IsHit(const Ray& ray, const BoxCollider3D& box, float* outDistance)
    {
        Vector3 min = box.GetCenter() - box.GetHalfSize();
        Vector3 max = box.GetCenter() + box.GetHalfSize();

        float tMin = 0.0f;
        float tMax = FLT_MAX;

        // Manually extract components
        float ro[3] = { ray.origin.x, ray.origin.y, ray.origin.z };
        float rd[3] = { ray.direction.x, ray.direction.y, ray.direction.z };
        float bMin[3] = { min.x, min.y, min.z };
        float bMax[3] = { max.x, max.y, max.z };

        for (int i = 0; i < 3; ++i)
        {
            float origin = ro[i];
            float dir = rd[i];

            if (fabs(dir) < 1e-6f)
            {
                if (origin < bMin[i] || origin > bMax[i]) return false;
            }
            else
            {
                float inv = 1.0f / dir;
                float t0 = (bMin[i] - origin) * inv;
                float t1 = (bMax[i] - origin) * inv;
                if (inv < 0.0f) std::swap(t0, t1);

                tMin = std::max(tMin, t0);
                tMax = std::min(tMax, t1);

                if (tMax <= tMin) return false;
            }
        }

        if (outDistance)
            *outDistance = tMin;

        return true;
    }


    //====================================
    // AABB overlap
    //====================================
    Vector3 CalculateOverlap(const BoxCollider3D& a, const BoxCollider3D& b)
    {
        Vector3 aMin = a.GetCenter() - a.GetHalfSize();
        Vector3 aMax = a.GetCenter() + a.GetHalfSize();
        Vector3 bMin = b.GetCenter() - b.GetHalfSize();
        Vector3 bMax = b.GetCenter() + b.GetHalfSize();

        return Vector3(
            std::min(aMax.x - bMin.x, bMax.x - aMin.x),
            std::min(aMax.y - bMin.y, bMax.y - aMin.y),
            std::min(aMax.z - bMin.z, bMax.z - aMin.z)
        );
    }


    //====================================
    // OBB vs OBB (SAT test)
    //====================================
    bool IsHit(const OBBCollider& A, const OBBCollider& B)
    {
        Matrix R = A.GetOrientation();
        Matrix S = B.GetOrientation();

        Vector3 Ax = Vector3(R._11, R._12, R._13);
        Vector3 Ay = Vector3(R._21, R._22, R._23);
        Vector3 Az = Vector3(R._31, R._32, R._33);

        Vector3 Bx = Vector3(S._11, S._12, S._13);
        Vector3 By = Vector3(S._21, S._22, S._23);
        Vector3 Bz = Vector3(S._31, S._32, S._33);

        Vector3 Aaxis[3] = { Ax, Ay, Az };
        Vector3 Baxis[3] = { Bx, By, Bz };

        Vector3 T = B.GetCenter() - A.GetCenter();
        T = Vector3(T.Dot(Ax), T.Dot(Ay), T.Dot(Az));

        float RA[3] = { A.GetHalfSize().x, A.GetHalfSize().y, A.GetHalfSize().z };
        float RB[3] = { B.GetHalfSize().x, B.GetHalfSize().y, B.GetHalfSize().z };

        float RMat[3][3];
        float AbsR[3][3];

        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                RMat[i][j] = Aaxis[i].Dot(Baxis[j]);
                AbsR[i][j] = fabs(RMat[i][j]) + 1e-6f;
            }
        }

        // Test A axes
        float tA[3] = { T.x, T.y, T.z };

        for (int i = 0; i < 3; i++)
        {
            float ra = RA[i];
            float rb = RB[0] * AbsR[i][0] + RB[1] * AbsR[i][1] + RB[2] * AbsR[i][2];
            if (fabs(tA[i]) > ra + rb) return false;
        }

        // Test B axes
        for (int j = 0; j < 3; j++)
        {
            float ra =
                RA[0] * AbsR[0][j] +
                RA[1] * AbsR[1][j] +
                RA[2] * AbsR[2][j];

            float rb = RB[j];

            float t =
                fabs(T.x * RMat[0][j] +
                    T.y * RMat[1][j] +
                    T.z * RMat[2][j]);

            if (t > ra + rb) return false;
        }

        return true;
    }


    //====================================
    // Ray vs OBB
    //====================================
    bool IsHit(const Ray& ray, const OBBCollider& obb, float* outDistance)
    {
        Matrix invRot = obb.GetOrientation().Transpose();

        Vector3 pLocal = ray.origin - obb.GetCenter();

        Vector3 p(
            pLocal.Dot(invRot.Right()),
            pLocal.Dot(invRot.Up()),
            pLocal.Dot(invRot.Forward())
        );

        Vector3 d(
            ray.direction.Dot(invRot.Right()),
            ray.direction.Dot(invRot.Up()),
            ray.direction.Dot(invRot.Forward())
        );

        float tMin = 0.0f;
        float tMax = FLT_MAX;

        float origin[3] = { p.x, p.y, p.z };
        float dir[3] = { d.x, d.y, d.z };
        float h[3] = { obb.GetHalfSize().x, obb.GetHalfSize().y, obb.GetHalfSize().z };

        for (int i = 0; i < 3; i++)
        {
            float bMin = -h[i];
            float bMax = h[i];

            if (fabs(dir[i]) < 1e-6f)
            {
                if (origin[i] < bMin || origin[i] > bMax)
                    return false;
            }
            else
            {
                float inv = 1.0f / dir[i];
                float t0 = (bMin - origin[i]) * inv;
                float t1 = (bMax - origin[i]) * inv;

                if (inv < 0.0f) std::swap(t0, t1);

                tMin = std::max(tMin, t0);
                tMax = std::min(tMax, t1);

                if (tMax < tMin)
                    return false;
            }
        }

        float hitT = (tMin >= 0.0f) ? tMin : tMax;
        if (hitT < 0.0f)
            return false;

        if (outDistance)
            *outDistance = hitT;

        return true;
    }
}