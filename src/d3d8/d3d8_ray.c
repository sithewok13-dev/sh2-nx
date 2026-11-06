/* The free camera's wall test (game/ee.c): does a line from James to where the camera wants to be pass through
 * the world? Silent Hill 2's own collision isn't known, so this asks what the game drew instead. Every frame the
 * solid world's draws are noted as they are made: fixed-function, untransformed and unblended vertices (XYZ, so no
 * skinned characters, no screen-space 2D), writing depth, not alpha blended (no fog sheets, particles or glass),
 * from static buffers (a dynamic one may be rewritten before the frame ends). At Present that list becomes the
 * last frame's world; a ray then goes through it in world space (each draw's WORLD transform; its vertices read
 * from the game's own copy of the buffer), first against each draw's bounding box, then its triangles.
 * The game culls what its camera can't see, but the camera is the free camera there, and the walls a camera backs
 * through are the ones between it and James: in view, so drawn. */
#include "d3d8.h"
#include <math.h>

typedef struct {
    Res *vb, *ib;
    uint32_t vb_ver, prim, start, count, base, stride, idx32;
    float world[16];
    float lo[3], hi[3];  /* world bounds */
} Draw;

static Draw *cur, *last;
static int ncur, nlast, capcur, caplast;
static int frames_seen;
static uint32_t tris_cur, tris_last;
/* Below this many solid triangles a frame isn't the world (a menu, a loading screen, or a world drawn some way this
 * doesn't see): the test then answers -1, unknown, rather than clear. */
uint32_t ray_min_triangles = 1500;

/* A buffer's bounds, in its own space, cached until it is next unlocked. */
typedef struct { Res *b; uint32_t ver, stride; float lo[3], hi[3]; } Bounds;
static Bounds cache[1024];

static int bounds_of(Res *vb, uint32_t stride, float *lo, float *hi) {
    unsigned h = ((uintptr_t)vb >> 4) * 2654435761u % 1024;
    Bounds *c = &cache[h];
    if (c->b != vb || c->ver != vb->ver || c->stride != stride) {
        uint32_t n = stride ? vb->size / stride : 0;
        if (!n || !vb->mem) return 0;
        float l[3] = {1e30f, 1e30f, 1e30f}, u[3] = {-1e30f, -1e30f, -1e30f};
        for (uint32_t i = 0; i < n; i++) {
            const float *p = GPTR(vb->mem + i * stride);
            for (int k = 0; k < 3; k++) {
                if (!(p[k] > -1e7f && p[k] < 1e7f)) continue;
                if (p[k] < l[k]) l[k] = p[k];
                if (p[k] > u[k]) u[k] = p[k];
            }
        }
        *c = (Bounds){vb, vb->ver, stride, {l[0], l[1], l[2]}, {u[0], u[1], u[2]}};
    }
    memcpy(lo, c->lo, 12), memcpy(hi, c->hi, 12);
    return lo[0] <= hi[0];
}

static void xform(const float *m, const float *v, float *o) {  /* D3D row vector times row-major matrix */
    for (int k = 0; k < 3; k++) o[k] = v[0] * m[k] + v[1] * m[4 + k] + v[2] * m[8 + k] + m[12 + k];
}

#define DYNAMIC 0x200  /* D3DUSAGE_DYNAMIC */
void ray_collect(uint32_t prim, uint32_t start, uint32_t count, int indexed) {
    if (prim < 4 || prim > 6 || !count) return;               /* triangles only */
    if (dev->vs >= 0xF0000000u || (dev->vs & 0xE) != 2) return;  /* fixed-function XYZ */
    if (dev->rs[27] || !dev->rs[7] || !dev->rs[14]) return;     /* blended, or no depth test / write */
    Res *vb = res_of(dev->stream[0]), *ib = indexed ? res_of(dev->ib) : NULL;
    if (!vb || vb->kind != RES_VB || (vb->usage & DYNAMIC) || (indexed && (!ib || (ib->usage & DYNAMIC)))) return;
    uint32_t stride = dev->stride[0];
    if (stride < 12) return;
    if (ncur == capcur) {
        capcur = capcur ? capcur * 2 : 512;
        if (capcur > 16384) { capcur = 16384; return; }
        cur = realloc(cur, capcur * sizeof *cur);
    }
    Draw *d = &cur[ncur];
    float lo[3], hi[3];
    if (!bounds_of(vb, stride, lo, hi)) return;
    *d = (Draw){vb, ib, vb->ver, prim, start, count, indexed ? dev->base_vertex : 0, stride,
                ib && ib->fmt == FMT_INDEX32};
    memcpy(d->world, dev->world[0], 64);
    /* the box's eight corners into the world */
    for (int k = 0; k < 3; k++) d->lo[k] = 1e30f, d->hi[k] = -1e30f;
    for (int c = 0; c < 8; c++) {
        float v[3] = {c & 1 ? hi[0] : lo[0], c & 2 ? hi[1] : lo[1], c & 4 ? hi[2] : lo[2]}, w[3];
        xform(d->world, v, w);
        for (int k = 0; k < 3; k++) { if (w[k] < d->lo[k]) d->lo[k] = w[k]; if (w[k] > d->hi[k]) d->hi[k] = w[k]; }
    }
    tris_cur += prim == 4 ? count : count;
    ncur++;
}

void ray_frame_end(void) {
    Draw *t = last; int tc = caplast;
    last = cur, nlast = ncur, caplast = capcur;
    cur = t, capcur = tc, ncur = 0;
    tris_last = tris_cur, tris_cur = 0;
    if (frames_seen < 1000) frames_seen++;
}

/* The share of the way from a to b before the world (1: clear). */
static int seg_box(const float *a, const float *d, const float *lo, const float *hi) {
    float t0 = 0, t1 = 1;
    for (int k = 0; k < 3; k++) {
        if (fabsf(d[k]) < 1e-9f) { if (a[k] < lo[k] || a[k] > hi[k]) return 0; continue; }
        float u = (lo[k] - a[k]) / d[k], v = (hi[k] - a[k]) / d[k];
        if (u > v) { float s = u; u = v; v = s; }
        if (u > t0) t0 = u;
        if (v < t1) t1 = v;
        if (t0 > t1) return 0;
    }
    return 1;
}
static float seg_tri(const float *a, const float *d, const float *p0, const float *p1, const float *p2) {
    float e1[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]}, e2[3] = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
    float h[3] = {d[1] * e2[2] - d[2] * e2[1], d[2] * e2[0] - d[0] * e2[2], d[0] * e2[1] - d[1] * e2[0]};
    float det = e1[0] * h[0] + e1[1] * h[1] + e1[2] * h[2];
    if (fabsf(det) < 1e-12f) return 2;
    float f = 1 / det, s[3] = {a[0] - p0[0], a[1] - p0[1], a[2] - p0[2]};
    float u = f * (s[0] * h[0] + s[1] * h[1] + s[2] * h[2]);
    if (u < 0 || u > 1) return 2;
    float q[3] = {s[1] * e1[2] - s[2] * e1[1], s[2] * e1[0] - s[0] * e1[2], s[0] * e1[1] - s[1] * e1[0]};
    float v = f * (d[0] * q[0] + d[1] * q[1] + d[2] * q[2]);
    if (v < 0 || u + v > 1) return 2;
    float t = f * (e2[0] * q[0] + e2[1] * q[1] + e2[2] * q[2]);
    return t >= 0 && t <= 1 ? t : 2;
}

static uint32_t index_of(const Draw *d, uint32_t i) {
    if (!d->ib) return d->start + i;
    uint32_t at = d->ib->mem + (d->start + i) * (d->idx32 ? 4 : 2);
    if (at + 4 > d->ib->mem + d->ib->size) return 0xFFFFFFFFu;
    return (d->idx32 ? MEM32(at) : MEM16(at)) + d->base;
}

/* game/ee.c: -1 when there is no world to test against yet (the camera then keeps to open rooms). */
float d3d_world_ray(const float *a, const float *b) {
    if (!nlast || frames_seen < 2 || tris_last < ray_min_triangles) return -1;
    float d[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]}, best = 1;
    for (int n = 0; n < nlast; n++) {
        const Draw *dr = &last[n];
        if (dr->vb->ver != dr->vb_ver || !seg_box(a, d, dr->lo, dr->hi)) continue;
        uint32_t nv = dr->vb->size / dr->stride, ntri = dr->prim == 4 ? dr->count : dr->count;
        for (uint32_t t = 0; t < ntri; t++) {
            uint32_t i0, i1, i2;
            if (dr->prim == 4) i0 = 3 * t, i1 = 3 * t + 1, i2 = 3 * t + 2;
            else if (dr->prim == 5) i0 = t, i1 = t + 1, i2 = t + 2;
            else i0 = 0, i1 = t + 1, i2 = t + 2;
            uint32_t v0 = index_of(dr, i0), v1 = index_of(dr, i1), v2 = index_of(dr, i2);
            if (v0 >= nv || v1 >= nv || v2 >= nv) continue;
            float p[3][3];
            xform(dr->world, GPTR(dr->vb->mem + v0 * dr->stride), p[0]);
            xform(dr->world, GPTR(dr->vb->mem + v1 * dr->stride), p[1]);
            xform(dr->world, GPTR(dr->vb->mem + v2 * dr->stride), p[2]);
            float h = seg_tri(a, d, p[0], p[1], p[2]);
            if (h < best) best = h;
        }
    }
    return best;
}
