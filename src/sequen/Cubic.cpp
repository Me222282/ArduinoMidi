#include <Arduino.h>
#include "Cubic.h"

CubicInput getInput(float x)
{
    float x2 = x * x;
    return { x2 * x, x2, x };
}

float getCubicValue(Cubic c, CubicInput ci)
{
    return (c.a * ci.x3) + (c.b * ci.x2) + (c.c * ci.x) + c.d;
}

Cubic generateCubic(uint16_t p1, uint16_t p2, uint16_t p3, uint16_t p4)
{
    int32_t m = 1, n = 1, o = 1, p = 1;
    int32_t i = p1, j = p2, k = p3, l = p4;
    int32_t e = p1 * p1, f = p2 * p2, g = p3 * p3, h = p4 * p4;
    int32_t a = e * p1, b = f * p2, c = g * p3, d = h * p4;

    int32_t kp_lo = k * p - l * o;
    int32_t jp_ln = j * p - l * n;
    int32_t jo_kn = j * o - k * n;
    int32_t ip_lm = i * p - l * m;
    int32_t io_km = i * o - k * m;
    int32_t in_jm = i * n - j * m;

    int32_t a11 = f * kp_lo - g * jp_ln + h * jo_kn;
    int32_t a12 = e * kp_lo - g * ip_lm + h * io_km;
    int32_t a13 = e * jp_ln - f * ip_lm + h * in_jm;
    int32_t a14 = e * jo_kn - f * io_km + g * in_jm;

    int32_t det = (a * a11) - (b * a12) + (c * a13) - (d * a14);

    float invDet = 1.0f / det;

    int32_t gp_ho = g * p - h * o;
    int32_t fp_hn = f * p - h * n;
    int32_t fo_gn = f * o - g * n;
    int32_t ep_hm = e * p - h * m;
    int32_t eo_gm = e * o - g * m;
    int32_t en_fm = e * n - f * m;

    int32_t gl_hk = g * l - h * k;
    int32_t fl_hj = f * l - h * j;
    int32_t fk_gj = f * k - g * j;
    int32_t el_hi = e * l - h * i;
    int32_t ek_gi = e * k - g * i;
    int32_t ej_fi = e * j - f * i;
    
    float a_ = -a11 + a13 - /*2*/a14 - a14;
    
    float b_b = (a * jo_kn - b * io_km + c * in_jm);
    float b_ = (b * kp_lo - c * jp_ln + d * jo_kn) -
        (a * jp_ln - b * ip_lm + d * in_jm) +
        b_b + b_b;
    
    float c_c = (a * fo_gn - b * eo_gm + c * en_fm);
    float c_ = -(b * gp_ho - c * fp_hn + d * fo_gn) +
        (a * fp_hn - b * ep_hm + d * en_fm) -
        c_c - c_c;
    
    float d_d = (a * fk_gj - b * ek_gi + c * ej_fi);
    float d_ = (b * gl_hk - c * fl_hj + d * fk_gj) -
        (a * fl_hj - b * el_hi + d * ej_fi) +
        d_d + d_d;
    
    return { a_ * invDet, b_ * invDet, c_ * invDet, d_ * invDet };
}