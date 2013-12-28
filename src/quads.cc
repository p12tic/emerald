/*  This file is part of emerald

    Copyright (C) 2006  Novell Inc.
    Copyright (C) 2013  Povilas Kanapickas <povilas@radix.lt>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "quads.h"

void update_window_extents(window_settings* ws)
{
    //where 4 is v_corn_rad (8 is 2*4), 6 is...?
    // 0,       0,          L_EXT+4,    TT_H+4,     0,0,0,0
    // L_EXT+4  0,          -8,         T_EXT+2,    0,0,1,0
    // L_EXT-4, 0,          R_EXT+4,    TT_H+4,     1,0,0,0
    // 0,       T_EXT+6,    L_EXT,      TT_H-6,     0,0,0,1
    // L_EXT,   T_EXT+2,    0,          TT_H-2,     0,0,1,0
    // L_EXT,   T_EXT+6,    R_EXT,      TT_H-6,     1,0,0,1
    // 0,       TT_H,       L_EXT+4,    B_EXT+4,    0,1,0,0
    // L_EXT+4, TT_H+4,     -8,         B_EXT,      0,1,1,0
    // L_EXT-4, TT_H,       R_EXT+4,    B_EXT+4,    1,1,0,0
    int l_ext = ws->win_extents.left;
    int r_ext = ws->win_extents.right;
    int t_ext = ws->win_extents.top;
    int b_ext = ws->win_extents.bottom;
    int tt_h = ws->titlebar_height;

    /*pos_t newpos[3][3] = {
      {
      {  0,  0, 10, 21,   0, 0, 0, 0 },
      { 10,  0, -8,  6,   0, 0, 1, 0 },
      {  2,  0, 10, 21,   1, 0, 0, 0 }
      }, {
      {  0, 10,  6, 11,   0, 0, 0, 1 },
      {  6,  6,  0, 15,   0, 0, 1, 0 },
      {  6, 10,  6, 11,   1, 0, 0, 1 }
      }, {
      {  0, 17, 10, 10,   0, 1, 0, 0 },
      { 10, 21, -8,  6,   0, 1, 1, 0 },
      {  2, 17, 10, 10,   1, 1, 0, 0 }
      }
      }; */
    pos_t newpos[3][3] = { {
            {0, 0, l_ext + 4, tt_h + 4, 0, 0, 0, 0},
            {l_ext + 4, 0, -8, t_ext + 2, 0, 0, 1, 0},
            {l_ext - 4, 0, r_ext + 4, tt_h + 4, 1, 0, 0, 0}
        }, {
            {0, t_ext + 6, l_ext, tt_h - 6, 0, 0, 0, 1},
            {l_ext, t_ext + 2, 0, tt_h - 2, 0, 0, 1, 0},
            {l_ext, t_ext + 6, r_ext, tt_h - 6, 1, 0, 0, 1}
        }, {
            {
                0, tt_h, l_ext + 4, b_ext + 4, 0, 1, 0,
                0
            },
            {
                l_ext + 4, tt_h + 4, -8, b_ext, 0, 1, 1,
                0
            },
            {
                l_ext - 4, tt_h, r_ext + 4, b_ext + 4, 1,
                1, 0, 0
            }
        }
    };
    memcpy(ws->pos, newpos, sizeof(pos_t) * 9);
}

int my_add_quad_row(decor_quad_t* q,
                    int width, int left,
                    int right, int ypush, int vgrav, int x0, int y0)
{
    int p1y = (vgrav == GRAVITY_NORTH) ? -ypush : 0;
    int p2y = (vgrav == GRAVITY_NORTH) ? 0 : ypush - 1;

    int fwidth = width - (left + right);

    q->p1.x = -left;
    q->p1.y = p1y;                                // opt: never changes
    q->p1.gravity = vgrav | GRAVITY_WEST;
    q->p2.x = 0;
    q->p2.y = p2y;
    q->p2.gravity = vgrav | GRAVITY_WEST;
    q->align = 0;                                // opt: never changes
    q->clamp = 0;
    q->stretch = 0;
    q->max_width = left;
    q->max_height = ypush;                // opt: never changes
    q->m.x0 = x0;
    q->m.y0 = y0;                                // opt: never changes
    q->m.xx = 1;                                // opt: never changes
    q->m.xy = 0;
    q->m.yy = 1;
    q->m.yx = 0;                                // opt: never changes

    q++;

    q->p1.x = 0;
    q->p1.y = p1y;
    q->p1.gravity = vgrav | GRAVITY_WEST;
    q->p2.x = 0;
    q->p2.y = p2y;
    q->p2.gravity = vgrav | GRAVITY_EAST;
    q->align = ALIGN_LEFT | ALIGN_TOP;
    q->clamp = 0;
    q->stretch = STRETCH_X;
    q->max_width = fwidth;
    q->max_height = ypush;
    q->m.x0 = x0 + left;
    q->m.y0 = y0;
    q->m.xx = 1;
    q->m.xy = 0;
    q->m.yy = 1;
    q->m.yx = 0;

    q++;

    q->p1.x = 0;
    q->p1.y = p1y;
    q->p1.gravity = vgrav | GRAVITY_EAST;
    q->p2.x = right;
    q->p2.y = p2y;
    q->p2.gravity = vgrav | GRAVITY_EAST;
    q->max_width = right;
    q->max_height = ypush;
    q->align = 0;
    q->clamp = 0;
    q->stretch = 0;
    q->m.x0 = x0 + left + fwidth;
    q->m.y0 = y0;
    q->m.xx = 1;
    q->m.yy = 1;
    q->m.xy = 0;
    q->m.yx = 0;

    return 3;
}

int my_add_quad_col(decor_quad_t* q,
                    int height, int xpush, int hgrav, int x0, int y0)
{
    int p1x = (hgrav == GRAVITY_WEST) ? -xpush : 0;
    int p2x = (hgrav == GRAVITY_WEST) ? 0 : xpush;

    q->p1.x = p1x;
    q->p1.y = 0;
    q->p1.gravity = GRAVITY_NORTH | hgrav;
    q->p2.x = p2x;
    q->p2.y = 0;
    q->p2.gravity = GRAVITY_SOUTH | hgrav;
    q->max_width = xpush;
    q->max_height = height;
    q->align = 0;
    q->clamp = CLAMP_VERT;
    q->stretch = STRETCH_Y;
    q->m.x0 = x0;
    q->m.y0 = y0;
    q->m.xx = 1;
    q->m.xy = 0;
    q->m.yy = 1;
    q->m.yx = 0;

    return 1;
}

int my_set_window_quads(decor_quad_t* q, int width, int height,
                        window_settings* ws, bool max_horz, bool max_vert)
{
    int nq;
    int mnq = 0;

    if (!max_vert || !max_horz || !ws->use_decoration_cropping) {
        //TOP QUAD
        nq = my_add_quad_row(q, width, ws->left_space, ws->right_space,
                             ws->titlebar_height + ws->top_space,
                             GRAVITY_NORTH, 0, 0);
        q += nq;
        mnq += nq;


        //BOTTOM QUAD
        nq = my_add_quad_row(q, width, ws->left_space, ws->right_space,
                             ws->bottom_space, GRAVITY_SOUTH, 0,
                             height - ws->bottom_space);
        q += nq;
        mnq += nq;

        nq = my_add_quad_col(q, height -
                             (ws->titlebar_height + ws->top_space +
                              ws->bottom_space), ws->left_space, GRAVITY_WEST,
                             0, ws->top_space + ws->titlebar_height);
        q += nq;
        mnq += nq;

        nq = my_add_quad_col(q, height -
                             (ws->titlebar_height + ws->top_space +
                              ws->bottom_space), ws->right_space,
                             GRAVITY_EAST, width - ws->right_space,
                             ws->top_space + ws->titlebar_height);
        q += nq;
        mnq += nq;
    } else {
        q->p1.x = 0;
        q->p1.y = -(ws->titlebar_height + ws->win_extents.top);
        q->p1.gravity = GRAVITY_NORTH | GRAVITY_WEST;
        q->p2.x = 0;
        q->p2.y = 0;
        q->p2.gravity = GRAVITY_NORTH | GRAVITY_EAST;
        q->max_width = width - (ws->left_space + ws->right_space);
        q->max_height = ws->titlebar_height + ws->win_extents.top;
        q->align = ALIGN_LEFT | ALIGN_TOP;
        q->clamp = 0;
        q->stretch = STRETCH_X;
        q->m.x0 = ws->left_space;
        q->m.y0 = ws->top_space - ws->win_extents.top;
        q->m.xx = 1;
        q->m.xy = 0;
        q->m.yy = 1;
        q->m.yx = 0;
        q++;
        mnq++;
    }

    return mnq;
}

int set_switcher_quads(decor_quad_t* q, int width, int height, window_settings* ws)
{
    int n, nQuad = 0;

    /* 1 top quads */
    q->p1.x = -ws->left_space;
    q->p1.y = -ws->top_space - SWITCHER_TOP_EXTRA;
    q->p1.gravity = GRAVITY_NORTH | GRAVITY_WEST;
    q->p2.x = ws->right_space;
    q->p2.y = 0;
    q->p2.gravity = GRAVITY_NORTH | GRAVITY_EAST;
    q->max_width = SHRT_MAX;
    q->max_height = SHRT_MAX;
    q->align = 0;
    q->clamp = 0;
    q->stretch = 0;
    q->m.xx = 1;
    q->m.xy = 0;
    q->m.yx = 0;
    q->m.yy = 1;
    q->m.x0 = 0;
    q->m.y0 = 0;

    q++;
    nQuad++;

    /* left quads */
    n = decor_set_vert_quad_row(q,
                                0, ws->switcher_top_corner_space, 0, ws->bottom_corner_space,
                                -ws->left_space, 0, GRAVITY_WEST,
                                height - ws->top_space - ws->titlebar_height - ws->bottom_space,
                                (ws->switcher_top_corner_space - ws->switcher_bottom_corner_space) >> 1,
                                0, 0.0, ws->top_space + SWITCHER_TOP_EXTRA, false);

    q += n;
    nQuad += n;

    /* right quads */
    n = decor_set_vert_quad_row(q,
                                0, ws->switcher_top_corner_space, 0, ws->switcher_bottom_corner_space,
                                0, ws->right_space, GRAVITY_EAST,
                                height - ws->top_space - ws->titlebar_height - ws->bottom_space,
                                (ws->switcher_top_corner_space - ws->switcher_bottom_corner_space) >> 1,
                                0, width - ws->right_space, ws->top_space + SWITCHER_TOP_EXTRA, false);

    q += n;
    nQuad += n;

    /* 1 bottom quad */
    q->p1.x = -ws->left_space;
    q->p1.y = 0;
    q->p1.gravity = GRAVITY_SOUTH | GRAVITY_WEST;
    q->p2.x = ws->right_space;
    q->p2.y = ws->bottom_space + SWITCHER_SPACE;
    q->p2.gravity = GRAVITY_SOUTH | GRAVITY_EAST;
    q->max_width = SHRT_MAX;
    q->max_height = SHRT_MAX;
    q->align = 0;
    q->clamp = 0;
    q->stretch = 0;
    q->m.xx = 1;
    q->m.xy = 0;
    q->m.yx = 0;
    q->m.yy = 1;
    q->m.x0 = 0;
    q->m.y0 = height - ws->bottom_space - SWITCHER_SPACE;

    nQuad++;

    return nQuad;
}

int set_shadow_quads(decor_quad_t* q, int width, int height, window_settings* ws)
{
    int n, nQuad = 0;

    /* top quads */
    n = decor_set_horz_quad_line(q,
                                 ws->shadow_left_space, ws->shadow_left_corner_space,
                                 ws->shadow_right_space, ws->shadow_right_corner_space,
                                 -ws->shadow_top_space, 0, GRAVITY_NORTH, width,
                                 (ws->shadow_left_corner_space - ws->shadow_right_corner_space) >> 1,
                                 0, 0.0, 0.0);

    q += n;
    nQuad += n;

    /* left quads */
    n = decor_set_vert_quad_row(q,
                                0, ws->shadow_top_corner_space, 0, ws->shadow_bottom_corner_space,
                                -ws->shadow_left_space, 0, GRAVITY_WEST,
                                height - ws->shadow_top_space - ws->shadow_bottom_space,
                                (ws->shadow_top_corner_space - ws->shadow_bottom_corner_space) >> 1,
                                0, 0.0, ws->shadow_top_space, false);

    q += n;
    nQuad += n;

    /* right quads */
    n = decor_set_vert_quad_row(q,
                                0, ws->shadow_top_corner_space, 0, ws->shadow_bottom_corner_space, 0,
                                ws->shadow_right_space, GRAVITY_EAST,
                                height - ws->shadow_top_space - ws->shadow_bottom_space,
                                (ws->shadow_top_corner_space - ws->shadow_bottom_corner_space) >> 1,
                                0, width - ws->shadow_right_space, ws->shadow_top_space, false);

    q += n;
    nQuad += n;

    /* bottom quads */
    n = decor_set_horz_quad_line(q,
                                 ws->shadow_left_space, ws->shadow_left_corner_space,
                                 ws->shadow_right_space, ws->shadow_right_corner_space, 0,
                                 ws->shadow_bottom_space, GRAVITY_SOUTH, width,
                                 (ws->shadow_left_corner_space - ws->shadow_right_corner_space) >> 1,
                                 0, 0.0, ws->shadow_top_space + ws->shadow_top_corner_space +
                                 ws->shadow_bottom_corner_space + 1.0);

    nQuad += n;

    return nQuad;
}

