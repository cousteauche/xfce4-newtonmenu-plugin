/*  $Id$
 *
 *  Copyright (C) 2012 John Doo <john@foo.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __NEWTONBUTTON_H__
#define __NEWTONBUTTON_H__

G_BEGIN_DECLS

/* plugin structure */
// SEED:ConsiderAdvancedStyling - Explore options for more advanced button styling (padding, margins, custom CSS).

typedef struct
{
    XfcePanelPlugin *plugin;

    /* panel widgets */
    GtkWidget       *event_box;     /* Main clickable area, might contain the button */
    GtkWidget       *button;        /* The actual button widget displayed on the panel */
    GtkWidget       *button_box;    /* A GtkBox inside the button to hold icon and/or label */
    GtkWidget       *icon_image;    /* GtkImage to display the icon */
    GtkWidget       *label_widget;  /* GtkLabel to display the text */

    /* newtonbutton settings */
    gboolean         display_icon_prop; /* TRUE to display icon, FALSE to display text label */
    gchar           *icon_name_prop;   /* Icon name (from theme) or path to an image file */
    gchar           *label_text_prop;  /* Text to display if not using an icon */
    // SEED:AddIconSizeConfig - Add settings for icon size (e.g., small, panel-relative, custom px).
    // SEED:AddLabelFontConfig - Add settings for label font and color.

    /* Internal state */
    // SEED:AddMenuCache - Cache the main menu if it's static or doesn't change often.
}
NewtonbuttonPlugin;



void
newtonbutton_save (XfcePanelPlugin *plugin,
             NewtonbuttonPlugin    *newtonbutton);

G_END_DECLS

#endif /* !__NEWTONBUTTON_H__ */
