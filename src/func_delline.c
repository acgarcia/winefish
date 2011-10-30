/* $Id: func_delline.c 566 2006-04-30 01:13:01Z kyanh $ */

/* Winefish LaTeX Editor (based on Bluefish HTML Editor)
 *
 * Copyright (C) 2006 kyanh <kyanh@o2.pl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <gtk/gtk.h>

#include "bluefish.h"

/*
 opt & FUNC_VALUE_0: delete the whole line
 opt & FUNC_VALUE_1: delete to right (end of line)
 opt & FUNC_VALUE_2: delete to left (begining of line)
*/
gint func_delete_line( GtkWidget *widget, GdkEventKey *kevent, Tbfwin *bfwin, gint opt ) {
	DEBUG_MSG("func_delete_line: started with opt = %d\n", opt);
	Tdocument *doc = bfwin->current_document;
	if (!doc) return 0;

	GtkTextIter itstart, itend;
	if ( gtk_text_buffer_get_selection_bounds( doc->buffer, &itstart, &itend ) ) return 0;
	/* delete the whole line */
	if ( opt & FUNC_VALUE_0 ) {
		gtk_text_iter_set_line_offset( &itstart, 0 );
		if (gtk_text_iter_forward_line( &itend )  || ( gtk_text_iter_get_line(&itstart) != gtk_text_iter_get_line(&itend) ) )
			gtk_text_iter_backward_char(&itend);
	/* delete to end of line */
	} else if (opt & FUNC_VALUE_1 ) {
		if (gtk_text_iter_forward_line( &itend )  || ( gtk_text_iter_get_line(&itstart) != gtk_text_iter_get_line(&itend) ) )
			gtk_text_iter_backward_char(&itend);
	/* delete to begin of line */
	} else if (opt & FUNC_VALUE_2 ) {
		gtk_text_iter_set_line_offset( &itstart, 0 );
	}
	if (gtk_text_iter_compare(&itstart, &itend) == -1 ) {
		gtk_text_buffer_delete( doc->buffer, &itstart, &itend );
		return 1;
	}else if ( gtk_text_iter_compare(&itstart, &itend) ==0
			  && ( opt & FUNC_VALUE_1 || ( opt & FUNC_VALUE_0 && gtk_text_iter_get_line_offset(&itend) ==0 ) ) ) {
		gtk_text_iter_forward_char(&itend);
		gtk_text_buffer_delete( doc->buffer, &itstart, &itend );
		return 1;
	}
	return 0;
}

void menu_del_line_cb( Tbfwin *bfwin, guint callback_action, GtkWidget *widget ) {
	gint opt = 0;
	switch (callback_action) {
	case 1: opt = FUNC_VALUE_1; break;
	case 2: opt = FUNC_VALUE_2; break;
	default: opt = FUNC_VALUE_0; break;
	}
	func_delete_line( widget, NULL, bfwin, opt );
}
