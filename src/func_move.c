/* $Id: func_move.c 594 2006-05-02 13:47:00Z kyanh $ */

/* Winefish LaTeX Editor (based on Bluefish)
 *
 * Copyright (c) 2006 kyanh <kyanh@o2.pl>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#define DEBUG

#include <gtk/gtk.h>

#include "bluefish.h"
#include "func_move.h"

/* move cursor or move selection */

gint func_move(GtkWidget *widget, GdkEventKey *kevent, Tbfwin *bfwin, gint opt) {
	DEBUG_MSG("func_move: started with opt=%d, select=%d\n", opt >> FUNC_VALUE_, opt & FUNC_VALUE_0);

	Tdocument *doc = bfwin->current_document;

	if (!doc) return 0;

	GtkTextIter itend, iter, iter_;
	GtkTextMark *mark;
	gint retval  = 1;
	gint c_offset, d_offset;
	gint select;

#if 0
	/* _selection_bounds() always returns iter <= itend :( */
	gtk_text_buffer_get_selection_bounds( doc->buffer, &iter, &itend );
#endif
	mark = gtk_text_buffer_get_insert(doc->buffer);
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &itend, mark); /* cursor */
	mark = gtk_text_buffer_get_selection_bound(doc->buffer);
	gtk_text_buffer_get_iter_at_mark(doc->buffer, &iter, mark);

	select = opt & (FUNC_VALUE_0|FUNC_VALUE_1|FUNC_VALUE_2);
	opt = opt >> FUNC_VALUE_;
	switch(opt) {
	case FUNC_MOVE_LINE:
		if ( ! (select & FUNC_VALUE_0) ) {
			DEBUG_MSG("func_move: move_line failed\n");
			retval=0;
			break;
		}
		DEBUG_MSG("func_move: move_line started...\n");
		if (gtk_text_iter_compare(&iter, &itend) <=0) {
			gtk_text_iter_set_line_offset(&iter, 0);
			iter_ = itend;
			if (gtk_text_iter_forward_line( &itend )  || ( gtk_text_iter_get_line(&iter_) != gtk_text_iter_get_line(&itend) ) )
				gtk_text_iter_backward_char(&itend);
		}else{
			gtk_text_iter_set_line_offset(&itend, 0);
			iter_ = iter;
			if (gtk_text_iter_forward_line( &iter )  || ( gtk_text_iter_get_line(&iter_) != gtk_text_iter_get_line(&iter) ) )
				gtk_text_iter_backward_char(&iter);
		}
		break;
	case FUNC_MOVE_END:
		gtk_text_iter_forward_to_end(&itend);
		break;
	case FUNC_MOVE_START:
		gtk_text_iter_set_line (&itend, 0);
		break;
	case FUNC_MOVE_LINE_START:
		gtk_text_iter_set_line_offset ( &itend, 0 );
		break;
	case FUNC_MOVE_LINE_END:
		if (gtk_text_iter_forward_line( &itend ) || gtk_text_iter_get_line(&iter) != gtk_text_iter_get_line(&itend))
			gtk_text_iter_backward_char(&itend);
		break;
	case FUNC_MOVE_LINE_UP:
		c_offset = gtk_text_iter_get_line_offset(&iter);
		if ( gtk_text_iter_backward_line(&itend) ) {
			d_offset = gtk_text_iter_get_chars_in_line(&itend);
			if (c_offset >= d_offset) c_offset = MAX(0,d_offset - 1);
			gtk_text_iter_set_line_offset(&itend, c_offset);
		}
		break;
	case FUNC_MOVE_LINE_DOWN:
		c_offset = gtk_text_iter_get_line_offset(&iter);
		if (gtk_text_iter_forward_line(&itend) ) {
			d_offset = gtk_text_iter_get_chars_in_line(&itend);
			if (c_offset >= d_offset) c_offset = MAX(0,d_offset - 1);
			gtk_text_iter_set_line_offset(&itend, c_offset);
		}
		break;
	case FUNC_MOVE_CHAR_LEFT:
		gtk_text_iter_backward_char(&itend);
		break;
	case FUNC_MOVE_CHAR_RIGHT:
		gtk_text_iter_forward_char(&itend);
		break;
	case FUNC_MOVE_SENTENCE_START:
		gtk_text_iter_backward_sentence_start(&itend);
		break;
	case FUNC_MOVE_SENTENCE_END:
		gtk_text_iter_forward_sentence_end(&itend);
		break;
	case FUNC_MOVE_WORD_START:
		gtk_text_iter_backward_word_start(&itend);
		break;
	case FUNC_MOVE_WORD_END:
		gtk_text_iter_forward_word_end(&itend);
		break;
#ifdef ENABLE_MOVE_DISPLAY_LINE
	case FUNC_MOVE_DISPLAY_END:
		gtk_text_view_forward_display_line_end(GTK_TEXT_VIEW(doc->view), &itend);
		break;
	case FUNC_MOVE_DISPLAY_START:
		gtk_text_view_backward_display_line_start(GTK_TEXT_VIEW(doc->view), &itend);
		break;
#endif /* ENABLE_MOVE_DISPLAY_LINE */
	default:
		retval = 0;
		break;
	}
	if (retval) {
		if (select & FUNC_VALUE_0 ) {
			gtk_text_buffer_select_range(doc->buffer, &itend, &iter);
		}else if (select & FUNC_VALUE_1) {
			gtk_text_buffer_delete(doc->buffer, &itend, &iter);
		}else{
			gtk_text_buffer_place_cursor(doc->buffer, &itend);
			gtk_text_view_scroll_to_iter( GTK_TEXT_VIEW( doc->view ), &itend, 0, FALSE, 0, 0);
		}
	}
	return retval;
}
