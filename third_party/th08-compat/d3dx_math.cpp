// Math adapter imported from th08-solver 1c68dc9 (MIT).
#include <d3dx8.h>
#include <cstring>
#include <xmmintrin.h>
D3DXMATRIX *D3DXMatrixIdentity(D3DXMATRIX *out)
{
    if (out == NULL) return NULL;
    memset(out, 0, sizeof(*out)); out->_11 = out->_22 = out->_33 = out->_44 = 1.0f; return out;
}

D3DXMATRIX *D3DXMatrixMultiply(D3DXMATRIX *out, const D3DXMATRIX *left, const D3DXMATRIX *right)
{
    if (out == NULL || left == NULL || right == NULL) return NULL;
    D3DXMATRIX result;
    const FLOAT *a = *left, *b = *right;
    FLOAT *r = result;
    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 4; ++column)
            r[row * 4 + column] = a[row * 4] * b[column] + a[row * 4 + 1] * b[4 + column] +
                                  a[row * 4 + 2] * b[8 + column] + a[row * 4 + 3] * b[12 + column];
    *out = result; return out;
}

D3DXMATRIX *D3DXMatrixRotationX(D3DXMATRIX *out, FLOAT angle)
{
    D3DXMatrixIdentity(out); if (out == NULL) return NULL;
    FLOAT cosine = cosf(angle), sine = sinf(angle);
    out->_22 = cosine; out->_23 = sine; out->_32 = -sine; out->_33 = cosine; return out;
}

D3DXMATRIX *D3DXMatrixRotationY(D3DXMATRIX *out, FLOAT angle)
{
    D3DXMatrixIdentity(out); if (out == NULL) return NULL;
    FLOAT cosine = cosf(angle), sine = sinf(angle);
    out->_11 = cosine; out->_13 = -sine; out->_31 = sine; out->_33 = cosine; return out;
}

D3DXMATRIX *D3DXMatrixRotationZ(D3DXMATRIX *out, FLOAT angle)
{
    D3DXMatrixIdentity(out); if (out == NULL) return NULL;
    FLOAT cosine = cosf(angle), sine = sinf(angle);
    out->_11 = cosine; out->_12 = sine; out->_21 = -sine; out->_22 = cosine; return out;
}

D3DXMATRIX *D3DXMatrixRotationQuaternion(D3DXMATRIX *out, const D3DXQUATERNION *q)
{
    if (out == NULL || q == NULL) return NULL;
    out->_11 = 1 - 2 * (q->y * q->y + q->z * q->z);
    out->_12 = 2 * (q->x * q->y + q->z * q->w); out->_13 = 2 * (q->x * q->z - q->y * q->w); out->_14 = 0;
    out->_21 = 2 * (q->x * q->y - q->z * q->w);
    out->_22 = 1 - 2 * (q->x * q->x + q->z * q->z); out->_23 = 2 * (q->y * q->z + q->x * q->w); out->_24 = 0;
    out->_31 = 2 * (q->x * q->z + q->y * q->w); out->_32 = 2 * (q->y * q->z - q->x * q->w);
    out->_33 = 1 - 2 * (q->x * q->x + q->y * q->y); out->_34 = 0;
    out->_41 = out->_42 = out->_43 = 0; out->_44 = 1; return out;
}

D3DXVECTOR3 *D3DXVec3Cross(D3DXVECTOR3 *out, const D3DXVECTOR3 *left, const D3DXVECTOR3 *right)
{
    if (out == NULL || left == NULL || right == NULL) return NULL;
    D3DXVECTOR3 result(left->y * right->z - left->z * right->y,
                       left->z * right->x - left->x * right->z,
                       left->x * right->y - left->y * right->x);
    *out = result; return out;
}

FLOAT D3DXVec3Dot(const D3DXVECTOR3 *left, const D3DXVECTOR3 *right)
{ return left != NULL && right != NULL ? left->x * right->x + left->y * right->y + left->z * right->z : 0.0f; }

FLOAT D3DXVec3LengthSq(const D3DXVECTOR3 *value) { return D3DXVec3Dot(value, value); }
FLOAT D3DXVec3Length(const D3DXVECTOR3 *value) { return sqrtf(D3DXVec3LengthSq(value)); }

#if defined(__i386__) || defined(__x86_64__)
__attribute__((target("sse")))
#endif
D3DXVECTOR3 *D3DXVec3Normalize(D3DXVECTOR3 *out, const D3DXVECTOR3 *value)
{
    if (out == NULL || value == NULL) return NULL;
    const D3DXVECTOR3 copy = *value;
#if defined(__i386__) || defined(__x86_64__)
    const __m128 x = _mm_set_ss(copy.x), y = _mm_set_ss(copy.y), z = _mm_set_ss(copy.z);
    const __m128 square = _mm_add_ss(_mm_add_ss(_mm_mul_ss(x, x), _mm_mul_ss(y, y)), _mm_mul_ss(z, z));
    if (_mm_cvtss_f32(square) < 1.4210854715202004e-14f)
        out->x = out->y = out->z = 0.0f;
    else
    {
        // Retail D3DX refines RSQRT once, rounding each SSE operation to float.
        const __m128 estimate = _mm_rsqrt_ss(square);
        const __m128 correction = _mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(square, estimate), estimate));
        const __m128 scale = _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), estimate), correction);
        out->x = _mm_cvtss_f32(_mm_mul_ss(scale, x));
        out->y = _mm_cvtss_f32(_mm_mul_ss(scale, y));
        out->z = _mm_cvtss_f32(_mm_mul_ss(scale, z));
    }
#else
    const FLOAT square = D3DXVec3LengthSq(&copy);
    if (square < 1.4210854715202004e-14f)
        out->x = out->y = out->z = 0.0f;
    else
        *out = copy / sqrtf(square);
#endif
    return out;
}

D3DXMATRIX *D3DXMatrixLookAtLH(D3DXMATRIX *out, const D3DXVECTOR3 *eye,
                               const D3DXVECTOR3 *at, const D3DXVECTOR3 *up)
{
    if (out == NULL || eye == NULL || at == NULL || up == NULL) return NULL;
    D3DXVECTOR3 zaxis, xaxis, yaxis, direction = *at - *eye;
    D3DXVec3Normalize(&zaxis, &direction); D3DXVec3Cross(&xaxis, up, &zaxis);
    D3DXVec3Normalize(&xaxis, &xaxis); D3DXVec3Cross(&yaxis, &zaxis, &xaxis);
    out->_11 = xaxis.x; out->_12 = yaxis.x; out->_13 = zaxis.x; out->_14 = 0;
    out->_21 = xaxis.y; out->_22 = yaxis.y; out->_23 = zaxis.y; out->_24 = 0;
    out->_31 = xaxis.z; out->_32 = yaxis.z; out->_33 = zaxis.z; out->_34 = 0;
    out->_41 = -D3DXVec3Dot(&xaxis, eye); out->_42 = -D3DXVec3Dot(&yaxis, eye);
    out->_43 = -D3DXVec3Dot(&zaxis, eye); out->_44 = 1; return out;
}

D3DXMATRIX *D3DXMatrixPerspectiveFovLH(D3DXMATRIX *out, FLOAT fov, FLOAT aspect, FLOAT nearZ, FLOAT farZ)
{
    if (out == NULL || aspect == 0 || farZ == nearZ) return NULL;
    memset(out, 0, sizeof(*out)); FLOAT yScale = 1.0f / tanf(fov * 0.5f);
    out->_11 = yScale / aspect; out->_22 = yScale; out->_33 = farZ / (farZ - nearZ);
    out->_34 = 1.0f; out->_43 = -nearZ * farZ / (farZ - nearZ); return out;
}

D3DXVECTOR3 *D3DXVec3TransformCoord(D3DXVECTOR3 *out, const D3DXVECTOR3 *value, const D3DXMATRIX *matrix)
{
    if (out == NULL || value == NULL || matrix == NULL) return NULL;
    FLOAT x = value->x * matrix->_11 + value->y * matrix->_21 + value->z * matrix->_31 + matrix->_41;
    FLOAT y = value->x * matrix->_12 + value->y * matrix->_22 + value->z * matrix->_32 + matrix->_42;
    FLOAT z = value->x * matrix->_13 + value->y * matrix->_23 + value->z * matrix->_33 + matrix->_43;
    FLOAT w = value->x * matrix->_14 + value->y * matrix->_24 + value->z * matrix->_34 + matrix->_44;
    if (fabsf(w) > 1.0e-8f) { x /= w; y /= w; z /= w; }
    out->x = x; out->y = y; out->z = z; return out;
}

D3DXVECTOR3 *D3DXVec3Project(D3DXVECTOR3 *out, const D3DXVECTOR3 *value, const D3DVIEWPORT8 *viewport,
                             const D3DXMATRIX *projection, const D3DXMATRIX *view, const D3DXMATRIX *world)
{
    if (out == NULL || value == NULL || viewport == NULL || projection == NULL || view == NULL || world == NULL) return NULL;
    D3DXMATRIX worldView, transform; D3DXVECTOR3 projected;
    D3DXMatrixMultiply(&worldView, world, view); D3DXMatrixMultiply(&transform, &worldView, projection);
    D3DXVec3TransformCoord(&projected, value, &transform);
    out->x = viewport->X + (projected.x + 1.0f) * viewport->Width * 0.5f;
    out->y = viewport->Y + (1.0f - projected.y) * viewport->Height * 0.5f;
    out->z = viewport->MinZ + projected.z * (viewport->MaxZ - viewport->MinZ); return out;
}
