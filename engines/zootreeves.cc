/*
 * zootreeves theme engine
 *
 * zootreeves.c
 *
 * Copyright (C) 2006 Quinn Storm <livinglatexkali@gmail.com> (original legacy theme engine)
 * Copyright (C) 2006 Ben Reeves <john@reeves160.freeserve.co.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

//zootreeves engine

#include <emerald.h>
#include <engine.h>
#include <zootreeves_icon.h>

#define SECT "zootreeves_settings"

#define DIP_ROUND_TRI  (1 << 4)

struct pixmaps {
    cairo_surface_t* titlebar_surface;
    cairo_surface_t* titlebar_surface_large;
    cairo_surface_t* titlebar_surface_buttons;
    bool    buttonpart_enabled;
    bool    buttonpart_repeat_enabled;
    bool    titlebarpart_repeat_enabled;
    bool    titlebarpart_enabled;
    bool    titlebar_enabled;
    bool    titlebar_repeat_enabled;
};

struct private_fs {
    alpha_color inner;
    alpha_color outer;
    alpha_color title_inner;
    alpha_color title_outer;
    alpha_color window_halo;
    alpha_color window_frame_halo;
    alpha_color window_highlight;
    bool    gradient_repeat_enabled;
    bool    gradient_repeat_direction_vertical;
    bool    gradient_repeat_direction_diagonal;
    alpha_color window_shadow;
    alpha_color separator_line;
    alpha_color contents_highlight;
    alpha_color contents_shadow;
    alpha_color contents_halo;
    double  gradient_repeat_height;
};

struct private_ws {
    bool done_indent;
    bool round_top_left;
    bool enable_maximised_colors;
    bool round_top_right;
    bool round_bottom_left;
    bool round_bottom_right;
    bool show_border_minimised;
    bool round_tri;
    bool show_border_maximised;
    bool enable_bar_dip_button_part;
    bool enable_title_bar_dip;
    bool enable_left_bar_dip;
    bool enable_left_bar_dip_lower_part;
    bool gradient_repeat_disabled_maximised;
    int     left_bar_dip_offset;
    double      outer_maximised_alpha;
    decor_color_t outer_maximised_color;
    double      inner_maximised_alpha;
    decor_color_t inner_maximised_color;
    double  titlebar_radius;
    double  frame_radius;
    double  window_gap;
    int     minimised_border;
    int     title_bar_dip_title_width;
    int     title_bar_dip_radius;
    int     title_bar_dip_button_width;
    int     left_bar_dip_radius;
    pixmaps     pixmaps;

};

extern "C"
void get_meta_info(EngineMetaInfo* emi)
{
    emi->version = g_strdup("0.1");
    emi->description = g_strdup(_("Evolved from the legacy engine"));
    emi->last_compat = g_strdup("0.0"); // old themes marked still compatible for testing-NeOS
    emi->icon = Gdk::Pixbuf::create_from_inline(sizeof(my_pixbuf), my_pixbuf, true);
}

void
rounded_rectangle_independent(cairo_t* cr,
                              double  x,
                              double  y,
                              double  w,
                              double  h,
                              double  title_bar_height,
                              int    corner,
                              window_settings* ws,
                              double  left_top_radius,
                              double  right_top_radius,
                              double  left_bottom_radius,
                              double  right_bottom_radius,
                              double  radius_tri,
                              int pane_1_width,
                              int dip_gap,
                              bool enable_dip,
                              bool enable_button_part,
                              bool left_bar_dip,
                              int bottom_border_width,
                              int left_bar_dip_radius,
                              bool enable_left_bar_dip_lower_part,
                              double  left_bar_dip_offset
                             )
{

    int left_top_radius_on = 1;
    int right_top_radius_on = 1;
    int left_bottom_radius_on = 1;
    int right_bottom_radius_on = 1;
    int radius_tri_on = 0;


    if (left_top_radius == 0) {
        left_top_radius_on = 0;
    }

    if (right_top_radius == 0) {
        right_top_radius_on = 0;
    }

    if (left_bottom_radius == 0) {
        left_bottom_radius_on = 0;
    }

    if (right_bottom_radius == 0) {
        right_bottom_radius_on = 0;
    }

    if (radius_tri != 0) {
        radius_tri_on = 1;
    }


    int curve, radius2;
    double cx, cy;

    if (left_top_radius_on == 1) {
        if ((corner & CORNER_TOPLEFT)) {
            cairo_move_to(cr, x + left_top_radius, y);
        } else {
            cairo_move_to(cr, x, y);
        }
    }






    if (radius_tri_on == 1 && (corner & DIP_ROUND_TRI)) {

        if (radius_tri > title_bar_height) {
            radius_tri = title_bar_height;
        }

        curve = 2;

        radius2 = title_bar_height - radius_tri;

        cairo_arc(cr, x + pane_1_width, y  + radius_tri, radius_tri, M_PI * 1.5, M_PI * curve);
        cairo_get_current_point(cr, &cx, &cy);
        cairo_arc_negative(cr, x + pane_1_width + (title_bar_height), cy, radius2, M_PI * (curve - 1), M_PI * 2.5);


        radius2 = title_bar_height - radius_tri;

        if (enable_button_part == true) {
            //Can use h to offset x because it is a square - would be radius - radius2
            cairo_arc_negative(cr, x + pane_1_width + dip_gap - radius_tri - radius2, y + radius_tri, radius2, M_PI * 2.5, M_PI * 2.0);
            cairo_get_current_point(cr, &cx, &cy);
            cairo_arc(cr, cx + radius_tri, cy, radius_tri, M_PI * 1.0, M_PI * 1.5);
        } else {
            cairo_get_current_point(cr, &cx, &cy);
            cairo_line_to(cr, x + w, cy);
        }

    }


    if (enable_dip == true && !(corner & DIP_ROUND_TRI)) {

        curve = 2;

        radius2 = title_bar_height - radius_tri;

        cairo_arc(cr, x + pane_1_width - radius_tri, y  + radius_tri, radius_tri, M_PI * 1.5, M_PI * curve);
        cairo_get_current_point(cr, &cx, &cy);
        cairo_line_to(cr, cx, cy + radius2);
        cairo_line_to(cr, cx + dip_gap, cy + radius2);
        //cairo_arc_negative (cr, x +pane_1_width + (title_bar_height), cy, radius2, M_PI* (curve - 1), M_PI * 2.5);


        if (enable_button_part == true) {
            cairo_get_current_point(cr, &cx, &cy);
            cairo_line_to(cr, cx, cy - radius2);
            cairo_get_current_point(cr, &cx, &cy);
            cairo_arc(cr, cx + radius_tri, cy, radius_tri, M_PI, M_PI * 1.5);
            //cairo_arc (cr, cx + radius_tri, cy, radius_tri, M_PI * 1.0, M_PI * 1.5);
        } else {
            cairo_get_current_point(cr, &cx, &cy);
            cairo_line_to(cr, x + w, cy);
        }

    }


    if (enable_button_part == true) {
        if (right_top_radius_on == 1) {
            if ((corner & CORNER_TOPRIGHT)) {
                cairo_arc(cr, x + w - right_top_radius, y + right_top_radius, right_top_radius,
                          M_PI * 1.5, M_PI * 2.0);
            } else {
                cairo_line_to(cr, x + w, y);
            }
        }
    }

    if (right_bottom_radius_on == 1) {
        if ((corner & CORNER_BOTTOMRIGHT))
            cairo_arc(cr, x + w - right_bottom_radius, y + h - right_bottom_radius, right_bottom_radius,
                      0.0, M_PI * 0.5);
        else {
            cairo_line_to(cr, x + w, y + h);
        }
    }

    if (left_bottom_radius_on == 1 && (corner & CORNER_BOTTOMLEFT))
        cairo_arc(cr, x + left_bottom_radius, y + h - left_bottom_radius, left_bottom_radius,
                  M_PI * 0.5, M_PI);
    else

        if (enable_left_bar_dip_lower_part == false && left_bar_dip == true) {
            cairo_line_to(cr, x + (2 * left_bar_dip_radius), y + h);
        } else {
            cairo_line_to(cr, x, y + h);
        }

    if (left_bar_dip == false) {
        if (left_top_radius_on == 1) {
            if ((corner & CORNER_TOPLEFT)) {
                cairo_arc(cr, x + left_top_radius, y + left_top_radius, left_top_radius, M_PI, M_PI * 1.5);
            } else {
                cairo_line_to(cr, x, y);
            }
        }
    }


    if (left_bar_dip == true) {
        left_bar_dip_offset = (((h - bottom_border_width - title_bar_height + 1)  / 100) * (100 - left_bar_dip_offset));

        cairo_get_current_point(cr, &cx, &cy);
        cairo_line_to(cr, cx, cy - bottom_border_width + 2);
        cairo_get_current_point(cr, &cx, &cy);

        if (enable_left_bar_dip_lower_part == true) {
            cairo_arc(cr, cx + left_bar_dip_radius, cy, left_bar_dip_radius, M_PI * 1.0, M_PI * 1.5);

            cairo_get_current_point(cr, &cx, &cy);
            cairo_arc_negative(cr, cx, cy - left_bar_dip_radius, left_bar_dip_radius, M_PI * 0.5, M_PI * 2.0);
        } else {
            cairo_line_to(cr, cx, cy - (2 * left_bar_dip_radius));
        }

        cairo_get_current_point(cr, &cx, &cy);
        cairo_line_to(cr, cx, cy - 1 - left_bar_dip_offset + (4 * left_bar_dip_radius));

        cairo_get_current_point(cr, &cx, &cy);
        cairo_arc_negative(cr, cx - left_bar_dip_radius, cy, left_bar_dip_radius, M_PI * 0, M_PI * 3.5);

        cairo_get_current_point(cr, &cx, &cy);
        cairo_arc(cr, cx, cy - left_bar_dip_radius, left_bar_dip_radius, M_PI * 2.5, M_PI * 3.0);


        if (left_top_radius_on == 1) {
            if ((corner & CORNER_TOPLEFT)) {
                cairo_arc(cr, x + left_top_radius, y + left_top_radius, left_top_radius, M_PI, M_PI * 1.5);
            } else {
                cairo_line_to(cr, x, y);
            }
        }


    }
}


void
rounded_square(cairo_t* cr,
               double  x,
               double  y,
               double  w,
               double  h,
               int    corner,
               window_settings* ws,
               double  radius_top_left,
               double  radius_top_right,
               double  radius_bottom_left,
               double  radius_bottom_right,
               double  radius_top_right_tri,
               double  radius_top_left_tri,
               int always_allow,
               bool left_bar_dip,
               int left_bar_dip_radius,
               bool enable_left_bar_dip_lower_part,
               double left_bar_dip_offset
              )
{

    // always_allow 1 = radius_top_left
    // always_allow 2 = radius_top_right
    // always_allow 3 = radius_bottom_left
    // always_allow 4 = radius_bottom_right

    int radius_top_left_on = 1;
    int radius_top_right_on = 1;
    int radius_bottom_left_on = 1;
    int radius_bottom_right_on = 1;
    int radius_top_right_tri_on = 1;
    int radius_top_left_tri_on = 1;


    if (radius_top_left != 0 && ((corner & CORNER_TOPLEFT) || always_allow == 1)) {
        radius_top_left_on = 0;
    }

    if (radius_top_right != 0 && ((corner & CORNER_TOPRIGHT) || always_allow == 2)) {
        radius_top_right_on = 0;
    }

    if (radius_bottom_left != 0 && ((corner & CORNER_BOTTOMLEFT) || always_allow == 3)) {
        radius_bottom_left_on = 0;
    }

    if (radius_bottom_right != 0 && ((corner & CORNER_BOTTOMRIGHT) || always_allow == 4)) {
        radius_bottom_right_on = 0;
    }

    if (radius_top_right_tri != 0 && (corner & DIP_ROUND_TRI)) {
        //Can't have both on at the same time!
        radius_top_right_tri_on = 0;
        radius_top_right_on = 1;
    }

    if (radius_top_left_tri != 0 && (corner & DIP_ROUND_TRI)) {
        //Can't have both on at the same time!
        radius_top_left_tri_on = 0;
        radius_top_left_on = 1;
    }

    double cx, cy;
    double curve, width, height;
    double radius2;

    if (left_bar_dip == true) {

        left_bar_dip_offset = (h / 100) * left_bar_dip_offset;
        //printf("Left bar heigth = %f, Dip bar offset = %f\n", h, left_bar_dip_offset);

        width = w;
        height = h;

        //if ((left_bar_dip_radius * 2) > left_bar_dip_radius) {
        //  radius_top_left = 0.5 * (radius_top_left - 1);
        //}

        //Fix this curve malarcy
        curve = 0.5;

        cairo_move_to(cr, x, y);
        //cairo_line_to (cr, x + 5, y + 5);

        cairo_line_to(cr, x, y + left_bar_dip_offset);
        cairo_get_current_point(cr, &cx, &cy);
        cairo_arc_negative(cr, cx + left_bar_dip_radius, cy, left_bar_dip_radius, M_PI * 1.0, M_PI * curve);
        cairo_get_current_point(cr, &cx, &cy);
        //x_width = radius - x_width;
        //printf("Width %f\n", a_width);
        //printf("Height %f\n", a_height);


        cairo_arc(cr, cx, cy + left_bar_dip_radius, left_bar_dip_radius, M_PI * (curve + 1), M_PI * 2.0);

        cairo_get_current_point(cr, &cx, &cy);


        //cairo_arc_negative (cr, cx, cy, radius, M_PI * (curve + 2.5), M_PI * 2.5);

        if (enable_left_bar_dip_lower_part == true) {
            cairo_line_to(cr, cx, cy + height - left_bar_dip_offset  - (4 * left_bar_dip_radius));
            cairo_get_current_point(cr, &cx, &cy);
            cairo_arc(cr, cx - left_bar_dip_radius, cy, left_bar_dip_radius, M_PI * 2.0, M_PI * (curve + 2.0));
            cairo_get_current_point(cr, &cx, &cy);
            cairo_arc_negative(cr, cx, cy + left_bar_dip_radius, left_bar_dip_radius, M_PI * 1.5, M_PI * (curve + 2.5));
            cairo_get_current_point(cr, &cx, &cy);
            cairo_line_to(cr, cx + width, cy);
            cairo_get_current_point(cr, &cx, &cy);
            cairo_line_to(cr, cx, cy - height);
            cairo_get_current_point(cr, &cx, &cy);
            cairo_line_to(cr, cx - width, cy);
        } else {
            cairo_line_to(cr, cx, cy + height - left_bar_dip_offset - (2 * left_bar_dip_radius));
            cairo_get_current_point(cr, &cx, &cy);
            cairo_line_to(cr, cx + width - (2 * left_bar_dip_radius), cy);
            cairo_get_current_point(cr, &cx, &cy);
            cairo_line_to(cr, cx, cy - height);
            cairo_get_current_point(cr, &cx, &cy);
            cairo_line_to(cr, cx - width + (2 * left_bar_dip_radius), cy);
        }






    } else {

        if (radius_top_right_tri_on == 0) {

            if (radius_top_right_tri > h) {
                radius_top_right_tri = h;
            }

            curve = 2;

            radius2 = h - radius_top_right_tri;

            cairo_arc(cr, x + w + radius_top_right_tri - radius_top_right_tri,
                      y  + radius_top_right_tri, radius_top_right_tri, M_PI * 1.5, M_PI * curve);
            cairo_get_current_point(cr, &cx, &cy);
            cairo_arc_negative(cr, x + w + radius_top_right_tri + (h - radius_top_right_tri),
                               cy, radius2, M_PI * (curve - 1), M_PI * 2.5);

            cairo_get_current_point(cr, &cx, &cy);
            cairo_line_to(cr, cx - radius2 - radius_top_right_tri, cy);
            cairo_line_to(cr, cx - radius2 - radius_top_right_tri , y);
        }

        if (radius_top_left_tri_on == 0) {

            if (radius_top_left_tri > h) {
                radius_top_left_tri = h;
            }

            curve = 2;

            radius2 = h - radius_top_left_tri;

            //Can use h to offset x because it is a square - would be radius - radius2
            cairo_arc_negative(cr, x - radius2 - radius_top_left_tri, y + radius_top_left_tri, radius2, M_PI * 2.5, M_PI * 2.0);
            cairo_get_current_point(cr, &cx, &cy);
            cairo_arc(cr, cx + radius_top_left_tri, cy, radius_top_left_tri, M_PI * 1.0, M_PI * 1.5);


            //cairo_get_current_point (cr, &cx, &cy);
            cairo_get_current_point(cr, &cx, &cy);
            cairo_line_to(cr, cx, cy + radius2 + radius_top_left_tri);
            cairo_line_to(cr, cx - radius2 - radius_top_left_tri , cy + radius2 + radius_top_left_tri);



        }


        if (radius_top_left_on == 0) {
            cairo_move_to(cr, x + radius_top_left, y);
        } else {
            cairo_move_to(cr, x, y);
        }


        if (radius_top_right_on == 0) {
            cairo_arc(cr, x + w - radius_top_right, y + radius_top_right, radius_top_right, M_PI * 1.5, M_PI * 2.0);
        } else {
            cairo_line_to(cr, x + w, y);
        }

        if (radius_bottom_right_on == 0) {
            cairo_arc(cr, x + w - radius_bottom_right, y + h - radius_bottom_right, radius_bottom_right,
                      0.0, M_PI * 0.5);
        } else {
            cairo_line_to(cr, x + w, y + h);
        }

        if (radius_bottom_left_on == 0) {
            cairo_arc(cr, x + radius_bottom_left, y + h - radius_bottom_left, radius_bottom_left,
                      M_PI * 0.5, M_PI);
        } else {
            cairo_line_to(cr, x, y + h);
        }

        if (radius_top_left_on == 0) {
            cairo_arc(cr, x + radius_top_left, y + radius_top_left, radius_top_left, M_PI, M_PI * 1.5);
        } else {
            cairo_line_to(cr, x, y);
        }

    }
}

void
fill_rounded_square(cairo_t*       cr,
                    double        x,
                    double        y,
                    double        w,
                    double        h,
                    int       corner,
                    alpha_color* c0,
                    alpha_color* c1,
                    int       gravity,
                    window_settings* ws,
                    double  radius_top_left,
                    double  radius_top_right,
                    double  radius_bottom_left,
                    double  radius_bottom_right,
                    double  radius_top_right_tri,
                    double  radius_top_left_tri,
                    int always_allow,
                    bool left_bar_dip,
                    int left_bar_dip_radius,
                    bool enable_left_bar_dip_lower_part,
                    double left_bar_dip_offset,
                    int pattern_size,
                    int gradient_repeat_direction,
                    double common_gradient_starting_point_x,
                    double common_gradient_starting_point_y,
                    cairo_surface_t* surface,
                    bool enable_pixmaps,
                    bool repeat_pixmap
                   )
{
    cairo_pattern_t* pattern;
    int gradient_offset = 0;

    if (corner & DIP_ROUND_TRI && (radius_top_left_tri > 0)) {
        gradient_offset = h;
    }


    rounded_square(cr, x, y, w, h, corner, ws, radius_top_left,
                   radius_top_right, radius_bottom_left,
                   radius_bottom_right, radius_top_right_tri,
                   radius_top_left_tri, always_allow, left_bar_dip,
                   left_bar_dip_radius, enable_left_bar_dip_lower_part, left_bar_dip_offset);

    bool pattern_vert = true;
    if (pattern_size == 0) {
        pattern_size = h;
        pattern_vert = false;
    }

    if (gravity & SHADE_RIGHT) {
        common_gradient_starting_point_x = common_gradient_starting_point_x + w;
        w = -w;
    } else if (!(gravity & SHADE_LEFT)) {
        common_gradient_starting_point_x = w = 0;
    }

    if (gravity & SHADE_BOTTOM) {
        common_gradient_starting_point_y = common_gradient_starting_point_y + h;
        h = -h;
    } else if (!(gravity & SHADE_TOP)) {
        common_gradient_starting_point_y = h = 0;
    }

    if (enable_pixmaps == false) {
        if (gradient_repeat_direction == 1 && pattern_vert == true) {
            pattern = cairo_pattern_create_linear(common_gradient_starting_point_x, common_gradient_starting_point_y, common_gradient_starting_point_x + pattern_size, common_gradient_starting_point_y);
            //pattern = cairo_pattern_create_linear (x + w, y + pattern_size, x, y);
        } else if (gradient_repeat_direction == 2 && pattern_vert == true) {
            pattern = cairo_pattern_create_linear(common_gradient_starting_point_x, common_gradient_starting_point_y, common_gradient_starting_point_x + pattern_size, common_gradient_starting_point_y + pattern_size);
        } else  {
            pattern = cairo_pattern_create_linear(common_gradient_starting_point_x, common_gradient_starting_point_y, common_gradient_starting_point_x, common_gradient_starting_point_y + pattern_size);
        }

        cairo_pattern_add_color_stop_rgba(pattern, 0.0, c0->color.r, c0->color.g,
                                          c0->color.b, c0->alpha);

        cairo_pattern_add_color_stop_rgba(pattern, 1.0, c1->color.r, c1->color.g,
                                          c1->color.b, c1->alpha);
        cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REFLECT);

    } else {


        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_surface(cr, surface, x - gradient_offset, y);
        pattern = cairo_pattern_reference(cairo_get_source(cr));
        if (repeat_pixmap == true) {
            cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
        } else {
            cairo_pattern_set_extend(pattern, CAIRO_EXTEND_NONE);
        }
    }

    cairo_set_source(cr, pattern);
    cairo_fill(cr);
    cairo_pattern_destroy(pattern);
}

extern "C"
void engine_draw_frame(decor_t* d, cairo_t* cr)
{
    double        x1, y1, x2, y2, h;
    int       top;
    alpha_color inner_color;
    alpha_color outer_color;
    alpha_color inner_title_color;
    alpha_color outer_title_color;

    frame_settings* fs = d->fs;
    private_fs* pfs = fs->engine_fs;
    window_settings* ws = fs->ws;
    private_ws* pws = ws->engine_ws;

    //printf("Higher - Red: %i, Green %i, Blue %i \n",
    //pfs->window_frame_halo.color.r, pfs->window_frame_halo.color.g,
    //pfs->window_frame_halo.color.b); The bug has already appeared here, even
    //though I havn't done anything to the color code so far

    double pleft;
    double ptop;
    double pwidth;
    double pheight;
    top = ws->win_extents.top + ws->titlebar_height;

    x1 = ws->left_space - ws->win_extents.left;
    y1 = ws->top_space - ws->win_extents.top;
    x2 = d->width - ws->right_space + ws->win_extents.right;
    y2 = d->height - ws->bottom_space + ws->win_extents.bottom;
    pleft = x1 + ws->win_extents.left - 0.5;
    ptop = y1 + top - 0.5;
    pwidth = x2 - x1 - ws->win_extents.left - ws->win_extents.right + 1;
    pheight = y2 - y1 - top - ws->win_extents.bottom + 1;

    h = d->height - ws->top_space - ws->titlebar_height - ws->bottom_space;



    int minimised_border;
    float window_gap = 0;
    //x offset due to left dip bar
    int left_bar_dip_offset = 0;
    bool maximised;
    bool enable_left_bar_dip = false;

    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

    int corners =
        ((pws->round_top_left) ? CORNER_TOPLEFT : 0) |
        ((pws->round_top_right) ? CORNER_TOPRIGHT : 0) |
        ((pws->round_tri) ? DIP_ROUND_TRI : 0) |
        ((pws->round_bottom_left) ? CORNER_BOTTOMLEFT : 0) |
        ((pws->round_bottom_right) ? CORNER_BOTTOMRIGHT : 0);


    if (d->state & (WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY | WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY)) {
        corners = 0;
        maximised = true;
        enable_left_bar_dip = false;

        if (pws->enable_maximised_colors == true) {
            inner_color.alpha = pws->inner_maximised_alpha;
            inner_color.color = pws->inner_maximised_color;
            outer_color.alpha = pws->outer_maximised_alpha;
            outer_color.color = pws->outer_maximised_color;
            inner_title_color.alpha = pws->inner_maximised_alpha;
            inner_title_color.color = pws->inner_maximised_color;
            outer_title_color.alpha = pws->outer_maximised_alpha;
            outer_title_color.color = pws->outer_maximised_color;
        } else {
            inner_color = pfs->inner;
            outer_color = pfs->outer;
            inner_title_color = pfs->title_inner;
            outer_title_color = pfs->title_outer;
        }

        minimised_border = 0;
        window_gap = 0.5;

    } else {
        maximised = false;
        minimised_border = pws->minimised_border;
        //printf("Setting large border widths");
        window_gap = pws->window_gap;
        inner_color = pfs->inner;
        outer_color = pfs->outer;
        inner_title_color = pfs->title_inner;
        outer_title_color = pfs->title_outer;

        if (pws->enable_left_bar_dip == true && !(d->state & (WNCK_WINDOW_STATE_SHADED))) {
            enable_left_bar_dip = true;
            left_bar_dip_offset = (2 * pws->left_bar_dip_radius);

            if (pws->done_indent == false && ws->tobj_layout[0] != '(') {
                char* layout;
                layout = g_strdup_printf("(-%i)%s", left_bar_dip_offset, ws->tobj_layout);
                g_free(ws->tobj_layout);
                ws->tobj_layout = layout;
                pws->done_indent = true;
            }
        }
    }

////////////DRAW FRAME///////////////////////////////////////////
    // always_allow 1 = radius_top_left
    // always_allow 2 = radius_top_right
    // always_allow 3 = radius_bottom_left
    // always_allow 4 = radius_bottom_right

    double width = x2 - x1 + (window_gap * 2);
    double x_start = x1 - window_gap;
    double x_end = x2 + window_gap;
    double pane_1_width = pws->title_bar_dip_title_width;
    double title_bar_dip_dip_width = width - (pws->title_bar_dip_title_width + pws->title_bar_dip_button_width);
    bool enable_dip;

    enable_dip = false;
    bool do_button_part = true;
    double title_bar_dip_radius = 0;
    double title_bar_dip_radius_offset = 0;

//TIDY ALL THIS UP! -lazy
//Maximised causes problems
    if (maximised == false && pws->enable_bar_dip_button_part == false) {
        do_button_part = false;
    }


    if (pws->enable_title_bar_dip == true && maximised == false
            && ((pws->title_bar_dip_title_width + pws->title_bar_dip_button_width) < width)) {
        enable_dip = true;
        title_bar_dip_radius = pws->title_bar_dip_radius;
        if (pws->round_tri == true) {
            title_bar_dip_radius_offset = top - 0.5 + minimised_border;
        }
    } else {
        do_button_part = true;
    }


////////////////////////////////////

    /* ORDER radius_top_left, radius_top_right, radius_bottom_left,
        radius_bottom_right, radius_tri_left, radius_tri_right */

    int gradient_repeat_height;
    int gradient_repeat_direction = 0;
    alpha_color lower_frame_gradient_inner;
    alpha_color lower_frame_gradient_outer;
    if ((pfs->gradient_repeat_enabled == true && maximised == false)
            | (pfs->gradient_repeat_enabled == true
               && pws->gradient_repeat_disabled_maximised == false && maximised == true)) {
        gradient_repeat_height = pfs->gradient_repeat_height;
        //The bottom gradient is reversed so you have to flip it round
        lower_frame_gradient_inner = outer_color;
        lower_frame_gradient_outer = inner_color;
        if (pfs->gradient_repeat_direction_vertical == true) {
            gradient_repeat_direction = 1;
        } else if (pfs->gradient_repeat_direction_diagonal == true) {
            gradient_repeat_direction = 2;
        } else {
            gradient_repeat_direction = 0;
        }
    } else {
        lower_frame_gradient_inner = inner_color;
        lower_frame_gradient_outer = outer_color;
        gradient_repeat_height = 0;
    }

    if (enable_dip == true) {

////////////////////////////////Trim text//////////////////////////////////////////
        PangoLayoutLine* line;

        int name_length = pane_1_width * 1000 - (2000 * ptop);
        pango_layout_set_width(d->layout, name_length);
        //char name[strlen(pango_layout_get_text(d->layout)) + 3];
        const char* name;
        name = pango_layout_get_text(d->layout);
        pango_layout_set_wrap(d->layout, PANGO_WRAP_CHAR);

//////// //if anyone reports segfaults look at this first///////////////////////////
        if (pango_layout_get_line_count(d->layout) > 1) {
            line = pango_layout_get_line(d->layout, 0);
            name = g_strndup(name, line->length - 3);
            //name = strcat(name, "...");
            pango_layout_set_text(d->layout, name, line->length);
        }
///////////////////////////////////////////////////////////////////////////////////


///////////////////////////////Pixmaps//////////////////////////////////////////
        bool pixmaps_titlebarpart_enabled = false;
        bool pixmaps_buttonpart_enabled = false;
        if (pws->pixmaps.titlebarpart_enabled == true) {
            pixmaps_titlebarpart_enabled = true;
        }

        if (pws->pixmaps.buttonpart_enabled == true) {
            pixmaps_buttonpart_enabled = true;
        }

///////////////////////////////////////////////////////////////////////////////////


        fill_rounded_square(cr,
                            x_start - minimised_border - left_bar_dip_offset,
                            y1 + 0.5 - minimised_border,
                            pane_1_width + minimised_border - title_bar_dip_radius_offset,
                            top - 0.5 + minimised_border,
                            (CORNER_TOPRIGHT | CORNER_TOPLEFT | DIP_ROUND_TRI)  & corners,
                            &inner_title_color, &outer_title_color,
                            SHADE_TOP, ws,
                            pws->titlebar_radius, title_bar_dip_radius, 0, 0,
                            title_bar_dip_radius, 0, 2, false, 0, false, 0,
                            gradient_repeat_height, gradient_repeat_direction,
                            x1, y1, pws->pixmaps.titlebar_surface,
                            pixmaps_titlebarpart_enabled,
                            pws->pixmaps.titlebarpart_repeat_enabled);

        if (pws->enable_bar_dip_button_part == true) {
            fill_rounded_square(cr,
                                x_end - pws->title_bar_dip_button_width - left_bar_dip_offset + title_bar_dip_radius_offset,
                                y1 + 0.5 - minimised_border,
                                pws->title_bar_dip_button_width + minimised_border + left_bar_dip_offset - title_bar_dip_radius_offset,
                                top - 0.5 + minimised_border,
                                (CORNER_TOPLEFT | CORNER_TOPRIGHT | DIP_ROUND_TRI) & corners,
                                &inner_title_color, &outer_title_color,
                                SHADE_TOP, ws,
                                title_bar_dip_radius, pws->titlebar_radius, 0, 0, 0,
                                title_bar_dip_radius, 1, false, 0, false, 0,
                                gradient_repeat_height, gradient_repeat_direction,
                                x1, y1, pws->pixmaps.titlebar_surface_buttons,
                                pixmaps_buttonpart_enabled, pws->pixmaps.buttonpart_repeat_enabled);
        }
    } else {
        fill_rounded_square(cr,
                            x_start - minimised_border  - left_bar_dip_offset,
                            y1 + 0.5 - minimised_border,
                            width + (2 * minimised_border) + left_bar_dip_offset,
                            top - 0.5 + minimised_border,
                            (CORNER_TOPLEFT | CORNER_TOPRIGHT) & corners,
                            &inner_title_color, &outer_title_color,
                            SHADE_TOP, ws,
                            pws->titlebar_radius, pws->titlebar_radius, 0, 0,
                            0, 0, 0, false, 0, false, 0, gradient_repeat_height,
                            gradient_repeat_direction, x1, y1,
                            pws->pixmaps.titlebar_surface_large,
                            pws->pixmaps.titlebar_enabled,
                            pws->pixmaps.titlebar_repeat_enabled);
    }

    if ((maximised == false && pws->show_border_minimised == true) | (maximised == true && pws->show_border_maximised == true)) {

        fill_rounded_square(cr,
                            x_start - minimised_border - left_bar_dip_offset,
                            y1 + top,
                            ws->win_extents.left + minimised_border + left_bar_dip_offset,
                            h + window_gap + 1,
                            0,
                            &lower_frame_gradient_outer, &lower_frame_gradient_inner,
                            SHADE_TOP, ws,
                            10, 0, 0, 0, 0, 0, 0, enable_left_bar_dip,
                            pws->left_bar_dip_radius, pws->enable_left_bar_dip_lower_part,
                            pws->left_bar_dip_offset, gradient_repeat_height,
                            gradient_repeat_direction, x1, y1, pws->pixmaps.titlebar_surface, false, false);

        fill_rounded_square(cr,
                            x_end - ws->win_extents.right,
                            y1 + top,
                            ws->win_extents.right + minimised_border,
                            h + window_gap + 0.5,
                            0,
                            &lower_frame_gradient_outer, &lower_frame_gradient_inner,
                            SHADE_TOP, ws,
                            0, 0, 0, 0, 0, 0, 0, false, 0, false, 0,
                            gradient_repeat_height, gradient_repeat_direction,
                            x1, y1, pws->pixmaps.titlebar_surface, false, false);


        //Have to decrease the size of the bottom right corner as the left dip bar is cutting into it
        int tmp_minus = 0;
        if (pws->enable_left_bar_dip_lower_part == false && enable_left_bar_dip == true) {
            tmp_minus = (2 * pws->left_bar_dip_radius);
        }

        fill_rounded_square(cr,
                            x1 - window_gap - minimised_border + tmp_minus - left_bar_dip_offset,
                            y2 - ws->win_extents.bottom  + window_gap,
                            x2 - x1 + ws->win_extents.left - ws->win_extents.right - tmp_minus + left_bar_dip_offset + (window_gap * 2),
                            ws->win_extents.bottom - 0.5 + minimised_border,
                            (CORNER_BOTTOMLEFT | CORNER_BOTTOMRIGHT) & corners,
                            &outer_color, &inner_color,
                            SHADE_BOTTOM, ws,
                            0, 0, 0, pws->frame_radius, pws->frame_radius, 0, 0,
                            false, 0, false, 0, gradient_repeat_height,
                            gradient_repeat_direction, x1, y1,
                            pws->pixmaps.titlebar_surface, false, false);


    }

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    /* =====================titlebar separator line */

    if (pfs->separator_line.alpha != 0) {
        cairo_set_source_alpha_color(cr, &pfs->separator_line);
        cairo_move_to(cr, x1 + 4.5, y1 + top - 0.5);
        cairo_rel_line_to(cr, x2 - x1 - 9, 0.0);
        cairo_stroke(cr);
    }

    //FRAME

    rounded_rectangle(cr,
                      x1 + 0.5, y1 + 0.5,
                      x2 - x1 - 1.0, y2 - y1 - 1.0,
                      (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                       CORNER_BOTTOMRIGHT) & corners, ws,
                      pws->frame_radius);

    cairo_clip(cr);

    cairo_translate(cr, 1.0, 1.0);

    if (pfs->window_highlight.alpha != 0) {
        if (enable_dip == true) {
            rounded_rectangle_independent(cr,
                                          x1 + 0.5 - minimised_border - window_gap - left_bar_dip_offset,
                                          y1 - minimised_border,
                                          x2 - x1 + (2 * (minimised_border + window_gap)) + left_bar_dip_offset,
                                          y2 - y1 - 1.0 + window_gap + (2 * (minimised_border)),
                                          top - 0.5 + minimised_border,
                                          (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                                           CORNER_BOTTOMRIGHT | DIP_ROUND_TRI) & corners, ws,
                                          pws->titlebar_radius, pws->titlebar_radius,
                                          pws->frame_radius, pws->frame_radius,
                                          title_bar_dip_radius, minimised_border + pane_1_width - title_bar_dip_radius_offset,
                                          title_bar_dip_dip_width + (2 * title_bar_dip_radius_offset), true,
                                          do_button_part, enable_left_bar_dip, minimised_border + ws->win_extents.bottom,
                                          pws->left_bar_dip_radius, pws->enable_left_bar_dip_lower_part, pws->left_bar_dip_offset);
        } else {

            rounded_rectangle_independent(cr,
                                          x1 + 0.5 - minimised_border - window_gap - left_bar_dip_offset,
                                          y1 - minimised_border,
                                          x2 - x1 + (2 * (minimised_border + window_gap)) + left_bar_dip_offset,
                                          y2 - y1 - 1.0 + window_gap + (2 * (minimised_border)),
                                          top - 0.5 + minimised_border,
                                          (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                                           CORNER_BOTTOMRIGHT | DIP_ROUND_TRI) & corners, ws,
                                          pws->titlebar_radius, pws->titlebar_radius,
                                          pws->frame_radius, pws->frame_radius, 0, 0, 0, false,
                                          do_button_part, enable_left_bar_dip,
                                          minimised_border + ws->win_extents.bottom,
                                          pws->left_bar_dip_radius, pws->enable_left_bar_dip_lower_part,
                                          pws->left_bar_dip_offset);
        }

        //HIGHLIGHT HERE
        cairo_set_source_alpha_color(cr, &pfs->window_highlight);
        cairo_stroke(cr);
    }

    if (pfs->window_shadow.alpha != 0) {

        cairo_reset_clip(cr);
        cairo_translate(cr, -2.0, -2.0);

        if (enable_dip == true) {
            rounded_rectangle_independent(cr,
                                          x1 + 0.5 - minimised_border - window_gap - left_bar_dip_offset,
                                          y1 + 1.5 - minimised_border,
                                          x2 - x1 + (2 * (minimised_border + window_gap)) + left_bar_dip_offset,
                                          y2 - y1 - 1.0 + window_gap + (2 * (minimised_border)),
                                          top - 0.5 + minimised_border,
                                          (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                                           CORNER_BOTTOMRIGHT | DIP_ROUND_TRI) & corners, ws,
                                          pws->titlebar_radius, pws->titlebar_radius,
                                          pws->frame_radius, pws->frame_radius,
                                          title_bar_dip_radius, minimised_border + pane_1_width - title_bar_dip_radius_offset,
                                          title_bar_dip_dip_width + (2 * title_bar_dip_radius_offset),
                                          true, do_button_part, enable_left_bar_dip, minimised_border + ws->win_extents.bottom,
                                          pws->left_bar_dip_radius, pws->enable_left_bar_dip_lower_part, pws->left_bar_dip_offset);
        } else {
            rounded_rectangle_independent(cr,
                                          x1 + 0.5 - minimised_border - window_gap - left_bar_dip_offset,
                                          y1 + 1.5 - minimised_border,
                                          x2 - x1 + (2 * (minimised_border + window_gap)) + left_bar_dip_offset,
                                          y2 - y1 - 1.0 + window_gap + (2 * (minimised_border)),
                                          top - 0.5 + minimised_border,
                                          (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                                           CORNER_BOTTOMRIGHT | DIP_ROUND_TRI) & corners, ws,
                                          pws->titlebar_radius, pws->titlebar_radius,
                                          pws->frame_radius, pws->frame_radius, 0, 0, 0, false,
                                          do_button_part, enable_left_bar_dip,
                                          minimised_border + ws->win_extents.bottom,
                                          pws->left_bar_dip_radius, pws->enable_left_bar_dip_lower_part,
                                          pws->left_bar_dip_offset);
        }

        //SHADOW HERE
        cairo_set_source_alpha_color(cr, &pfs->window_shadow);
        cairo_stroke(cr);
    }


    if (pfs->window_frame_halo.alpha != 0) {
        printf("ok\n");
        cairo_reset_clip(cr);
        cairo_translate(cr, 1.0, 1.0);

        if (enable_dip == true) {
            rounded_rectangle_independent(cr,
                                          x1 + 0.5 - minimised_border - window_gap - left_bar_dip_offset,
                                          y1 + 0.5 - minimised_border,
                                          x2 - x1 + (2 * (minimised_border + window_gap)) + left_bar_dip_offset,
                                          y2 - y1 - 1.0 + window_gap + (2 * (minimised_border)),
                                          top - 0.5 + minimised_border,
                                          (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                                           CORNER_BOTTOMRIGHT | DIP_ROUND_TRI) & corners, ws,
                                          pws->titlebar_radius, pws->titlebar_radius,
                                          pws->frame_radius, pws->frame_radius,
                                          title_bar_dip_radius, minimised_border + pane_1_width - title_bar_dip_radius_offset,
                                          title_bar_dip_dip_width + (2 * title_bar_dip_radius_offset),
                                          true, do_button_part, enable_left_bar_dip,
                                          minimised_border + ws->win_extents.bottom,
                                          pws->left_bar_dip_radius,
                                          pws->enable_left_bar_dip_lower_part,
                                          pws->left_bar_dip_offset);
        } else {

            rounded_rectangle_independent(cr,
                                          x1 + 0.5 - minimised_border - window_gap - left_bar_dip_offset,
                                          y1 + 0.5 - minimised_border,
                                          x2 - x1 + (2 * (minimised_border + window_gap)) + left_bar_dip_offset,
                                          y2 - y1 - 1.5 + window_gap + (2 * (minimised_border)),
                                          top - 0.5 + minimised_border,
                                          (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                                           CORNER_BOTTOMRIGHT | DIP_ROUND_TRI) & corners, ws,
                                          pws->titlebar_radius, pws->titlebar_radius,
                                          pws->frame_radius, pws->frame_radius, 0, 0, 0,
                                          false, do_button_part, enable_left_bar_dip,
                                          minimised_border + ws->win_extents.bottom,
                                          pws->left_bar_dip_radius, pws->enable_left_bar_dip_lower_part,
                                          pws->left_bar_dip_offset);

        }

        cairo_set_source_alpha_color(cr, &pfs->window_frame_halo);
        cairo_stroke(cr);

    }



    //INNER STUFF

    //TODO - make this a bit more pixel-perfect...but it works for now

    if (pfs->contents_shadow.alpha != 0) {
        cairo_set_line_width(cr, 1.0);

        cairo_move_to(cr, pleft + pwidth + 1.5, ptop - 1);
        cairo_rel_line_to(cr, -pwidth - 2.5, 0);
        cairo_rel_line_to(cr, 0, pheight + 2.5);
        cairo_set_source_alpha_color(cr, &pfs->contents_shadow);
        cairo_stroke(cr);
    }

    if (pfs->contents_highlight.alpha != 0) {
        cairo_move_to(cr, pleft + pwidth + 1, ptop - 1.5);
        cairo_rel_line_to(cr, 0, pheight + 2.5);
        cairo_rel_line_to(cr, -pwidth - 2.5, 0);
        cairo_set_source_alpha_color(cr, &pfs->contents_highlight);
        cairo_stroke(cr);
    }

    if (pfs->contents_halo.alpha != 0) {
        cairo_move_to(cr, pleft, ptop);
        cairo_rel_line_to(cr, pwidth, 0);
        cairo_rel_line_to(cr, 0, pheight);
        cairo_rel_line_to(cr, -pwidth, 0);
        cairo_rel_line_to(cr, 0, -pheight);
        cairo_set_source_alpha_color(cr, &pfs->contents_halo);
        cairo_stroke(cr);
    }
}

extern "C"
void load_engine_settings(const KeyFile& f, window_settings* ws)
{
    private_ws* pws = ws->engine_ws;

    PFACS(outer);
    PFACS(inner);
    PFACS(title_outer);
    PFACS(title_inner);
    PFACS(window_frame_halo);
    PFACS(window_highlight);
    PFACS(window_shadow);
    PFACS(separator_line);
    PFACS(contents_shadow);
    PFACS(contents_highlight);
    PFACS(contents_halo);
    load_color_setting(f, &pws->outer_maximised_color, "outer_maximised_color", SECT);
    load_color_setting(f, &pws->inner_maximised_color, "inner_maximised_color", SECT);
    load_bool_setting(f, &pws->enable_maximised_colors, "enable_maximised_colors", SECT);
    load_bool_setting(f, &pws->gradient_repeat_disabled_maximised, "gradient_repeat_disabled_maximised", SECT);
    load_bool_setting(f, &pws->enable_title_bar_dip, "enable_title_bar_dip", SECT);
    load_bool_setting(f, &pws->round_top_left, "round_top_left", SECT);
    load_bool_setting(f, &pws->round_top_right, "round_top_right", SECT);
    load_bool_setting(f, &pws->round_bottom_left, "round_bottom_left", SECT);
    load_bool_setting(f, &pws->round_bottom_right, "round_bottom_right", SECT);
    load_bool_setting(f, &pws->round_tri, "round_tri", SECT);
    load_bool_setting(f, &pws->enable_bar_dip_button_part, "enable_bar_dip_button_part", SECT);
    load_bool_setting(f, &pws->enable_left_bar_dip, "enable_left_bar_dip", SECT);
    load_bool_setting(f, &pws->show_border_minimised, "show_border_minimised", SECT);
    load_bool_setting(f, &pws->enable_left_bar_dip_lower_part, "enable_left_bar_dip_lower_part", SECT);

    load_float_setting(f, &pws->outer_maximised_alpha, "outer_maximised_alpha", SECT);
    load_float_setting(f, &pws->inner_maximised_alpha, "inner_maximised_alpha", SECT);
    load_float_setting(f, &pws->frame_radius, "frame_radius", SECT);
    load_float_setting(f, &pws->titlebar_radius, "titlebar_radius", SECT);
    load_float_setting(f, &pws->window_gap, "window_gap", SECT);
    load_int_setting(f, &pws->title_bar_dip_title_width, "title_bar_dip_title_width", SECT);
    load_int_setting(f, &pws->title_bar_dip_radius, "title_bar_dip_radius", SECT);
    load_int_setting(f, &pws->title_bar_dip_button_width, "title_bar_dip_button_width", SECT);
    load_int_setting(f, &pws->minimised_border, "minimised_border", SECT);
    load_int_setting(f, &pws->left_bar_dip_radius, "left_bar_dip_radius", SECT);
    load_int_setting(f, &pws->left_bar_dip_offset, "left_bar_dip_offset", SECT);

    load_bool_setting(f, &pws->pixmaps.titlebarpart_enabled, "pixmaps_titlebarpart_enabled", SECT);
    load_bool_setting(f, &pws->pixmaps.buttonpart_enabled, "pixmaps_buttonpart_enabled", SECT);
    load_bool_setting(f, &pws->pixmaps.titlebarpart_repeat_enabled, "pixmaps_titlebarpart_repeat_enabled", SECT);
    load_bool_setting(f, &pws->pixmaps.buttonpart_repeat_enabled, "pixmaps_buttonpart_repeat_enabled", SECT);
    load_bool_setting(f, &pws->pixmaps.titlebar_repeat_enabled, "pixmaps_titlebar_repeat_enabled", SECT);
    load_bool_setting(f, &pws->pixmaps.titlebar_enabled, "pixmaps_titlebar_enabled", SECT);

    pws->pixmaps.titlebar_surface = cairo_image_surface_create_from_png(make_filename("pixmaps", "titlebarpart", "png").c_str());
    pws->pixmaps.titlebar_surface_buttons = cairo_image_surface_create_from_png(make_filename("pixmaps", "buttonpart", "png").c_str());
    pws->pixmaps.titlebar_surface_large = cairo_image_surface_create_from_png(make_filename("pixmaps", "titlebar", "png").c_str());

    load_bool_setting(f, &((private_fs*)ws->fs_act->engine_fs)->gradient_repeat_enabled, "active_gradient_repeat_enabled" , SECT);
    load_bool_setting(f, &((private_fs*)ws->fs_inact->engine_fs)->gradient_repeat_enabled, "inactive_gradient_repeat_enabled" , SECT);


    load_float_setting(f, &((private_fs*)ws->fs_act->engine_fs)->gradient_repeat_height, "active_gradient_repeat_height" , SECT);
    load_float_setting(f, &((private_fs*)ws->fs_inact->engine_fs)->gradient_repeat_height, "inactive_gradient_repeat_height" , SECT);


    load_bool_setting(f, &((private_fs*)ws->fs_act->engine_fs)->gradient_repeat_direction_vertical, "active_gradient_repeat_direction_vertical" , SECT);
    load_bool_setting(f, &((private_fs*)ws->fs_inact->engine_fs)->gradient_repeat_direction_vertical, "inactive_gradient_repeat_direction_vertical" , SECT);

    load_bool_setting(f, &((private_fs*)ws->fs_act->engine_fs)->gradient_repeat_direction_diagonal, "active_gradient_repeat_direction_diagonal" , SECT);
    load_bool_setting(f, &((private_fs*)ws->fs_inact->engine_fs)->gradient_repeat_direction_diagonal, "inactive_gradient_repeat_direction_diagonal" , SECT);



}

extern "C"
void init_engine(window_settings* ws)
{
    private_fs* pfs;
    private_ws* pws;

    pws = new private_ws;
    ws->engine_ws = pws;
    pws->round_top_left = true;
    pws->round_top_right = true;
    pws->round_bottom_left = true;
    pws->round_bottom_right = true;
    pws->round_tri = true;
    pws->enable_maximised_colors = true;
    pws->show_border_minimised = true;
    pws->enable_bar_dip_button_part = true;
    pws->show_border_maximised = true;
    pws->enable_title_bar_dip = true;
    pws->minimised_border = 0;
    pws->enable_left_bar_dip = true;
    pws->enable_left_bar_dip_lower_part = true;
    pws->left_bar_dip_offset = 0;
    pws->frame_radius = 5.0;
    pws->titlebar_radius = 5.0;
    pws->left_bar_dip_radius = 7.5;
    pws->gradient_repeat_disabled_maximised = true;
    pws->pixmaps.buttonpart_repeat_enabled = false;
    pws->pixmaps.buttonpart_enabled = false;
    pws->pixmaps.titlebarpart_repeat_enabled = false;
    pws->pixmaps.titlebarpart_enabled = false;
    pws->window_gap = 15.0;
    pws->title_bar_dip_title_width = 500;
    pws->title_bar_dip_button_width = 100;
    pws->title_bar_dip_radius = 20;
    pws->outer_maximised_alpha = 0.5;
    pws->inner_maximised_alpha = 0.5;
    pws->outer_maximised_color.r = 40.0;
    pws->outer_maximised_color.g = 40.0;
    pws->outer_maximised_color.b = 40.0;
    pws->inner_maximised_color.r = 40.0;
    pws->inner_maximised_color.g = 40.0;
    pws->inner_maximised_color.b = 40.0;

    pfs = new private_fs;
    ws->fs_act->engine_fs = pfs;
    ACOLOR(inner, 0.8, 0.8, 0.8, 0.5);
    ACOLOR(outer, 0.8, 0.8, 0.8, 0.5);
    ACOLOR(title_inner, 0.8, 0.8, 0.8, 0.8);
    ACOLOR(title_outer, 0.8, 0.8, 0.8, 0.8);
    ACOLOR(window_highlight, 1.0, 1.0, 1.0, 0.8);
    ACOLOR(window_shadow, 0.6, 0.6, 0.6, 0.8);
    ACOLOR(window_frame_halo, 1.0, 1.0, 1.0, 1.0);
    ACOLOR(separator_line, 0.0, 0.0, 0.0, 0.0);
    ACOLOR(contents_highlight, 1.0, 1.0, 1.0, 0.8);
    ACOLOR(contents_shadow, 0.6, 0.6, 0.6, 0.8);
    ACOLOR(contents_halo, 0.8, 0.8, 0.8, 0.8);

    pfs = new private_fs;
    ws->fs_inact->engine_fs = pfs;

    ACOLOR(inner, 0.8, 0.8, 0.8, 0.3);
    ACOLOR(outer, 0.8, 0.8, 0.8, 0.3);
    ACOLOR(title_inner, 0.8, 0.8, 0.8, 0.6);
    ACOLOR(title_outer, 0.8, 0.8, 0.8, 0.6);
    ACOLOR(window_highlight, 1.0, 1.0, 1.0, 0.7);
    ACOLOR(window_shadow, 0.6, 0.6, 0.6, 0.7);
    ACOLOR(window_frame_halo, 1.0, 1.0, 1.0, 1.0);
    ACOLOR(separator_line, 0.0, 0.0, 0.0, 0.0);
    ACOLOR(contents_highlight, 1.0, 1.0, 1.0, 0.8);
    ACOLOR(contents_shadow, 0.6, 0.6, 0.6, 0.8);
    ACOLOR(contents_halo, 0.8, 0.8, 0.8, 0.8);
}

extern "C"
void fini_engine(window_settings* ws)
{
    delete ((private_fs*)ws->fs_act->engine_fs);
    delete ((private_fs*)ws->fs_inact->engine_fs);
}

void layout_layout_frame(Gtk::Box& vbox)
{
    GtkWidget* hbox;
    GtkWidget* junk;

    {
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, false, false);
    hbox.pack_start(*Gtk::manage(new Gtk::Label(_("Border Gap"))),
                    false, false, 0);
    auto& scaler = *scaler_new(0, 20, 0.5);
    hbox.pack_start(scaler, true, true);
    SettingItem::create(scaler, SECT, "window_gap");
    }{
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, false, false);
    auto& btn = *Gtk::manage(new Gtk::CheckButton(_("Show Border when maximised?")));
    vbox.pack_start(btn, false, false);
    SettingItem::create(btn, SECT, "show_border_maximised");
    }{
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, false, false);
    auto& btn = *Gtk::manage(new Gtk::CheckButton(_("Show when minimised?")));
    vbox.pack_start(btn, false, false);
    SettingItem::create(btn, SECT, "show_border_minimised");
    }{
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, false, false);
    hbox.pack_start(*Gtk::manage(new Gtk::Label(_("Minimised Border Size"))),
                    false, false, 0);
    auto& scaler = *scaler_new(0, 30, 1);
    hbox.pack_start(scaler, true, true);
    SettingItem::create(scaler, SECT, "minimised_border");
    }
}

void layout_left_bar_frame(Gtk::Box& vbox)
{
    Gtk::Box* hbox;
    Gtk::CheckButton* btn;
    Gtk::Scale* scaler;

    hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, false, false);
    btn = Gtk::manage(new Gtk::CheckButton(_("Enable Left Bar Dip?")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "enable_left_bar_dip");

    hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, false, false);
    hbox->pack_start(*Gtk::manage(new Gtk::Label(_("Left Bar Radius"))), false, false, 0);
    scaler = scaler_new(0, 20, 1);
    hbox->pack_start(*scaler, true, true);
    SettingItem::create(*scaler, SECT, "left_bar_dip_radius");

    hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, false, false);
    btn = Gtk::manage(new Gtk::CheckButton(_("Enable Lower Bulge? (Useless at the moment, but will be used soon)")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "enable_left_bar_dip_lower_part");

    hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, false, false);
    hbox->pack_start(*Gtk::manage(new Gtk::Label(_("Top Corner Offset (%)"))), false, false, 0);
    scaler = scaler_new(0, 90, 1);
    hbox->pack_start(*scaler, true, true);
    SettingItem::create(*scaler, SECT, "left_bar_dip_offset");
}


void layout_title_bar_frame(Gtk::Box& vbox)
{
    Gtk::Box* hbox;
    Gtk::CheckButton* btn;
    Gtk::Scale* scaler;

    hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, false, false);
    btn = Gtk::manage(new Gtk::CheckButton(_("Enable Title Bar Dip?")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "enable_title_bar_dip");

    hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, false, false);
    btn = Gtk::manage(new Gtk::CheckButton(_("Enable Button Part?")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "enable_bar_dip_button_part");

    hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, false, false);
    hbox->pack_start(*Gtk::manage(new Gtk::Label(_("Title Part Width"))), false, false, 0);
    scaler = scaler_new(80, 800, 1);
    hbox->pack_start(*scaler, true, true);
    SettingItem::create(*scaler, SECT, "title_bar_dip_title_width");

    hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, false, false);
    hbox->pack_start(*Gtk::manage(new Gtk::Label(_("Button Part Width)"))), false, false, 0);
    scaler = scaler_new(10, 800, 1);
    hbox->pack_start(*scaler, true, true);
    SettingItem::create(*scaler, SECT, "title_bar_dip_button_width");

    hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, false, false);
    hbox->pack_start(*Gtk::manage(new Gtk::Label(_("Dip Corners Radius)"))), false, false, 0);
    scaler = scaler_new(1, 20, 1);
    hbox->pack_start(*scaler, true, true);
    SettingItem::create(*scaler, SECT, "title_bar_dip_radius");

    hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, false, false);
    btn = Gtk::manage(new Gtk::CheckButton(_("Round Inside Corners As well?")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "round_tri");
}

void layout_maximised_colors(Gtk::Box& vbox)
{
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, false, false);

    auto* btn = Gtk::manage(new Gtk::CheckButton(_("Enable Different Maximised Colors?")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "enable_maximised_colors");

    btn = Gtk::manage(new Gtk::CheckButton(_("Turn Off repeating gradients when maximised?")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "gradient_repeat_disabled_maximised");

    auto& scroller = *Gtk::manage(new Gtk::ScrolledWindow());
    scroller.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    vbox.pack_start(scroller, true, true);
    table_new(3, false, false);

    scroller.add(get_current_table());

    make_labels("Colors");
    table_append_separator();
    table_append(*Gtk::manage(new Gtk::Label(_("Outer Frame Blend"))), false);

    Gtk::ColorButton* cbtn;
    cbtn = Gtk::manage(new Gtk::ColorButton());
    table_append(*cbtn, false);
    SettingItem::create(*cbtn, SECT, "outer_maximised_color");

    Gtk::Scale* scaler;
    scaler = scaler_new(0.0, 1.0, 0.01);
    table_append(*scaler, true);
    SettingItem::create(*scaler, SECT, "outer_maximised_alpha");

    table_append(*Gtk::manage(new Gtk::Label(_("Inner Frame Blend"))), false);
    cbtn = Gtk::manage(new Gtk::ColorButton());
    table_append(*cbtn, false);
    SettingItem::create(*cbtn, SECT, "inner_maximised_color");
    scaler = scaler_new(0.0, 1.0, 0.01);
    table_append(*scaler, true);
    SettingItem::create(*scaler, SECT, "inner_maximised_alpha");
}

void layout_corners_frame(Gtk::Box& vbox)
{
    Gtk::Box* hbox;
    Gtk::CheckButton* btn;
    Gtk::Scale* scaler;
    btn = Gtk::manage(new Gtk::CheckButton(_("Round Top Left Corner?")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "round_top_left");

    btn = Gtk::manage(new Gtk::CheckButton(_("Round Top Right Corner")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "round_top_right");

    btn = Gtk::manage(new Gtk::CheckButton(_("Round Bottom Left Corner")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "round_bottom_left");

    btn = Gtk::manage(new Gtk::CheckButton(_("Round Bottom Right Corner")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "round_bottom_right");

    hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, false, false);
    hbox->pack_start(*Gtk::manage(new Gtk::Label(_("Frame Rounding Radius"))),
                     false, false, 0);

    scaler = scaler_new(0, 20, 0.5);
    hbox->pack_start(*scaler, true, true);
    SettingItem::create(*scaler, SECT, "frame_radius");

    hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, false, false);
    hbox->pack_start(*Gtk::manage(new Gtk::Label(_("Titlebar Rounding Radius"))),
                     false, false, 0);

    scaler = scaler_new(0, 20, 0.5);
    hbox->pack_start(*scaler, true, true);
    SettingItem::create(*scaler, SECT, "titlebar_radius");

}

void add_border_slider(char* text, char* key, int value)
{
    table_append(*Gtk::manage(new Gtk::Label(text)), false);

    auto& scaler = *scaler_new(0, 20, 1);
    table_append(scaler, true);
    scaler.set_value(value);
    SettingItem::create(scaler, "borders", key);
}

void my_engine_settings(Gtk::Box& hbox, bool active)
{

    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    hbox.pack_start(vbox, true, true);
    vbox.pack_start(*Gtk::manage(new Gtk::Label(active ? "Active Window" : "Inactive Window")),
                    false, false);
    vbox.pack_start(*Gtk::manage(new Gtk::HSeparator()), false, false);

    auto& scroller = *Gtk::manage(new Gtk::ScrolledWindow());
    scroller.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    vbox.pack_start(scroller, true, true);

    table_new(3, false, false);

    scroller.add(get_current_table());

    make_labels(_("Colors"));
    table_append_separator();
    add_color_alpha_value(_("Outer Frame Blend"), "outer", SECT, active);
    add_color_alpha_value(_("Inner Frame Blend"), "inner", SECT, active);
    table_append_separator();
    add_color_alpha_value(_("Outer Titlebar Blend"), "title_outer", SECT, active);
    add_color_alpha_value(_("Inner Titlebar Blend"), "title_inner", SECT, active);

    table_append_separator();

    table_append(*Gtk::manage(new Gtk::Label(_("Repeat Gradient?"))), false);
    table_append(*Gtk::manage(new Gtk::Label("")), false);

    Gtk::CheckButton* btn;
    char* key_line = g_strdup_printf(active ? "active_%s" : "inactive_%s", "gradient_repeat_enabled");
    btn = Gtk::manage(new Gtk::CheckButton());
    table_append(*btn, true);
    SettingItem::create(*btn, SECT, key_line);

    table_append(*Gtk::manage(new Gtk::Label(_("Vertical Repeat?"))), false);
    table_append(*Gtk::manage(new Gtk::Label("")), false);
    key_line = g_strdup_printf(active ? "active_%s" : "inactive_%s", "gradient_repeat_direction_vertical");
    btn = Gtk::manage(new Gtk::CheckButton());
    table_append(*btn, true);
    SettingItem::create(*btn, SECT, key_line);

    key_line = g_strdup_printf(active ? "active_%s" : "inactive_%s", "gradient_repeat_direction_diagonal");
    table_append(*Gtk::manage(new Gtk::Label(_("Diagonal Repeat?"))), false);
    table_append(*Gtk::manage(new Gtk::Label("")), false);
    btn = Gtk::manage(new Gtk::CheckButton());
    table_append(*btn, true);
    SettingItem::create(*btn, SECT, key_line);

    table_append(*Gtk::manage(new Gtk::Label(_("Repeat Frequency"))), false);
    table_append(*Gtk::manage(new Gtk::Label("")), false);
    key_line = g_strdup_printf(active ? "active_%s" : "inactive_%s", "gradient_repeat_height");
    auto& scaler = *scaler_new(0, 20, 0.5);
    table_append(scaler, true);
    SettingItem::create(scaler, SECT, key_line);
    table_append_separator();

    add_color_alpha_value(_("Titlebar Separator"), "separator_line", SECT, active);
    table_append_separator();
    add_color_alpha_value(_("Frame Outline"), "window_frame_halo", SECT, active);
    add_color_alpha_value(_("Frame Highlight"), "window_highlight", SECT, active);
    add_color_alpha_value(_("Frame Shadow"), "window_shadow", SECT, active);
    table_append_separator();
    add_color_alpha_value(_("Contents Outline"), "contents_halo", SECT, active);
    add_color_alpha_value(_("Contents Highlight"), "contents_highlight", SECT, active);
    add_color_alpha_value(_("Contents Shadow"), "contents_shadow", SECT, active);

    add_color_alpha_value(_("Titlebar Separator"), "separator_line", SECT, active);
    table_append_separator();
    add_color_alpha_value(_("Contents Shadow"), "contents_shadow", SECT, active);
}

void layout_engine_colors(Gtk::Box& vbox)
{
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, true, true);
    my_engine_settings(hbox, true);
    hbox.pack_start(*Gtk::manage(new Gtk::VSeparator()), false, false);
    my_engine_settings(hbox, false);
}

void layout_pixmaps(Gtk::Box& vbox)
{
    Gtk::CheckButton* btn;
    btn = Gtk::manage(new Gtk::CheckButton(_("Enable Title Part Pixmap?")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "pixmaps_titlebarpart_enabled");

    btn = Gtk::manage(new Gtk::CheckButton(_("Repeat Title Part Pixmap?")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "pixmaps_titlebarpart_repeat_enabled");

    Gtk::Box* hbox;
    hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, false, false);

    hbox->pack_start(*Gtk::manage(new Gtk::Label(_("Title Bar Part Pixmap"))), false, false, 0);

    Gtk::FileChooserButton* filesel;

    {
    filesel = Gtk::manage(new Gtk::FileChooserButton(_("Choose Titlebar Part Pixmap"),
                                                     Gtk::FILE_CHOOSER_ACTION_OPEN));
    hbox->pack_start(*filesel, true, true);
    Gtk::FileFilter filter;
    filter.add_pattern("*.png");
    filter.set_name("PNG Images");
    filter.add_pixbuf_formats();
    filesel->add_filter(filter);

    auto* img = Gtk::manage(new Gtk::Image());
    /* set =*/ SettingItem::register_img_file_setting(*filesel, "pixmaps", "titlebarpart", img);
    }

    //----------------
    hbox->pack_start(*Gtk::manage(new Gtk::VSeparator()), false, false);


    btn = Gtk::manage(new Gtk::CheckButton(_("Enable Button Part Pixmap?")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "pixmaps_buttonpart_enabled");

    btn = Gtk::manage(new Gtk::CheckButton(_("Repeat Button Part Pixmap?")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "pixmaps_buttonpart_repeat_enabled");


    hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, false, false);
    hbox->pack_start(*Gtk::manage(new Gtk::Label(_("Button Part Pixmap"))), false, false, 0);

    {
    filesel = Gtk::manage(new Gtk::FileChooserButton(_("Choose Button Part Pixmap"),
                                                     Gtk::FILE_CHOOSER_ACTION_OPEN));
    hbox->pack_start(*filesel, true, true);
    Gtk::FileFilter filter;
    filter.add_pattern("*.png");
    filter.set_name("PNG Images");
    filter.add_pixbuf_formats();
    filesel->add_filter(filter);

    auto img = Gtk::manage(new Gtk::Image());
    /* set =*/ SettingItem::register_img_file_setting(*filesel, "pixmaps", "buttonpart", img);
    }

    //----------------
    hbox->pack_start(*Gtk::manage(new Gtk::VSeparator()), false, false);

    btn = Gtk::manage(new Gtk::CheckButton(_("Enable Titlebar Pixmap?")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "pixmaps_titlebar_enabled");

    btn = Gtk::manage(new Gtk::CheckButton(_("Repeat Titlebar Pixmap?")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "pixmaps_titlebar_repeat_enabled");

    hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, false, false);
    hbox->pack_start(*Gtk::manage(new Gtk::Label(_("Titlebar Pixmap"))), false, false);

    {
    filesel = Gtk::manage(new Gtk::FileChooserButton(_("Choose Titlebar Pixmap"),
                                                     Gtk::FILE_CHOOSER_ACTION_OPEN));
    hbox->pack_start(*filesel, true, true);
    Gtk::FileFilter filter;
    filter.add_pattern("*.png");
    filter.set_name("PNG Images");
    filter.add_pixbuf_formats();
    filesel->add_filter(filter);

    auto* img = Gtk::manage(new Gtk::Image());
    /* set =*/ SettingItem::register_img_file_setting(*filesel, "pixmaps", "titlebar", img);
    }
}

extern "C"
void layout_engine_settings(Gtk::Box& vbox)
{
    auto* note = Gtk::manage(new Gtk::Notebook());
    vbox.pack_start(*note, true, true);

    layout_engine_colors(*build_notebook_page(_("Active/Inactive"), *note));
    layout_maximised_colors(*build_notebook_page(_("Maximised"), *note));
    layout_corners_frame(*build_notebook_page(_("Corners"), *note));
    layout_layout_frame(*build_notebook_page(_("Border Layout"), *note));
    layout_title_bar_frame(*build_notebook_page(_("Title Bar"), *note));
    layout_left_bar_frame(*build_notebook_page(_("Left Bar"), *note));
    layout_pixmaps(*build_notebook_page(_("Pixmaps"), *note));
}
