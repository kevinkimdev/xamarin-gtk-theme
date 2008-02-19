/* Murrine theme engine
 * Copyright (C) 2007 Andrea Cimitan
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "murrine_draw.h"
#include "murrine_style.h"
#include "murrine_types.h"

#include "support.h"
#include "cairo-support.h"

#include <cairo.h>

#define M_PI 3.14159265358979323846

static void
murrine_draw_inset (cairo_t          *cr,
                    const MurrineRGB *bg_color,
                    double x, double y, double w, double h,
                    double radius, uint8 corners)
{
	MurrineRGB highlight, shadow;

	murrine_shade (bg_color, 1.15, &highlight);
	murrine_shade (bg_color, 0.4, &shadow);

	/* highlight */
	cairo_move_to (cr, x + w + (radius * -0.2928932188), y - (radius * -0.2928932188)); /* 0.2928932... 1-sqrt(2)/2 gives middle of curve */

	if (corners & MRN_CORNER_TOPRIGHT)
		cairo_arc (cr, x + w - radius, y + radius, radius, M_PI * 1.75, M_PI * 2);
	else
		cairo_line_to (cr, x + w, y);

	if (corners & MRN_CORNER_BOTTOMRIGHT)
		cairo_arc (cr, x + w - radius, y + h - radius, radius, 0, M_PI * 0.5);
	else
		cairo_line_to (cr, x + w, y + h);

	if (corners & MRN_CORNER_BOTTOMLEFT)
		cairo_arc (cr, x + radius, y + h - radius, radius, M_PI * 0.5, M_PI * 0.75);
	else
		cairo_line_to (cr, x, y + h);

	murrine_set_color_rgba (cr, &highlight, 0.48);
	cairo_stroke (cr);

	/* shadow */
	cairo_move_to (cr, x + (radius * 0.2928932188), y + h + (radius * -0.2928932188));

	if (corners & MRN_CORNER_BOTTOMLEFT)
		cairo_arc (cr, x + radius, y + h - radius, radius, M_PI * 0.75, M_PI);
	else
		cairo_line_to (cr, x, y + h);

	if (corners & MRN_CORNER_TOPLEFT)
		cairo_arc (cr, x + radius, y + radius, radius, M_PI, M_PI * 1.5);
	else
		cairo_line_to (cr, x, y);

	if (corners & MRN_CORNER_TOPRIGHT)
	    cairo_arc (cr, x + w - radius, y + radius, radius, M_PI * 1.5, M_PI * 1.75);
	else
		cairo_line_to (cr, x + w, y);

	murrine_set_color_rgba (cr, &shadow, 0.12);
	cairo_stroke (cr);
}

static void
murrine_draw_highlight_and_shade (cairo_t *cr,
                                  const MurrineColors *colors,
                                  const ShadowParameters *widget,
                                  int width, int height, int radius)
{
	MurrineRGB highlight;
	MurrineRGB shadow;
	uint8 corners = widget->corners;
	double x = 1.0;
	double y = 1.0;

	murrine_shade (&colors->bg[0], 1.15, &highlight);
	murrine_shade (&colors->bg[0], 0.4, &shadow);

	width  -= 3;
	height -= 3;

	cairo_save (cr);

	/* Top/Left highlight */
	if (corners & MRN_CORNER_BOTTOMLEFT)
		cairo_move_to (cr, x, y+height-radius);
	else
		cairo_move_to (cr, x, y+height);

	murrine_rounded_corner (cr, x, y, radius, corners & MRN_CORNER_TOPLEFT);

	if (corners & MRN_CORNER_TOPRIGHT)
		cairo_line_to (cr, x+width-radius, y);
	else
		cairo_line_to (cr, x+width, y);

	if (widget->shadow & MRN_SHADOW_OUT)
		murrine_set_color_rgba (cr, &highlight, 0.5);
	else
		murrine_set_color_rgba (cr, &shadow, 0.13);

	cairo_stroke (cr);

	/* Bottom/Right highlight -- this includes the corners */
	cairo_move_to (cr, x+width-radius, y); /* topright and by radius to the left */
	murrine_rounded_corner (cr, x+width, y, radius, corners & MRN_CORNER_TOPRIGHT);
	murrine_rounded_corner (cr, x+width, y+height, radius, corners & MRN_CORNER_BOTTOMRIGHT);
	murrine_rounded_corner (cr, x, y+height, radius, corners & MRN_CORNER_BOTTOMLEFT);

	if (widget->shadow & MRN_SHADOW_OUT)
		murrine_set_color_rgba (cr, &shadow, 0.13);
	else
		murrine_set_color_rgba (cr, &highlight, 0.5);

	cairo_stroke (cr);

	cairo_restore (cr);
}

static void
murrine_rgba_draw_button (cairo_t *cr,
                          const MurrineColors    *colors,
                          const WidgetParameters *widget,
                          int x, int y, int width, int height,
                          boolean horizontal)
{
	double xoffset = 0, yoffset = 0;
	MurrineRGB fill = colors->bg[widget->state_type];
	MurrineRGB border_disabled = colors->shade[6];
	MurrineRGB border_normal;
	MurrineRGB highlight;

	double custom_highlight_ratio = widget->highlight_ratio;
	MurrineGradients mrn_gradient_custom = widget->mrn_gradient;

	if (widget->disabled)
	{
		mrn_gradient_custom.gradient_stop_1 = get_decreased_ratio (widget->mrn_gradient.gradient_stop_1, 3.0);
		mrn_gradient_custom.gradient_stop_2 = get_decreased_ratio (widget->mrn_gradient.gradient_stop_2, 3.0);
		mrn_gradient_custom.gradient_stop_3 = get_decreased_ratio (widget->mrn_gradient.gradient_stop_3, 3.0);
		mrn_gradient_custom.gradient_stop_4 = get_decreased_ratio (widget->mrn_gradient.gradient_stop_4, 3.0);
		custom_highlight_ratio = get_decreased_ratio (widget->highlight_ratio, 2.0);
	}

	if (widget->is_default)
		murrine_mix_color (&fill, &colors->spot[1], 0.2, &fill);

	if (!horizontal)
		murrine_exchange_axis (cr, &x, &y, &width, &height);

	murrine_shade (&colors->shade[8], 0.95, &border_normal);
	murrine_shade (&fill, custom_highlight_ratio, &highlight);

	cairo_translate (cr, x, y);
	cairo_set_line_width (cr, 1.0);

	if (widget->xthickness > 1)
		xoffset = 1;
	if (widget->ythickness > 1)
		yoffset = 1;

	if (!widget->active)
	{
		murrine_rounded_rectangle (cr, xoffset, yoffset, width-(xoffset*2), height-(yoffset*2), widget->roundness, widget->corners);
		murrine_set_color_rgba (cr, widget->disabled ? &border_disabled : &border_normal, 0.16);
		cairo_stroke (cr);
	}

	murrine_mix_color (widget->disabled ? &border_disabled : &border_normal , &widget->parentbg, 0.2,
	                   widget->disabled ? &border_disabled : &border_normal);

	murrine_mix_color (widget->disabled ? &border_disabled : &border_normal , &fill, 0.25,
	                   widget->disabled ? &border_disabled : &border_normal);

	/* Default button */
	if (widget->is_default)
	{
		murrine_rounded_rectangle (cr, xoffset, yoffset, width-(xoffset*2), height-(yoffset*2), widget->roundness, widget->corners);
		murrine_set_color_rgba (cr, &colors->spot[1], 0.6);
		cairo_stroke (cr);
		murrine_shade (&border_normal, 0.8, &border_normal);
	}

	/* Draw the bg */
	if (widget->roundness < 2)
		cairo_rectangle (cr, xoffset + 1, yoffset + 1, width-(xoffset*2)-2, height-(yoffset*2)-2);
	else
		clearlooks_rounded_rectangle (cr, xoffset+0.5, yoffset+0.5, width-(xoffset*2)-1, height-(yoffset*2)-1, widget->roundness+1, widget->corners);
	murrine_set_gradient (cr, &fill, mrn_gradient_custom, xoffset+1, yoffset+1, 0, height-(yoffset*2)-2, widget->mrn_gradient.gradients, FALSE);

	cairo_save (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	if (widget->roundness > 1)
		cairo_clip_preserve (cr);

	int curve_pos = 1;
	if (widget->glazestyle != 4)
		curve_pos = 2;

	/* Draw the glass effect */
	if (widget->glazestyle > 0)
	{
		cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
		widget->glazestyle == 2 ? cairo_fill_preserve (cr) : cairo_fill (cr);
		if (widget->glazestyle < 3)
			murrine_draw_curved_highlight (cr, curve_pos, width, height);
		else
			murrine_draw_curved_highlight_top (cr, curve_pos, width, height);
	}
	else
	{
		cairo_fill (cr);
		murrine_draw_flat_highlight (cr, xoffset + 1, yoffset + 1, width-(xoffset*2)-2, height-(yoffset*2)-2);
	}

	murrine_set_gradient (cr, &highlight, mrn_gradient_custom, xoffset+1, yoffset+1, 0, height-(yoffset*2)-2, widget->mrn_gradient.gradients, TRUE);
	cairo_fill (cr);

	if (widget->glazestyle == 4)
	{
		murrine_draw_curved_highlight_bottom (cr, curve_pos, width, height);
		MurrineRGB shadow;
		murrine_shade (&fill, 1.0/custom_highlight_ratio, &shadow);
		murrine_set_gradient (cr, &shadow, mrn_gradient_custom, xoffset+1, yoffset+1, 0, height-(yoffset*2)-2, widget->mrn_gradient.gradients, TRUE);
		cairo_fill (cr);
	}

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	/* Draw the white inner border */
	if (widget->glazestyle != 4 && !widget->active)
	{
		murrine_shade (&fill,  widget->innerborder_ratio*custom_highlight_ratio, &highlight);
		if (horizontal)
		{
			murrine_draw_innerborder (cr, &highlight, &fill, mrn_gradient_custom,
			                          xoffset + 1.5, yoffset + 1.5,
			                          width-(xoffset*2)-3, height-(yoffset*2)-3,
			                          widget->mrn_gradient.gradients, horizontal,
			                          widget->glazestyle, widget->roundness, widget->corners);
		}
		else
		{
			murrine_exchange_axis (cr, &x, &y, &width, &height);
			murrine_draw_innerborder (cr, &highlight, &fill, mrn_gradient_custom,
			                          xoffset + 1.5, yoffset + 1.5,
			                          width-(xoffset*2)-3, height-(yoffset*2)-3,
			                          widget->mrn_gradient.gradients, horizontal,
			                          widget->glazestyle, widget->roundness, widget->corners);
			murrine_exchange_axis (cr, &x, &y, &width, &height);
		}
	}

	cairo_restore (cr);

	/* Draw pressed button shadow */
	if (widget->active)
	{
		cairo_pattern_t *pattern;
		MurrineRGB shadow;

		murrine_shade (&fill, 0.94, &shadow);

		cairo_save (cr);

		if (widget->roundness < 2)
			cairo_rectangle (cr, xoffset + 1, yoffset + 1, width-(xoffset*2)-2, height-(yoffset*2)-2);
		else
			clearlooks_rounded_rectangle (cr, xoffset+1, yoffset+1, width-(xoffset*2)-2, height-(yoffset*2)-2, widget->roundness-1,
			                              widget->corners & (MRN_CORNER_TOPLEFT | MRN_CORNER_TOPRIGHT | MRN_CORNER_BOTTOMLEFT));

		cairo_clip (cr);

		cairo_rectangle (cr, xoffset+1, yoffset+1, width-(xoffset*2)-2, 3);
		pattern = cairo_pattern_create_linear (xoffset+1, yoffset+1, xoffset+1, yoffset+4);
		cairo_pattern_add_color_stop_rgba (pattern, 0.0, shadow.r, shadow.g, shadow.b, 0.58);
		cairo_pattern_add_color_stop_rgba (pattern, 1.0, shadow.r, shadow.g, shadow.b, 0.0);
		cairo_set_source (cr, pattern);
		cairo_fill (cr);
		cairo_pattern_destroy (pattern);

		cairo_rectangle (cr, xoffset+1, yoffset+1, 3, height-(yoffset*2)-2);
		pattern = cairo_pattern_create_linear (xoffset+1, yoffset+1, xoffset+4, yoffset+1);
		cairo_pattern_add_color_stop_rgba (pattern, 0.0, shadow.r, shadow.g, shadow.b, 0.58);
		cairo_pattern_add_color_stop_rgba (pattern, 1.0, shadow.r, shadow.g, shadow.b, 0.0);
		cairo_set_source (cr, pattern);
		cairo_fill (cr);
		cairo_pattern_destroy (pattern);

		cairo_restore (cr);

		murrine_draw_inset (cr, &widget->parentbg, xoffset-0.5, yoffset-0.5, width-(xoffset*2)+1, height-(yoffset*2)+1, widget->roundness+1, widget->corners);
	}

	/* Draw the border */
	murrine_set_color_rgb (cr, widget->disabled ? &border_disabled : &border_normal);
	murrine_rounded_rectangle (cr, xoffset+0.5, yoffset+0.5, width-(xoffset*2)-1, height-(yoffset*2)-1, widget->roundness, widget->corners);
	cairo_stroke (cr);
}

static void
murrine_rgba_draw_entry (cairo_t *cr,
                         const MurrineColors    *colors,
                         const WidgetParameters *widget,
                         int x, int y, int width, int height)
{
	const MurrineRGB *base = &colors->base[widget->state_type];
	MurrineRGB border = colors->shade[widget->disabled ? 4 : 5];
	int xoffset, yoffset = 0;
	int radius = CLAMP (widget->roundness, 0, 3);

	if (widget->xthickness > 1)
		xoffset = 1;
	if (widget->ythickness > 1)
		yoffset = 1;

	murrine_shade (&border, 0.92, &border);

	cairo_translate (cr, x+0.5, y+0.5);
	cairo_set_line_width (cr, 1.0);

	/* Draw (erase) the background */
	/* // CLEARING should be useless... testing
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	*/
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	/* Fill the background (shouldn't have to) */
	cairo_rectangle (cr, -0.5, -0.5, width, height);
	murrine_set_color_rgba (cr, &widget->parentbg, WINDOW_OPACITY);
	cairo_fill (cr);

	/* Fill the entry's base color (why isn't is large enough by default?) */
	cairo_rectangle (cr, 1.5, 1.5, width-3, height-3);
	murrine_set_color_rgba (cr, base, ENTRY_OPACITY);
	cairo_fill (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	murrine_draw_inset (cr, &widget->parentbg, 0, 0, width-1, height-1, radius+1, widget->corners);

	/* Draw the focused border */
	if (widget->focus)
	{
		cairo_rectangle (cr, 2, 2, width-5, height-5);
		murrine_set_color_rgba (cr, &colors->spot[1], 0.6);
		cairo_stroke(cr);
	}
	else
	{
		MurrineRGB highlight;
		murrine_shade (base, 1.15, &highlight);

		cairo_move_to (cr, 2, height-3);
		cairo_line_to (cr, 2, 2);
		cairo_line_to (cr, width-3, 2);

		murrine_set_color_rgba (cr, &highlight, widget->disabled ? 0.3 : 0.6);
		cairo_stroke (cr);
	}

	/* Draw the border */
	if (widget->focus)
		murrine_set_color_rgb (cr, &colors->spot[2]);
	else
		murrine_set_color_rgb (cr, &border);

	murrine_rounded_rectangle (cr, 1, 1, width-3, height-3, radius, widget->corners);
	cairo_stroke (cr);
}

static void
murrine_scale_draw_gradient (cairo_t *cr,
                             const MurrineRGB *c1,
                             const MurrineRGB *c2,
                             int x, int y, int width, int height,
                             boolean alpha)
{
	if (alpha)
		murrine_set_color_rgba (cr, c1, 0.4);
	else
		murrine_set_color_rgb (cr, c1);

	cairo_rectangle (cr, x, y, width, height);
	cairo_fill (cr);

	cairo_rectangle (cr, x, y, width, height);
	murrine_set_color_rgba (cr, c2, 0.82);
	cairo_stroke (cr);
}

static void
murrine_rgba_draw_scale_trough (cairo_t *cr,
                                const MurrineColors    *colors,
                                const WidgetParameters *widget,
                                const SliderParameters *slider,
                                int x, int y, int width, int height)
{
	int     fill_x, fill_y, fill_width, fill_height; /* Fill x,y,w,h */
	int     trough_width, trough_height;
	double  translate_x, translate_y;
	int     fill_size = slider->fill_size;
	int     TROUGH_SIZE = 6;

	if (slider->horizontal)
	{
		if (fill_size > width-3)
			fill_size = width-3;

		fill_x        = slider->inverted ? width - fill_size - 3 : 0;
		fill_y        = 0;
		fill_width    = fill_size;
		fill_height   = TROUGH_SIZE-2;

		trough_width  = width-3;
		trough_height = TROUGH_SIZE-2;

		translate_x   = x + 0.5;
		translate_y   = y + 0.5 + (height/2) - (TROUGH_SIZE/2);
	}
	else
	{
		if (fill_size > height-3)
			fill_size = height-3;

		fill_x        = 0;
		fill_y        = slider->inverted ? height - fill_size - 3 : 0;
		fill_width    = TROUGH_SIZE-2;
		fill_height   = fill_size;

		trough_width  = TROUGH_SIZE-2;
		trough_height = height-3;

		translate_x   = x + 0.5 + (width/2) - (TROUGH_SIZE/2);
		translate_y   = y + 0.5;
	}

	cairo_set_line_width (cr, 1.0);
	cairo_translate (cr, translate_x, translate_y);

	murrine_draw_inset (cr, &widget->parentbg, 0, 0, trough_width+2, trough_height+2, 0, 0);

	cairo_translate (cr, 1, 1);

	murrine_scale_draw_gradient (cr, &colors->shade[1],
	                             &colors->shade[5], /* border */
	                             0, 0, trough_width, trough_height,
	                             TRUE);

	murrine_scale_draw_gradient (cr, &colors->spot[1],
	                             &colors->spot[2], /* border */
	                             fill_x, fill_y, fill_width, fill_height,
	                             FALSE);
}

static void
murrine_rgba_draw_progressbar_trough (cairo_t *cr,
                                      const MurrineColors    *colors,
                                      const WidgetParameters *widget,
                                      int x, int y, int width, int height)
{
	const MurrineRGB *border = &colors->shade[4];

	cairo_set_line_width (cr, 1.0);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	/* Fill with bg color */
	cairo_rectangle (cr, x, y, width, height);
	murrine_set_color_rgba (cr, &colors->bg[widget->state_type], 0.8);
	cairo_fill (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	/* Create trough box */
	cairo_rectangle (cr, x+1, y+1, width-2, height-2);
	murrine_set_color_rgba (cr, &colors->shade[1], 0.8);
	cairo_fill (cr);

	/* Draw border */
	cairo_rectangle (cr, x+0.5, y+0.5, width-1, height-1);
	murrine_set_color_rgba (cr, border, 0.8);
	cairo_stroke (cr);
}

static void
murrine_rgba_draw_progressbar_fill (cairo_t *cr,
                                    const MurrineColors         *colors,
                                    const WidgetParameters      *widget,
                                    const ProgressBarParameters *progressbar,
                                    int x, int y, int width, int height,
                                    gint offset)
{
	boolean    is_horizontal = progressbar->orientation < 2;
	double     tile_pos = 0;
	double     stroke_width;
	int        x_step;
	const      MurrineRGB *fill = &colors->spot[1];
	const      MurrineRGB *border = &colors->spot[2];
	MurrineRGB highlight;

	murrine_shade (fill, widget->highlight_ratio, &highlight);

	cairo_rectangle (cr, x, y, width, height);

	if (is_horizontal)
	{
		if (progressbar->orientation == MRN_ORIENTATION_LEFT_TO_RIGHT)
			rotate_mirror_translate (cr, 0, x, y, FALSE, FALSE);
		else
			rotate_mirror_translate (cr, 0, x+width, y, TRUE, FALSE);
	}
	else
	{
		int tmp = height; height  = width; width   = tmp;

		x = x + 1;
		y = y - 1;
		width = width + 2;
		height = height - 2;

		if (progressbar->orientation == MRN_ORIENTATION_TOP_TO_BOTTOM)
			rotate_mirror_translate (cr, M_PI/2, x, y, FALSE, FALSE);
		else
			rotate_mirror_translate (cr, M_PI/2, x, y+width, TRUE, FALSE);
	}

	cairo_save (cr);
	cairo_clip (cr);

	stroke_width = height*2;
	x_step = (((float)stroke_width/10)*offset);
	cairo_set_line_width (cr, 1.0);

	cairo_save (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	cairo_rectangle (cr, 1.5, 0.5, width-2, height-1);

	/* Draw fill */
	murrine_set_gradient (cr, fill, widget->mrn_gradient, 1.5, 0.5, 0, height-1, widget->mrn_gradient.gradients, FALSE);

	/* Draw the glass effect */
	if (widget->glazestyle > 0)
	{
		widget->glazestyle == 2 ? cairo_fill_preserve (cr) : cairo_fill (cr);
		if (widget->glazestyle < 3)
			murrine_draw_curved_highlight (cr, 1, width, height);
		else
			murrine_draw_curved_highlight_top (cr, 1, width, height);
	}
	else
	{
		cairo_fill (cr);
		murrine_draw_flat_highlight (cr, 1.5, 0.5, width-2, height);
	}

	murrine_set_gradient (cr, &highlight, widget->mrn_gradient, 1.5, 0.5, 0, height-1, widget->mrn_gradient.gradients, TRUE);
	cairo_fill (cr);

	if (widget->glazestyle == 4)
	{
		murrine_draw_curved_highlight_bottom (cr, 1, width, height+1);
		MurrineRGB shadow;
		murrine_shade (fill, 1.0/widget->highlight_ratio, &shadow);
		murrine_set_gradient (cr, &shadow, widget->mrn_gradient, 1.5, 0.5, 0, height-1, widget->mrn_gradient.gradients, TRUE);
		cairo_fill (cr);
	}

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	murrine_shade (fill, widget->highlight_ratio*widget->innerborder_ratio, &highlight);
	murrine_draw_innerborder (cr, &highlight, fill, widget->mrn_gradient,
	                          2.5, 1.5,
	                          width-5, height-3,
	                          widget->mrn_gradient.gradients, TRUE,
	                          widget->glazestyle, 0, MRN_CORNER_NONE);

	/* Draw strokes */
	while (tile_pos <= width+x_step-2)
	{
		cairo_move_to (cr, stroke_width/2-x_step, 0);
		cairo_line_to (cr, stroke_width-x_step,   0);
		cairo_line_to (cr, stroke_width/2-x_step, height);
		cairo_line_to (cr, -x_step, height);

		cairo_translate (cr, stroke_width, 0);
		tile_pos += stroke_width;
	}

	murrine_set_color_rgba (cr, border, 0.15);
	cairo_fill (cr);
	cairo_restore (cr);

	/* Draw the border */
	murrine_set_color_rgba (cr, border, 0.8);
	cairo_rectangle (cr, 1.5, 0.5, width-3, height-1);
	cairo_stroke (cr);
}

static void
murrine_rgba_draw_menubar (cairo_t *cr,
                           const MurrineColors *colors,
                           const WidgetParameters *widget,
                           int x, int y, int width, int height,
                           int menubarstyle)
{
	const MurrineRGB *fill = &colors->bg[0];

	cairo_translate (cr, x, y);
	cairo_rectangle (cr, 0, 0, width, height);

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	/* Glass menubar */
	if (menubarstyle == 1)
	{
		/* XXX: should use another gradient rgba_opacity */
		MurrineRGB highlight;
		murrine_set_gradient (cr, fill, widget->mrn_gradient, 0, 0, 0, height, widget->mrn_gradient.gradients, FALSE);
		murrine_shade (fill, widget->highlight_ratio, &highlight);

		if (widget->glazestyle > 0)
		{
			widget->glazestyle == 2 ? cairo_fill_preserve (cr) : cairo_fill (cr);
			if (widget->glazestyle < 3)
				murrine_draw_curved_highlight (cr, 0, width, height);
			else
				murrine_draw_curved_highlight_top (cr, 0, width, height);
		}
		else
		{
			cairo_fill (cr);
			murrine_draw_flat_highlight (cr, 0, 0, width, height);
		}

		murrine_set_gradient (cr, &highlight, widget->mrn_gradient, 0, 0, 0, height, widget->mrn_gradient.gradients, TRUE);
		cairo_fill (cr);

		if (widget->glazestyle == 4)
		{
			murrine_draw_curved_highlight_bottom (cr, 0, width, height);
			MurrineRGB shadow;
			murrine_shade (fill, 1.0/widget->highlight_ratio, &shadow);
			murrine_set_color_rgb (cr, &shadow);
			cairo_fill (cr);
		}

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

		if (widget->glazestyle == 2)
		{
			murrine_draw_innerborder (cr, &highlight, fill, widget->mrn_gradient,
			                          1.5, 1.5,
			                          width-3, height-3,
			                          widget->mrn_gradient.gradients, TRUE,
			                          widget->glazestyle, widget->roundness, widget->corners);
		}
	}
	else if (menubarstyle == 2) /* Gradient menubar */
	{
		cairo_pattern_t *pattern;
		MurrineRGB lower;
		murrine_shade (fill, 0.95, &lower);
		pattern = cairo_pattern_create_linear (0, 0, 0, height);
		cairo_pattern_add_color_stop_rgba (pattern, 0.0, fill->r, fill->g, fill->b, MENUBAR_OPACITY);
		cairo_pattern_add_color_stop_rgba (pattern, 1.0, lower.r, lower.g, lower.b, MENUBAR_OPACITY);
		cairo_set_source (cr, pattern);
		cairo_fill (cr);
		cairo_pattern_destroy (pattern);

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	}
	else if (menubarstyle == 3) /* Striped menubar */
	{
		cairo_pattern_t *pattern;
		MurrineRGB low, top;
		murrine_shade (fill, 0.9, &top);
		murrine_shade (fill, 1.1, &low);
		pattern = cairo_pattern_create_linear (0, 0, 0, height);
		cairo_pattern_add_color_stop_rgba (pattern, 0.0, top.r, top.g, top.b, MENUBAR_STRIPED_OPACITY);
		cairo_pattern_add_color_stop_rgba (pattern, 1.0, low.r, low.g, low.b, MENUBAR_STRIPED_OPACITY);
		cairo_set_source (cr, pattern);
		cairo_fill (cr);

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

		cairo_pattern_destroy (pattern);
		int counter = -height;
		cairo_set_line_width  (cr, 1.0);
		murrine_shade (&low, 0.9, &low);
		murrine_set_color_rgba (cr, &low, MENUBAR_STRIPED_OPACITY);
		while (counter < width)
		{
			cairo_move_to (cr, counter, height);
			cairo_line_to (cr, counter+height, 0);
			cairo_stroke  (cr);
			counter += 5;
		}
	}
	else /* Flat menubar */
	{
		murrine_set_color_rgba (cr, fill, MENUBAR_OPACITY);
		cairo_fill (cr);

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	}

	/* Draw bottom line */
	if (menubarstyle == 1 && widget->glazestyle == 2)
		cairo_rectangle (cr, 0.5, 0.5, width-1, height-1);
#ifndef HAVE_MACMENU
	else
	{
		cairo_set_line_width (cr, 1.0);
		cairo_move_to        (cr, 0, height-0.5);
		cairo_line_to        (cr, width, height-0.5);
	}

	murrine_set_color_rgb (cr, &colors->shade[4]);
	cairo_stroke          (cr);
#endif
}

static void
murrine_rgba_draw_toolbar (cairo_t *cr,
                           const MurrineColors    *colors,
                           const WidgetParameters *widget,
                           const ToolbarParameters *toolbar,
                           int x, int y, int width, int height)
{
	const MurrineRGB *dark = &colors->shade[4];
	const MurrineRGB *fill = &colors->bg[0];
	const MurrineRGB *top  = &colors->shade[0];

	cairo_translate (cr, x, y);
	cairo_rectangle (cr, 0, 0, width, height);

	cairo_set_line_width (cr, 1.0);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

#ifdef HAVE_MACMENU
	murrine_set_color_rgba (cr, fill, WINDOW_OPACITY);
	cairo_fill (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
#else
	/* Glass toolbar */
	if (toolbar->style == 1)
	{
		/* XXX: should use another gradient rgba_opacity */
		MurrineRGB highlight;
		murrine_set_gradient (cr, fill, widget->mrn_gradient, 0, 0, 0, height, widget->mrn_gradient.gradients, FALSE);
		murrine_shade (fill, widget->highlight_ratio, &highlight);

		/* Glass effect */
		if (widget->glazestyle > 0)
		{
			widget->glazestyle == 2 ? cairo_fill_preserve (cr) : cairo_fill (cr);
			if (widget->glazestyle < 3)
				murrine_draw_curved_highlight (cr, 0, width, height);
			else
				murrine_draw_curved_highlight_top (cr, 0, width, height);
		}
		else
		{
			cairo_fill (cr);
			murrine_draw_flat_highlight (cr, 0, 0, width, height);
		}

		murrine_set_gradient (cr, &highlight, widget->mrn_gradient, 0, 0, 0, height, widget->mrn_gradient.gradients, TRUE);
		cairo_fill (cr);

		if (widget->glazestyle == 4)
		{
			murrine_draw_curved_highlight_bottom (cr, 0, width, height);
			MurrineRGB shadow;
			murrine_shade (fill, 1.0/widget->highlight_ratio, &shadow);
			murrine_set_color_rgb (cr, &shadow);
			cairo_fill (cr);
		}

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	}
	else if (toolbar->style == 2)
	{
		cairo_pattern_t *pattern;
		MurrineRGB lower;
		murrine_shade (fill, 0.95, &lower);
		pattern = cairo_pattern_create_linear (0, 0, 0, height);
		cairo_pattern_add_color_stop_rgba (pattern, 0.0, fill->r, fill->g, fill->b, TOOLBAR_OPACITY);
		cairo_pattern_add_color_stop_rgba (pattern, 1.0, lower.r, lower.g, lower.b, TOOLBAR_OPACITY);
		cairo_set_source (cr, pattern);
		cairo_fill (cr);
		cairo_pattern_destroy (pattern);

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	}
	else /* Flat toolbar */
	{
		murrine_set_color_rgba (cr, fill, TOOLBAR_OPACITY);
		cairo_fill (cr);

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

		/* Draw highlight */
		if (!toolbar->topmost)
		{
			cairo_move_to          (cr, 0, 0.5);
			cairo_line_to          (cr, width, 0.5);
			murrine_set_color_rgba (cr, top, 0.5);
			cairo_stroke           (cr);
		}
	}
#endif

	/* Draw shadow */
	cairo_move_to         (cr, 0, height-0.5);
	cairo_line_to         (cr, width, height-0.5);
	murrine_set_color_rgb (cr, dark);
	cairo_stroke          (cr);
}

static void
murrine_get_frame_gap_clip (int x, int y, int width, int height,
                            const FrameParameters *frame,
                            MurrineRectangle      *bevel,
                            MurrineRectangle      *border)
{
	if (frame->gap_side == MRN_GAP_TOP)
	{
		MURRINE_RECTANGLE_SET ((*bevel),  1.5 + frame->gap_x,  -0.5,
		                       frame->gap_width - 3, 2.0);
		MURRINE_RECTANGLE_SET ((*border), 0.5 + frame->gap_x,  -0.5,
		                       frame->gap_width - 2, 2.0);
	}
	else if (frame->gap_side == MRN_GAP_BOTTOM)
	{
		MURRINE_RECTANGLE_SET ((*bevel),  1.5 + frame->gap_x,  height - 2.5,
		                       frame->gap_width - 3, 2.0);
		MURRINE_RECTANGLE_SET ((*border), 0.5 + frame->gap_x,  height - 1.5,
		                       frame->gap_width - 2, 2.0);
	}
	else if (frame->gap_side == MRN_GAP_LEFT)
	{
		MURRINE_RECTANGLE_SET ((*bevel),  -0.5, 1.5 + frame->gap_x,
		                       2.0, frame->gap_width - 3);
		MURRINE_RECTANGLE_SET ((*border), -0.5, 0.5 + frame->gap_x,
		                       1.0, frame->gap_width - 2);
	}
	else if (frame->gap_side == MRN_GAP_RIGHT)
	{
		MURRINE_RECTANGLE_SET ((*bevel),  width - 2.5, 1.5 + frame->gap_x,
		                       2.0, frame->gap_width - 3);
		MURRINE_RECTANGLE_SET ((*border), width - 1.5, 0.5 + frame->gap_x,
		                       1.0, frame->gap_width - 2);
	}
}

static void
murrine_rgba_draw_frame (cairo_t *cr,
                         const MurrineColors    *colors,
                         const WidgetParameters *widget,
                         const FrameParameters  *frame,
                         int x, int y, int width, int height)
{
	MurrineRGB *border = frame->border;
	MurrineRectangle bevel_clip;
	MurrineRectangle frame_clip;

	const MurrineRGB *dark = &colors->shade[5];

	MurrineRGB highlight, shadow_color;

	murrine_shade (&colors->bg[0], 1.15, &highlight);
	murrine_shade (&colors->bg[0], 0.4, &shadow_color);

	if (frame->shadow == MRN_SHADOW_NONE)
		return;

	if (frame->gap_x != -1)
		murrine_get_frame_gap_clip (x, y, width, height,
		                            frame, &bevel_clip, &frame_clip);

	cairo_set_line_width (cr, 1.0);
	cairo_translate      (cr, x+0.5, y+0.5);

	/* save everything */
	cairo_save (cr);

	/* Set clip for the bevel */
	if (frame->gap_x != -1)
	{
		/* Set clip for gap */
		cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
		cairo_rectangle     (cr, -0.5, -0.5, width, height);
		cairo_rectangle     (cr, bevel_clip.x, bevel_clip.y, bevel_clip.width, bevel_clip.height);
		cairo_clip          (cr);
	}

	/* Draw the bevel */
	if (frame->shadow == MRN_SHADOW_ETCHED_IN || frame->shadow == MRN_SHADOW_ETCHED_OUT)
	{
		if (frame->shadow == MRN_SHADOW_ETCHED_IN)
			murrine_rounded_rectangle (cr, 1, 1, width-2, height-2, widget->roundness, widget->corners);
		else
			murrine_rounded_rectangle (cr, 0, 0, width-2, height-2, widget->roundness, widget->corners);
		murrine_set_color_rgba (cr, &highlight, 0.5);
		cairo_stroke (cr);
	}
	else if (frame->shadow != MRN_SHADOW_NONE && frame->shadow != MRN_SHADOW_FLAT)
	{
		ShadowParameters shadow;
		shadow.corners = widget->corners;
		shadow.shadow  = frame->shadow;
		cairo_move_to (cr, 1, height-2);
		cairo_line_to (cr, 1, 1);
		cairo_line_to (cr, width-1.5, 1);
		if (frame->shadow & MRN_SHADOW_OUT)
			murrine_set_color_rgba (cr, &highlight, 0.5);
		else
			murrine_set_color_rgba (cr, &shadow_color, 0.13);
		cairo_stroke (cr);
		cairo_move_to (cr, width-2, 1.5);
		cairo_line_to (cr, width-2, height-2);
		cairo_line_to (cr, 0, height-2);
		if (frame->shadow & MRN_SHADOW_OUT)
			murrine_set_color_rgba (cr, &shadow_color, 0.13);
		else
			murrine_set_color_rgba (cr, &highlight, 0.5);
		cairo_stroke (cr);
	}

	/* restore the previous clip region */
	cairo_restore (cr);
	cairo_save    (cr);
	if (frame->gap_x != -1)
	{
		/* Set clip for gap */
		cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
		cairo_rectangle     (cr, -0.5, -0.5, width, height);
		cairo_rectangle     (cr, frame_clip.x, frame_clip.y, frame_clip.width, frame_clip.height);
		cairo_clip          (cr);
	}

	/* Draw frame */
	if (frame->shadow == MRN_SHADOW_ETCHED_IN || frame->shadow == MRN_SHADOW_ETCHED_OUT)
	{
		murrine_set_color_rgb (cr, dark);
		if (frame->shadow == MRN_SHADOW_ETCHED_IN)
			murrine_rounded_rectangle (cr, 0, 0, width-2, height-2, widget->roundness, widget->corners);
		else
			murrine_rounded_rectangle (cr, 1, 1, width-2, height-2, widget->roundness, widget->corners);
	}
	else
	{
		murrine_set_color_rgb (cr, border);
		murrine_rounded_rectangle (cr, 0, 0, width-1, height-1, widget->roundness, widget->corners);
	}
	cairo_stroke  (cr);
	cairo_restore (cr);
}

static void
murrine_rgba_draw_separator (cairo_t *cr,
                             const MurrineColors       *colors,
                             const WidgetParameters    *widget,
                             const SeparatorParameters *separator,
                             int x, int y, int width, int height)
{
	const MurrineRGB *dark      = &colors->shade[4];
	const MurrineRGB *highlight = &colors->shade[0];

	if (separator->horizontal)
	{
		cairo_set_line_width  (cr, 1.0);
		cairo_translate       (cr, x, y+0.5);

		cairo_move_to         (cr, 0.0,     0.0);
		cairo_line_to         (cr, width+1, 0.0);
		murrine_set_color_rgb (cr, dark);
		cairo_stroke          (cr);

#ifndef HAVE_MACMENU
		cairo_move_to          (cr, 0.0,   1.0);
		cairo_line_to          (cr, width, 1.0);
		murrine_set_color_rgba (cr, highlight, 0.5);
		cairo_stroke           (cr);
#endif
	}
	else
	{
		cairo_set_line_width  (cr, 1.0);
		cairo_translate       (cr, x+0.5, y);

		cairo_move_to         (cr, 0.0, 0.0);
		cairo_line_to         (cr, 0.0, height);
		murrine_set_color_rgb (cr, dark);
		cairo_stroke          (cr);

#ifndef HAVE_MACMENU
		cairo_move_to          (cr, 1.0, 0.0);
		cairo_line_to          (cr, 1.0, height);
		murrine_set_color_rgba (cr, highlight, 0.5);
		cairo_stroke           (cr);
#endif
	}
}

static void
murrine_rgba_draw_tab (cairo_t *cr,
                       const MurrineColors    *colors,
                       const WidgetParameters *widget,
                       const TabParameters    *tab,
                       int x, int y, int width, int height)
{
	const float      RADIUS = 3.0;
	int              corners;
	double           strip_size;
	const MurrineRGB *stripe_fill = &colors->spot[1];
	const MurrineRGB *stripe_border = &colors->spot[2];
	const MurrineRGB *fill;
	MurrineRGB       *border;
	cairo_pattern_t* pattern;

	fill = &colors->bg[widget->state_type];

	if (!widget->active)
		border = (MurrineRGB*)&colors->shade[5];
	else
		border = (MurrineRGB*)&colors->shade[4];

	/* Set clip */
	cairo_rectangle (cr, x, y, width, height);
	cairo_clip      (cr);
	cairo_new_path  (cr);

	/* Translate and set line width */
	cairo_set_line_width (cr, 1.0);
	cairo_translate      (cr, x+0.5, y+0.5);

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	/* Make the tabs slightly bigger than they should be, to create a gap */
	/* And calculate the strip size too, while you're at it */
	if (tab->gap_side == MRN_GAP_TOP || tab->gap_side == MRN_GAP_BOTTOM)
	{
		height += RADIUS;
		strip_size = (tab->gap_side == MRN_GAP_TOP ? 2.0/height : 2.0/(height-2));

		if (tab->gap_side == MRN_GAP_TOP)
		{
			cairo_translate (cr, 0.0, -3.0); /* gap at the other side */
			corners = MRN_CORNER_BOTTOMLEFT | MRN_CORNER_BOTTOMRIGHT;
		}
		else
			corners = MRN_CORNER_TOPLEFT | MRN_CORNER_TOPRIGHT;
	}
	else
	{
		width += RADIUS;
		strip_size = (tab->gap_side == MRN_GAP_LEFT ? 2.0/width : 2.0/(width-2));

		if (tab->gap_side == MRN_GAP_LEFT)
		{
			cairo_translate (cr, -3.0, 0.0); /* gap at the other side */
			corners = MRN_CORNER_TOPRIGHT | MRN_CORNER_BOTTOMRIGHT;
		}
		else
			corners = MRN_CORNER_TOPLEFT | MRN_CORNER_BOTTOMLEFT;
	}

/*	murrine_set_color_rgba (cr, &colors->bg[0], 0.0); */
/*	/* Draw (erase) the background */
/*	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (cr); */

	/* Set tab shape */
	if (widget->roundness < 2)
		cairo_rectangle (cr, 0, 0, width-1, height-1);
	else
		clearlooks_rounded_rectangle (cr, 0, 0, width-1, height-1, widget->roundness, corners);

	/* Draw fill */
	if (!widget->active)
	{
		murrine_set_color_rgba (cr, fill, NOTEBOOK_OPACITY);
		cairo_fill (cr);
	}

	/* Draw highlight */
/*	if (!widget->active)
	{
		ShadowParameters shadow;

		shadow.shadow  = MRN_SHADOW_OUT;
		shadow.corners = widget->corners;

		murrine_draw_highlight_and_shade (cr, colors, &shadow,
		                                  width,
		                                  height, widget->roundness-1);
	}
*/
	if (widget->active)
	{
		MurrineRGB shade1, shade2, shade3, shade4;

		MurrineGradients mrn_gradient_custom = widget->mrn_gradient;
		mrn_gradient_custom.gradient_stop_1 = get_decreased_ratio (widget->mrn_gradient.gradient_stop_1, 3.0);
		mrn_gradient_custom.gradient_stop_2 = get_decreased_ratio (widget->mrn_gradient.gradient_stop_2, 3.0);
		mrn_gradient_custom.gradient_stop_3 = get_decreased_ratio (widget->mrn_gradient.gradient_stop_3, 3.0);
		mrn_gradient_custom.gradient_stop_4 = get_decreased_ratio (widget->mrn_gradient.gradient_stop_4, 3.0);

		double custom_highlight_ratio = widget->highlight_ratio;
		custom_highlight_ratio = get_decreased_ratio (widget->highlight_ratio, 2.0);

		murrine_shade (fill, mrn_gradient_custom.gradient_stop_1*custom_highlight_ratio, &shade1);
		murrine_shade (fill, mrn_gradient_custom.gradient_stop_2*custom_highlight_ratio, &shade2);
		murrine_shade (fill, mrn_gradient_custom.gradient_stop_3, &shade3);
		murrine_shade (fill, mrn_gradient_custom.gradient_stop_4, &shade4);

		switch (tab->gap_side)
		{
			case MRN_GAP_TOP:
				pattern = cairo_pattern_create_linear (0, height-2, 0, 0);
				break;
			case MRN_GAP_BOTTOM:
				pattern = cairo_pattern_create_linear (0, 1, 0, height);
				break;
			case MRN_GAP_LEFT:
				pattern = cairo_pattern_create_linear (width-2, 0, 1, 0);
				break;
			case MRN_GAP_RIGHT:
				pattern = cairo_pattern_create_linear (1, 0, width-2, 0);
				break;
		}

		if (widget->roundness < 2)
			cairo_rectangle (cr, 0, 0, width-1, height-1);
		else
			clearlooks_rounded_rectangle (cr, 0, 0, width-1, height-1, widget->roundness, corners);

		cairo_pattern_add_color_stop_rgba (pattern, 0.0, shade1.r, shade1.g, shade1.b, NOTEBOOK_OPACITY);
		cairo_pattern_add_color_stop_rgba (pattern, 0.4, shade2.r, shade2.g, shade2.b, NOTEBOOK_OPACITY);
		cairo_pattern_add_color_stop_rgba (pattern, 0.4, shade3.r, shade3.g, shade3.b, NOTEBOOK_OPACITY);
		cairo_pattern_add_color_stop_rgba (pattern, 1.0, shade4.r, shade4.g, shade4.b, NOTEBOOK_OPACITY);
		cairo_set_source (cr, pattern);
		cairo_fill (cr);
		cairo_pattern_destroy (pattern);

		cairo_set_line_width (cr, 1.0);
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		murrine_set_color_rgba (cr, &colors->shade[0], 0.2);

		if (widget->roundness < 2)
			cairo_rectangle (cr, 1, 1, width-3, height-3);
		else
			clearlooks_rounded_rectangle (cr, 1, 1, width-3, height-3, widget->roundness, corners);

		cairo_stroke (cr);
	}
	else
	{
		/* Draw shade */
		switch (tab->gap_side)
		{
			case MRN_GAP_TOP:
				pattern = cairo_pattern_create_linear (0, height-2, 0, 0);
				break;
			case MRN_GAP_BOTTOM:
				pattern = cairo_pattern_create_linear (0, 0, 0, height);
				break;
			case MRN_GAP_LEFT:
				pattern = cairo_pattern_create_linear (width-2, 0, 0, 0);
				break;
			case MRN_GAP_RIGHT:
				pattern = cairo_pattern_create_linear (0, 0, width, 0);
				break;
		}

		if (widget->roundness < 2)
			cairo_rectangle (cr, 0, 0, width-1, height-1);
		else
			clearlooks_rounded_rectangle (cr, 0, 0, width-1, height-1, widget->roundness, corners);

		cairo_pattern_add_color_stop_rgba (pattern, 0.0,        stripe_fill->r, stripe_fill->g, stripe_fill->b, NOTEBOOK_OPACITY);
		cairo_pattern_add_color_stop_rgba (pattern, strip_size, stripe_fill->r, stripe_fill->g, stripe_fill->b, NOTEBOOK_OPACITY);
		cairo_pattern_add_color_stop_rgba (pattern, strip_size, fill->r, fill->g, fill->b, NOTEBOOK_OPACITY);
		cairo_set_source (cr, pattern);
		cairo_fill (cr);
		cairo_pattern_destroy (pattern);
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	}

	murrine_rounded_rectangle (cr, 0, 0, width-1, height-1, widget->roundness, corners);

	if (widget->active)
	{
		murrine_set_color_rgb  (cr, border);
		cairo_stroke (cr);
	}
	else
	{
		switch (tab->gap_side)
		{
			case MRN_GAP_TOP:
				pattern = cairo_pattern_create_linear (2, height-2, 2, 2);
				break;
			case MRN_GAP_BOTTOM:
				pattern = cairo_pattern_create_linear (2, 2, 2, height);
				break;
			case MRN_GAP_LEFT:
				pattern = cairo_pattern_create_linear (width-2, 2, 2, 2);
				break;
			case MRN_GAP_RIGHT:
				pattern = cairo_pattern_create_linear (2, 2, width, 2);
				break;
		}

		cairo_pattern_add_color_stop_rgb (pattern, 0.0,        stripe_border->r, stripe_border->g, stripe_border->b);
		cairo_pattern_add_color_stop_rgb (pattern, strip_size, stripe_border->r, stripe_border->g, stripe_border->b);
		cairo_pattern_add_color_stop_rgb (pattern, strip_size, stripe_border->r, stripe_border->g, stripe_border->b);
		cairo_pattern_add_color_stop_rgb (pattern, 0.8,        border->r,        border->g,        border->b);
		cairo_set_source (cr, pattern);
		cairo_stroke (cr);
		cairo_pattern_destroy (pattern);
	}
}

static void
murrine_rgba_draw_scrollbar_trough (cairo_t *cr,
                                    const MurrineColors       *colors,
                                    const WidgetParameters    *widget,
                                    const ScrollBarParameters *scrollbar,
                                    int x, int y, int width, int height)
{
	const MurrineRGB *bg     = &colors->shade[scrollbar->stepperstyle < 1 ? 1 : 0];
	const MurrineRGB *border = &colors->shade[scrollbar->stepperstyle < 1 ? 4 : 5];

	cairo_set_line_width (cr, 1.0);

	if (scrollbar->horizontal)
	{
		int tmp = height;
		rotate_mirror_translate (cr, M_PI/2, x, y, FALSE, FALSE);
		height = width;
		width = tmp;
	}
	else
	{
		cairo_translate (cr, x, y);
	}

	/* Draw fill */
	clearlooks_rounded_rectangle (cr, 1, 0, width-2, height, widget->roundness, widget->corners);
	murrine_set_color_rgba (cr, bg, 0.4);
	cairo_fill (cr);

	/* Draw border */
	murrine_rounded_rectangle (cr, 0.5, 0.5, width-1, height-1, widget->roundness, widget->corners);
	murrine_set_color_rgba (cr, border, 0.82);
	cairo_stroke (cr);
}

void
murrine_rgba_draw_scrollbar_stepper (cairo_t *cr,
                                     const MurrineColors       *colors,
                                     const WidgetParameters    *widget,
                                     const ScrollBarParameters *scrollbar,
                                     int x, int y, int width, int height)
{
	const MurrineRGB *fill  = &colors->bg[widget->state_type];
	MurrineRGB border_normal;
	MurrineRGB highlight;

	murrine_shade (&colors->shade[7], 0.95, &border_normal);
	murrine_shade (fill, widget->highlight_ratio, &highlight);

	if (!scrollbar->horizontal)
		murrine_exchange_axis (cr, &x, &y, &width, &height);

	/* Border color */
	murrine_mix_color (&border_normal, fill, 0.45, &border_normal);

	cairo_translate (cr, x, y);
	cairo_set_line_width (cr, 1.0);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	/* Draw the bg */
	if (widget->roundness < 2)
		cairo_rectangle (cr, 1, 1, width-2, height-2);
	else
		clearlooks_rounded_rectangle (cr, 0.5, 0.5, width-1, height-1, widget->roundness+1, widget->corners);
	murrine_set_gradient (cr, fill, widget->mrn_gradient, 1, 1, 0, height-2, widget->mrn_gradient.gradients, FALSE);

	cairo_save (cr);

	int curve_pos = 1;
	if (widget->glazestyle != 4)
		curve_pos = 2;

	/* Draw the glass effect */
	if (widget->glazestyle > 0)
	{
		cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
		widget->glazestyle == 2 ? cairo_fill_preserve (cr) : cairo_fill (cr);
		if (widget->glazestyle < 3)
			murrine_draw_curved_highlight (cr, curve_pos, width, height);
		else
			murrine_draw_curved_highlight_top (cr, curve_pos, width, height);
	}
	else
	{
		cairo_fill (cr);
		murrine_draw_flat_highlight (cr, 1, 1, width-2, height-2);
	}

	murrine_set_gradient (cr, &highlight, widget->mrn_gradient, 1, 1, 0, height-2, widget->mrn_gradient.gradients, TRUE);
	cairo_fill (cr);

	if (widget->glazestyle == 4)
	{
		murrine_draw_curved_highlight_bottom (cr, curve_pos, width, height);
		MurrineRGB shadow;
		murrine_shade (fill, 1.0/widget->highlight_ratio, &shadow);
		murrine_set_gradient (cr, &shadow, widget->mrn_gradient, 1, 1, 0, height-2, widget->mrn_gradient.gradients, TRUE);
		cairo_fill (cr);
	}

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	/* Draw the white inner border */
	if (widget->glazestyle != 4)
	{
		murrine_shade (fill, widget->innerborder_ratio*widget->highlight_ratio, &highlight);
		murrine_draw_innerborder (cr, &highlight, fill, widget->mrn_gradient,
		                              1.5, 1.5,
		                              width-3, height-3,
		                              widget->mrn_gradient.gradients, TRUE,
		                              widget->glazestyle, widget->roundness, widget->corners);
	}

	cairo_reset_clip (cr);
	cairo_restore (cr);

	murrine_set_color_rgb (cr, &border_normal);
	/* Draw the border */
	murrine_rounded_rectangle (cr, 0.5, 0.5, width-1, height-1, widget->roundness, widget->corners);
	cairo_stroke (cr);
}

void
murrine_rgba_draw_scrollbar_slider (cairo_t *cr,
                                    const MurrineColors       *colors,
                                    const WidgetParameters    *widget,
                                    const ScrollBarParameters *scrollbar,
                                    int x, int y, int width, int height)
{
	if (scrollbar->stepperstyle < 1)
	{
		if (scrollbar->junction & MRN_JUNCTION_BEGIN)
		{
			if (scrollbar->horizontal)
			{
				x -= 1;
				width += 1;
			}
			else
			{
				y -= 1;
				height += 1;
			}
		}
		if (scrollbar->junction & MRN_JUNCTION_END)
		{
			if (scrollbar->horizontal)
				width += 1;
			else
				height += 1;
		}
	}

	/* Set colors */
	MurrineRGB fill;
	if (scrollbar->has_color)
		fill = scrollbar->color;
	else
		fill = colors->bg[0];

	MurrineRGB border;
	murrine_shade (&colors->shade[7], 0.95, &border);
	MurrineRGB highlight;

	if (widget->prelight)
		murrine_shade (&fill, 1.06, &fill);

	murrine_shade (&fill, widget->highlight_ratio, &highlight);
	/* Draw the border */
	murrine_mix_color (&border, &fill, 0.5, &border);

	if (scrollbar->horizontal)
		cairo_translate (cr, x, y);

	else
	{
		int tmp = height;
		rotate_mirror_translate (cr, M_PI/2, x, y, FALSE, FALSE);
		height = width;
		width = tmp;
	}

	cairo_set_line_width (cr, 1.0);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	murrine_rounded_rectangle_fast (cr, 0.5, 0.5, width-1, height-1, widget->corners);
	murrine_set_color_rgb (cr, &border);
	cairo_stroke (cr);

	cairo_rectangle (cr, 1, 1, width-2, height-2);
	murrine_set_gradient (cr, &fill, widget->mrn_gradient, 1, 1, 0, height-2, widget->mrn_gradient.gradients, FALSE);

	/* Draw the glass effect */
	if (widget->glazestyle > 0)
	{
		widget->glazestyle == 2 ? cairo_fill_preserve (cr) : cairo_fill (cr);
		if (widget->glazestyle < 3)
			murrine_draw_curved_highlight (cr, 1, width, height);
		else
			murrine_draw_curved_highlight_top (cr, 1, width, height);
	}
	else
	{
		cairo_fill (cr);
		murrine_draw_flat_highlight (cr, 1, 1, width-2, height-2);
	}

	murrine_set_gradient (cr, &highlight, widget->mrn_gradient, 1, 1, 0, height-2, widget->mrn_gradient.gradients, TRUE);
	cairo_fill (cr);

	if (widget->glazestyle == 4)
	{
		murrine_draw_curved_highlight_bottom (cr, 1, width, height);
		MurrineRGB shadow;
		murrine_shade (&fill, 1.0/widget->highlight_ratio, &shadow);
		murrine_set_color_rgb (cr, &shadow);
		cairo_fill (cr);
	}

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	if (widget->glazestyle != 4)
	{
		murrine_shade (&fill, widget->innerborder_ratio*widget->highlight_ratio, &highlight);
		murrine_draw_innerborder (cr, &highlight, &fill, widget->mrn_gradient,
		                          1.5, 1.5,
		                          width-3, height-3,
		                          widget->mrn_gradient.gradients, TRUE,
		                          widget->glazestyle, 0, MRN_CORNER_NONE);
	}

	/* Draw the options */
	MurrineRGB style;
	if (scrollbar->style > 0)
		murrine_shade (&fill, 0.55, &style);

	/* Draw the circles */
	if (scrollbar->style == 1)
	{
		int circ_radius = 2;
		int circ_space = 5;
		int i;
		int x1 = circ_space+circ_radius;
		int y1 = height/2;
		for (i = circ_space; i < width-circ_space; i += 2*circ_radius+circ_space)
		{
			cairo_move_to (cr, i, 1);
			cairo_arc (cr, x1, y1, circ_radius, 0, M_PI*2);

			x1 += 2*circ_radius+circ_space;

			cairo_close_path (cr);
			murrine_set_color_rgba (cr, &style, 0.15);
			cairo_fill (cr);
		}
	}
	if (scrollbar->style > 2)
	{
		/* Draw the diagonal strokes */
		if (scrollbar->style < 5)
		{
			cairo_save (cr);
			cairo_rectangle (cr, 1, 1, width-2, height-2);
			cairo_clip (cr);
			cairo_new_path (cr);
			int counter = -width;
			cairo_set_line_width (cr, 5); /* stroke width */
			murrine_set_color_rgba (cr, &style, 0.08);
			while (counter < height)
			{
				cairo_move_to (cr, width, counter);
				cairo_line_to (cr, 0, counter+width);
				cairo_stroke  (cr);
				counter += 12;
			}
			cairo_restore (cr);
		}
		/* Draw the horizontal strokes */
		if (scrollbar->style > 4)
		{
			int stroke_width = 7;
			int stroke_space = 5;
			int i;
			murrine_set_color_rgba (cr, &style, 0.08);
			for (i = stroke_space; i < width-stroke_space; i += stroke_width+stroke_space)
			{
				cairo_move_to (cr, i, 1);
				cairo_rel_line_to (cr, 0, height-2);
				cairo_rel_line_to (cr, stroke_width, 0);
				cairo_rel_line_to (cr, 0, -(height-2));
				cairo_fill (cr);
			}
		}
	}
	/* Draw the handle */
	if (scrollbar->style > 0 && scrollbar->style % 2 == 0 )
	{
		int bar_x = width/2 - 4;
		cairo_translate (cr, 0.5, 0.5);
		int i;
		for (i=0; i<3; i++)
		{
			cairo_move_to (cr, bar_x, 4.5);
			cairo_line_to (cr, bar_x, height-5.5);
			murrine_set_color_rgb (cr, &border);
			cairo_stroke (cr);

			bar_x += 3;
		}
	}
}

static void
murrine_rgba_draw_tooltip (cairo_t *cr,
                           const MurrineColors    *colors,
                           const WidgetParameters *widget,
                           int x, int y, int width, int height)
{
	MurrineRGB border, highlight;

	MurrineGradients mrn_gradient_custom = widget->mrn_gradient;

	mrn_gradient_custom.gradient_stop_1 = get_decreased_ratio (widget->mrn_gradient.gradient_stop_1, 2.0);
	mrn_gradient_custom.gradient_stop_2 = get_decreased_ratio (widget->mrn_gradient.gradient_stop_2, 2.0);
	mrn_gradient_custom.gradient_stop_3 = get_decreased_ratio (widget->mrn_gradient.gradient_stop_3, 2.0);
	mrn_gradient_custom.gradient_stop_4 = get_decreased_ratio (widget->mrn_gradient.gradient_stop_4, 2.0);

	murrine_shade (&colors->bg[widget->state_type], 0.6, &border);
	murrine_shade (&colors->bg[widget->state_type], 1.0, &highlight);

	cairo_save (cr);

	cairo_translate (cr, x, y);
	cairo_set_line_width (cr, 1.0);
	cairo_rectangle (cr, 0, 0, width, height);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_fill (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	murrine_set_gradient (cr, &colors->bg[widget->state_type], mrn_gradient_custom, 0, 0, 0, height, widget->mrn_gradient.gradients, FALSE);
	clearlooks_rounded_rectangle (cr, 0, 0, width, height, widget->roundness > 1 ? widget->roundness+1 : 0, widget->corners);
	cairo_fill (cr);

	murrine_set_gradient (cr, &highlight, mrn_gradient_custom, 0, 0, 0, height, widget->mrn_gradient.gradients, TRUE);
	clearlooks_rounded_rectangle (cr, 0, 0, width, height/2, widget->roundness > 1 ? widget->roundness+1 : 0, 
	                              widget->corners == MRN_CORNER_ALL ? MRN_CORNER_TOPLEFT | MRN_CORNER_TOPRIGHT : MRN_CORNER_NONE);
	cairo_fill (cr);

	murrine_set_color_rgb (cr, &border);
	murrine_rounded_rectangle (cr, 0.5, 0.5, width-1, height-1, widget->roundness, widget->corners);
	cairo_stroke (cr);

	cairo_restore (cr);
}

static void
murrine_rgba_draw_handle (cairo_t *cr,
                          const MurrineColors    *colors,
                          const WidgetParameters *widget,
                          const HandleParameters *handle,
                          int x, int y, int width, int height)
{
	const MurrineRGB *dark  = &colors->shade[5];

	int bar_height;
	int bar_width  = 4;
	int i, bar_y = 1;
	int num_bars, bar_spacing;
	num_bars    = 3;
	bar_spacing = 3;
	bar_height = num_bars * bar_spacing;

	if (handle->horizontal)
	{
		int tmp = height;
		rotate_mirror_translate (cr, M_PI/2, x + 0.5 + width/2 - bar_height/2, y + height/2 - bar_width/2, FALSE, FALSE);
		height = width;
		width = tmp;
	}
	else
	{
		cairo_translate (cr, x + width/2 - bar_width/2, y + height/2 - bar_height/2 + 0.5);
	}

	cairo_set_line_width (cr, 1.0);

	for (i=0; i<num_bars; i++)
	{
		cairo_move_to (cr, 0, bar_y);
		cairo_line_to (cr, bar_width, bar_y);
		murrine_set_color_rgb (cr, dark);
		cairo_stroke (cr);

		bar_y += bar_spacing;
	}
}


static void
murrine_rgba_draw_radiobutton (cairo_t * cr,
                               const MurrineColors    *colors,
                               const WidgetParameters *widget,
                               const OptionParameters *status,
                               int x, int y, int width, int height,
                               double trans)
{
	MurrineRGB border;
	const MurrineRGB *dot;

	width = height = 15;

	if (widget->state_type == GTK_STATE_INSENSITIVE)
	{
		border = colors->shade[4];
		dot    = &colors->shade[4];
	}
	else
	{
		border = colors->shade[6];
		if (status->draw_bullet)
			border = colors->spot[2];
		dot    = &colors->text[widget->state_type];
	}
	murrine_mix_color (&border, widget->state_type != GTK_STATE_INSENSITIVE ?
	                   status->draw_bullet ? &colors->spot[1] : &colors->bg[0] : &colors->bg[0], 0.2, &border);

	cairo_translate (cr, x, y);

	cairo_set_line_width (cr, 2);
	cairo_arc (cr, 7, 7, 6, 0, M_PI*2);
	murrine_set_color_rgba (cr, &colors->shade[8], 0.12);
	cairo_stroke (cr);

	cairo_set_line_width (cr, 1.0);

	cairo_arc (cr, 7, 7, 5.5, 0, M_PI*2);

	if (widget->state_type != GTK_STATE_INSENSITIVE)
	{
		const MurrineRGB *bg = &colors->base[0];
		if (status->draw_bullet)
			bg = &colors->spot[1];
		if (widget->glazestyle != 2)
		{
			MurrineRGB highlight;
			murrine_shade (bg, widget->highlight_ratio, &highlight);
			murrine_set_gradient (cr, &highlight, widget->mrn_gradient, 0, 0, 0, 14, widget->mrn_gradient.gradients, FALSE);
		}
		else
			murrine_set_gradient (cr, bg, widget->mrn_gradient, 0, 0, 0, 14, widget->mrn_gradient.gradients, FALSE);
		cairo_fill_preserve (cr);
	}

	murrine_set_color_rgb (cr, &border);
	cairo_stroke (cr);

	cairo_arc (cr, 7, 7, 5, 0, M_PI*2);
	cairo_clip (cr);

	if (widget->state_type != GTK_STATE_INSENSITIVE)
	{
		const MurrineRGB *bg = &colors->base[0];
		if (status->draw_bullet)
			bg = &colors->spot[1];

		cairo_rectangle (cr, 0, 7, width, height);
		if (widget->glazestyle == 2)
		{
			MurrineRGB highlight;
			murrine_shade (bg, widget->highlight_ratio, &highlight);
			murrine_set_gradient (cr, &highlight, widget->mrn_gradient, 0, 0, 0, 14, widget->mrn_gradient.gradients, FALSE);
		}
		else
			murrine_set_gradient (cr, bg, widget->mrn_gradient, 0, 0, 0, 14, widget->mrn_gradient.gradients, FALSE);
		cairo_fill (cr);
	}

	/* inconsistent state is missing? */
	if (status->draw_bullet)
	{
		cairo_arc (cr, 7, 7, 2, 0, M_PI*2);
		/* murrine_set_color_rgb (cr, dot); */
		murrine_set_color_rgba (cr, dot, trans);
		cairo_fill (cr);
	}

	cairo_restore (cr);
}

static void
murrine_rgba_draw_checkbox (cairo_t * cr,
                            const MurrineColors    *colors,
                            const WidgetParameters *widget,
                            const OptionParameters *status,
                            int x, int y, int width, int height,
                            double trans)
{
	MurrineRGB border;
	const MurrineRGB *dot;

	width = height = 13;

	if (widget->state_type == GTK_STATE_INSENSITIVE)
	{
		border = colors->shade[4];
		dot    = &colors->shade[4];
	}
	else
	{
		border = colors->shade[6];
		if (status->draw_bullet)
			border = colors->spot[2];
		dot    = &colors->text[widget->state_type];
	}
	murrine_mix_color (&border, widget->state_type != GTK_STATE_INSENSITIVE ?
	                   status->draw_bullet ? &colors->spot[1] : &colors->bg[0] : &colors->bg[0], 0.24, &border);

	cairo_translate (cr, x, y);
	cairo_set_line_width (cr, 1.0);

	if (widget->xthickness > 2 && widget->ythickness > 2)
	{
		cairo_rectangle (cr, 0.5, 0.5, width-1, height-1);
		murrine_set_color_rgba (cr, &colors->shade[8], 0.12);
		cairo_stroke (cr);

		/* Draw the rectangle for the checkbox itself */
		cairo_rectangle (cr, 1.5, 1.5, width-3, height-3);
	}
	else
	{
		cairo_rectangle (cr, 0.5, 0.5, width-1, height-1);
	}


	if (widget->state_type != GTK_STATE_INSENSITIVE)
	{
		const MurrineRGB *bg = &colors->base[0];
		if (status->draw_bullet)
			bg = &colors->spot[1];

		if (widget->glazestyle == 2)
		{
			MurrineRGB highlight;
			murrine_shade (bg, widget->highlight_ratio, &highlight);
			murrine_set_gradient (cr, &highlight, widget->mrn_gradient, 0, 0, 0, 14, widget->mrn_gradient.gradients, FALSE);
		}
		else
			murrine_set_gradient (cr, bg, widget->mrn_gradient, 0, 0, 0, 14, widget->mrn_gradient.gradients, FALSE);
		cairo_fill_preserve (cr);
	}

	murrine_set_color_rgb (cr, &border);
	cairo_stroke (cr);

	if (widget->state_type != GTK_STATE_INSENSITIVE)
	{
		const MurrineRGB *bg = &colors->base[0];
		if (status->draw_bullet)
			bg = &colors->spot[1];

		MurrineRGB highlight;
		murrine_shade (bg, widget->highlight_ratio, &highlight);
		if (widget->xthickness > 2 && widget->ythickness > 2)
			cairo_rectangle (cr, 2, 2, width-4, (height-4)/2);
		else
			cairo_rectangle (cr, 1, 1, width-2, (height-2)/2);

		if (widget->glazestyle != 2)
		{
			MurrineRGB highlight;
			murrine_shade (bg, widget->highlight_ratio, &highlight);
			murrine_set_gradient (cr, &highlight, widget->mrn_gradient, 0, 0, 0, 14, widget->mrn_gradient.gradients, FALSE);
		}
		else
			murrine_set_gradient (cr, bg, widget->mrn_gradient, 0, 0, 0, 14, widget->mrn_gradient.gradients, FALSE);
		cairo_fill (cr);
	}

	cairo_scale (cr, width / 13.0, height / 13.0);
	if (status->draw_bullet)
	{
		if (status->inconsistent) /* Inconsistent */
		{
			cairo_set_line_width (cr, 2.0);
			cairo_move_to (cr, 3, height*0.5);
			cairo_line_to (cr, width-3, height*0.5);
		}
		else
		{
			cairo_translate (cr, -2, 0);
		/*
			if (widget && widget->parent && GTK_IS_MENU(widget->parent))
				cairo_translate (cr, 0, 0);
		*/
			cairo_move_to (cr, 4, 8);
			cairo_rel_line_to (cr, 5, 4);
			cairo_rel_curve_to (cr, 1.4, -5, -1, -1, 5.7, -12.5);
			cairo_rel_curve_to (cr, -4, 4, -4, 4, -6.7, 9.3);
			cairo_rel_line_to (cr, -2.3, -2.5);
		}

		murrine_set_color_rgba (cr, dot, trans);
		cairo_fill (cr);
	}
}

static void
murrine_rgba_draw_menu_frame (cairo_t *cr,
                              const MurrineColors    *colors,
                              const WidgetParameters *widget,
                              int x, int y, int width, int height,
                              int menustyle)
{
	const MurrineRGB *border = &colors->shade[5];
	uint8 corners = (menustyle == 1 ? MRN_CORNER_TOPRIGHT | MRN_CORNER_BOTTOMRIGHT :
	                                  MRN_CORNER_TOPRIGHT |
	                                  MRN_CORNER_BOTTOMLEFT | MRN_CORNER_BOTTOMRIGHT);

	cairo_translate      (cr, x, y);
	cairo_set_line_width (cr, 1.0);
	cairo_set_operator   (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	clearlooks_rounded_rectangle (cr, 0, 0, width, height, widget->roundness > 1 ? widget->roundness+1 : 0, corners);
	murrine_set_color_rgba (cr, &colors->bg[0], MENU_OPACITY);
	cairo_fill (cr);

	murrine_rounded_rectangle (cr, 0.5, 0.5, width-1, height-1, widget->roundness, corners);
	murrine_set_color_rgb (cr, border);
	cairo_stroke          (cr);

	if (menustyle == 1)
	{
		MurrineRGB *fill = (MurrineRGB*)&colors->spot[1];
		MurrineRGB border2;
		murrine_shade (fill, 0.5, &border2);

		cairo_rectangle (cr, 0.5, 0.5, 3, height-1);
		murrine_set_color_rgb (cr, &border2);
		cairo_stroke_preserve (cr);

		murrine_set_color_rgb (cr, fill);
		cairo_fill (cr);
	}
}

static void
murrine_rgba_draw_statusbar (cairo_t *cr,
                             const MurrineColors    *colors,
                             const WidgetParameters *widget,
                             int x, int y, int width, int height)
{
	const MurrineRGB *dark = &colors->shade[4];
	const MurrineRGB *highlight = &colors->shade[0];

	cairo_set_line_width  (cr, 1);
	cairo_translate       (cr, x, y+0.5);
	cairo_move_to         (cr, 0, 0);
	cairo_line_to         (cr, width, 0);
	murrine_set_color_rgb (cr, dark);
	cairo_stroke          (cr);

	cairo_translate        (cr, 0, 1);
	cairo_move_to          (cr, 0, 0);
	cairo_line_to          (cr, width, 0);
	murrine_set_color_rgba (cr, highlight, 0.5);
	cairo_stroke           (cr);
}

void
murrine_register_style_rgba (MurrineStyleFunctions *functions)
{
	functions->draw_button             = murrine_rgba_draw_button;
	functions->draw_entry              = murrine_rgba_draw_entry;
	functions->draw_scale_trough       = murrine_rgba_draw_scale_trough;
	functions->draw_progressbar_trough = murrine_rgba_draw_progressbar_trough;
	functions->draw_progressbar_fill   = murrine_rgba_draw_progressbar_fill;
	functions->draw_menubar            = murrine_rgba_draw_menubar;
	functions->draw_toolbar            = murrine_rgba_draw_toolbar;
	functions->draw_frame              = murrine_rgba_draw_frame;
	functions->draw_separator          = murrine_rgba_draw_separator;
	functions->draw_tab                = murrine_rgba_draw_tab;
	functions->draw_scrollbar_trough   = murrine_rgba_draw_scrollbar_trough;
	functions->draw_scrollbar_stepper  = murrine_rgba_draw_scrollbar_stepper;
	functions->draw_scrollbar_slider   = murrine_rgba_draw_scrollbar_slider;
	functions->draw_handle             = murrine_rgba_draw_handle;
	functions->draw_tooltip            = murrine_rgba_draw_tooltip;
	functions->draw_radiobutton        = murrine_rgba_draw_radiobutton;
	functions->draw_checkbox           = murrine_rgba_draw_checkbox;
	functions->draw_menu_frame         = murrine_rgba_draw_menu_frame;
	functions->draw_statusbar          = murrine_rgba_draw_statusbar;
}
