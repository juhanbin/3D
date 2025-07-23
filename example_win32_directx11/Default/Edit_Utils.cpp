#include "Edit_Defines.h"
//#include <DirectXCollision.h>
//using namespace DirectX;
//
//Ray CreatePickingRay(int mx, int my, int w, int h, const XMMATRIX& view, const XMMATRIX& proj)
//{
//    float px = (2.0f * mx / w - 1.0f);
//    float py = (1.0f - 2.0f * my / h);
//    XMVECTOR rayClip = XMVectorSet(px, py, 1.0f, 1.0f);
//
//    XMMATRIX invProj = XMMatrixInverse(nullptr, proj);
//    XMVECTOR rayEye = XMVector3TransformCoord(rayClip, invProj);
//    rayEye = XMVectorSetW(rayEye, 0.0f);
//
//    XMMATRIX invView = XMMatrixInverse(nullptr, view);
//    XMVECTOR rayDir = XMVector3TransformNormal(rayEye, invView);
//    rayDir = XMVector3Normalize(rayDir);
//    XMVECTOR rayOrigin = XMVector3TransformCoord(XMVectorZero(), invView);
//
//    Ray ray;
//    XMStoreFloat3(&ray.origin, rayOrigin);
//    XMStoreFloat3(&ray.dir, rayDir);
//    return ray;
//}
//
//bool RayIntersectsAABB(const Ray& ray, const BoundingBox& box, float* outDist)
//{
//    float dist = 0.0f;
//    bool hit = box.Intersects(XMLoadFloat3(&ray.origin), XMLoadFloat3(&ray.dir), dist);
//    if (hit && outDist) *outDist = dist;
//    return hit;
//}
