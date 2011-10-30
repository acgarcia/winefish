/* $Id: func_complete.c 586 2006-05-01 04:40:49Z kyanh $ */

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

/* #define DEBUG */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <string.h> /* strstr(), strlen() */

#include "bluefish.h"
#include "snooper2.h"
#include "func_complete.h"

static gboolean find_char( gunichar ch, gchar *data ) {
	return ( strchr( data, ch ) != NULL );
}

static void completion_row_activated_lcb(GtkTreeView *treeview, GtkTreePath *arg1, GtkTreeViewColumn *arg2, Tbfwin *bfwin ) {
	DEBUG_MSG("func_complete: row_activated_ event captured\n");
	func_complete_do( bfwin );
}

static void func_complete_init(Tbfwin *bfwin) {
	DEBUG_MSG("func_complete_init: started\n");

	Tcompletion *cpl;
	cpl = g_new(Tcompletion,1);
	cpl->window = gtk_window_new( GTK_WINDOW_POPUP );
	cpl->treeview = gtk_tree_view_new();
	cpl->show = COMPLETION_WINDOW_HIDE;
	cpl->cache = NULL;
	bfwin->completion = cpl;

	/* add column */
	{
		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;
		renderer = gtk_cell_renderer_text_new ();
		/* g_object_set(renderer, "background", "#ADD8E6", NULL); */
		column = gtk_tree_view_column_new_with_attributes ("", renderer, "text", 0, NULL);
		gtk_tree_view_column_set_min_width(column,100);
		gtk_tree_view_append_column (GTK_TREE_VIEW(cpl->treeview), column);
	}

	/* add scroll */
	GtkWidget *scrolwin;
	scrolwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolwin),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
	
	/* setting for this treeview */
	gtk_tree_view_set_enable_search( GTK_TREE_VIEW(cpl->treeview), FALSE );
	gtk_tree_view_set_headers_visible ( GTK_TREE_VIEW(cpl->treeview), FALSE );
	
	/* add the treeview to scroll window */
	gtk_container_add(GTK_CONTAINER(scrolwin), cpl->treeview);
	/* TODO: better size handler */
	gtk_widget_set_size_request(scrolwin, -1, 100);

	GtkWidget *frame;
	frame = gtk_frame_new(NULL);
	gtk_container_add( GTK_CONTAINER( frame ), scrolwin );
	gtk_container_add( GTK_CONTAINER( cpl->window ), frame );

	g_signal_connect(G_OBJECT(cpl->treeview), "row-activated", G_CALLBACK(completion_row_activated_lcb), bfwin);
}

gint func_complete_hide(Tbfwin *bfwin) {
	DEBUG_MSG("func_complete_hide: started\n");
	if ( SNOOPER_COMPLETION_ON(bfwin) ) {
		Tcompletion *cpl = bfwin->completion;
		gtk_widget_hide_all(cpl->window);
		cpl->show = COMPLETION_WINDOW_HIDE;
		return 1;
	}
#ifdef DEBUG
	else {
		DEBUG_MSG("func_complete_hide: nothing to do\n");
	}
#endif /* DEBUG */
	return 0;
}

gint func_complete_show( GtkWidget *widget_, GdkEventKey *kevent, Tbfwin *bfwin, gint opt ) {
	DEBUG_MSG("func_complete_show: on %p, started with opt=%d\n", bfwin, opt);

	Tdocument *doc = bfwin->current_document;
	GtkWidget *widget = doc->view;
	Tcompletion *cpl;
	GtkTreeModel *model;

	if ( !bfwin->completion ) func_complete_init( bfwin );
	cpl = bfwin->completion;

	if ( kevent && opt & FUNC_FROM_OTHER ) {
		/* để hạn chees tự động gọi khi nhấn phím mũi tên...
		nghĩa là: khi người dùng nhấn ít nhấn hai phím ký tự thif mới bắt đầu show() */
		if ( cpl->show <= COMPLETION_WINDOW_HIDE ) {
			if ( ! SNOOPER_A_CHARS(kevent) || ! SNOOPER_A_CHARS( ( (GdkEventKey *) SNOOPER(bfwin->snooper)->last_event) ) ) {
				DEBUG_MSG("func_complete_show: auto closed\n");
				return 0;
			}
		}
	}


	if ( ! GTK_WIDGET_HAS_FOCUS(widget) ) {
		DEBUG_MSG("func_complete_show: ah... the doc->view has focus \n");
		return 0;
	}

	{
		gchar *buf = NULL;/* func_complete_eat(widget, kevent, doc); */
		{/* get text at doc's iterator */
			GtkTextMark * imark;
			GtkTextIter itstart, iter, maxsearch;
			gint offset;

			imark = gtk_text_buffer_get_insert( doc->buffer );
			gtk_text_buffer_get_iter_at_mark( doc->buffer, &iter, imark );
			itstart = iter;
			maxsearch = iter;
			offset = gtk_text_iter_get_line_offset(&iter);
			gtk_text_iter_backward_chars( &maxsearch, MIN(offset, COMMAND_MAX_LENGTH ));
			/* 50 chars... may it be too long? */
			if ( gtk_text_iter_backward_find_char( &itstart, ( GtkTextCharPredicate ) find_char, GINT_TO_POINTER( "\\" ), &maxsearch ) ) {
				maxsearch = iter; /* re-use maxsearch */
				buf = gtk_text_buffer_get_text( doc->buffer, &itstart, &maxsearch, FALSE );
			}
		}
		DEBUG_MSG("func_complete_show: buffer detected = %s\n", buf);

		if ( !buf || strlen(buf) ==1 || ( opt & FUNC_FROM_OTHER && ( strlen(buf) < 4 ) ) ) {
			DEBUG_MSG("func_complete_show: strlen(buffer) <4. existing...\n");
			if (buf) g_free(buf);
			func_complete_hide(bfwin);
			return 0;
		}

		GList *completion_list_0 = NULL;
		GList *completion_list_1 = NULL;
		GList *completion_list = NULL;

		completion_list_0 = g_completion_complete(main_v->props.completion, buf, NULL);

		if (completion_list_0) {
			completion_list = g_list_copy(completion_list_0);
			completion_list_1 = g_completion_complete(main_v->props.completion_s, buf, NULL);
			if (completion_list_1) {
				completion_list = g_list_concat(completion_list, g_list_copy(completion_list_1));
				g_list_sort(completion_list, (GCompareFunc)strcmp);
			}
		}else {
			completion_list_1 = g_completion_complete(main_v->props.completion_s, buf, NULL);
			if (completion_list_1) {
				completion_list = g_list_copy(completion_list_1);
				g_list_sort(completion_list, (GCompareFunc)strcmp);
			}else{
				DEBUG_MSG("func_complete_show: complete failed. existing...\n");
				g_free(buf);
				func_complete_hide(bfwin);
				return 0;
			}
		}
		/* shouldnot remove completion_list_i after using it */

		/* Adds the second GList onto the end of the first GList.
		Note that the elements of the second GList are not copied.
		They are used directly. */

		/* there is *ONLY* one word and the user reach end of this word */
		if ( ( opt & FUNC_FROM_OTHER )
				&& g_list_length(completion_list) == 1 && strlen (completion_list->data) == strlen(buf) ) {
			DEBUG_MSG("func_complete_show: there's only *one* word. existing...\n");
			g_free(buf);
			func_complete_hide(bfwin);
			return 0;
		}

		g_free(cpl->cache); /* free the old one ;) */
		cpl->cache = buf; /* so we should not free buf */

		GtkListStore *store;
		GtkTreeIter iter;

		/* create list of completion words */
		DEBUG_MSG("func_complete_show: rebuild the word list for treeview\n");
		store = gtk_list_store_new (1, G_TYPE_STRING);

		GList *item;
		item = g_list_first(completion_list);
		while (item) {
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, (gchar *)item->data, -1);
			item = g_list_next(item);
		}
		g_list_free(completion_list);
		model = GTK_TREE_MODEL(store);
	}

	/* the old model will be remove automatically */
	gtk_tree_view_set_model(GTK_TREE_VIEW(cpl->treeview), model);
	g_object_unref(model);

	/** Moving the popup window */
	{
		gint root_x, x, root_y, y;
		GdkWindow *win;
		{
			GdkRectangle rect;
			GtkTextIter iter;
			gtk_text_buffer_get_iter_at_mark( doc->buffer, &iter, gtk_text_buffer_get_insert( doc->buffer ) );	
			gtk_text_view_get_iter_location( GTK_TEXT_VIEW(widget), &iter, &rect);
			gtk_text_view_buffer_to_window_coords( GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_WIDGET, rect.x, rect.y, &x, &y );
		}
		
		gtk_window_get_position(GTK_WINDOW(BFWIN(doc->bfwin)->main_window), &root_x, &root_y);
		x += root_x;
		y += root_y;
		win = gtk_text_view_get_window(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_WIDGET);
	
		/* get the position of text view window relative to its parents */
		gdk_window_get_geometry(win, &root_x, &root_y, NULL, NULL, NULL);
		x += root_x;
		y += root_y;
	
		gtk_window_move (GTK_WINDOW(cpl->window), x+16, y);
		{/* select the first item; need*not* for the first time */
			GtkTreePath *treepath = gtk_tree_path_new_from_string("0");
			if (treepath) {
				gtk_tree_view_set_cursor(GTK_TREE_VIEW(cpl->treeview), treepath, NULL, FALSE);
				gtk_tree_path_free(treepath);
			}
		}
	}

	gtk_widget_show_all(cpl->window);
	cpl->show = COMPLETION_WINDOW_SHOW;
	return 1;
}

gint func_complete_delete(GtkWidget *widget, Tbfwin *bfwin) {
	DEBUG_MSG("func_complete_delete: started\n");
	/* delete stuff:
	- get current position/command
	- hide the popup window
	- delete the selected item. this alters `main_v->props.completion(_s)->items' (GCompletion)
	- rebuild the popup
	- recaculate: func_complete_show(widget, kevent, doc)
	*/
	/* get current selection */
	GtkTreePath *treepath = NULL;
	GtkTreeModel *model;
	Tcompletion *cpl = bfwin->completion;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(cpl->treeview));
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(cpl->treeview), &treepath, NULL);
	gchar *user_selection = NULL;
	if (treepath) {
		GtkTreeIter iter;
		GValue *val = NULL;
		gtk_tree_model_get_iter(model, &iter, treepath);
		gtk_tree_path_free(treepath);
		val = g_new0(GValue, 1);
		gtk_tree_model_get_value(model, &iter, 0, val);
		user_selection = g_strdup((gchar *) (g_value_peek_pointer(val)));
		g_value_unset (val);
		g_free (val);
		DEBUG_MSG("func_complete_delete: user selected '%s'\n", user_selection);
	}
	if (user_selection) {
		func_complete_hide(bfwin);

		/* now delete */
		DEBUG_MSG("func_complete_delete: delete an item '%s'\n", user_selection);
		GList *tmp;
		GList *first_word = NULL;
		gint word_removed = 0;
		/** 0: donot removed; -1: cannot removed from global list; 1: removed; **/

		/** --- how to remove an item `foobar' ---
		we first have a list prefixed by `foobar'.
		get the first item of this list and remove the item from the main list by `g_list_remove_link'.
		finally, remove the item from the main list.
		**/
		/* if (!word_removed) */
		{
			tmp = g_completion_complete(main_v->props.completion,user_selection,NULL);
			if (tmp) {
				first_word = g_list_first(tmp);
				if (strcmp((gchar*)first_word->data, user_selection) == 0 ) {
					g_list_remove_link(tmp,first_word);
					DEBUG_MSG("completion: first word : %s\n",(gchar*)first_word->data);
					g_completion_remove_items(main_v->props.completion, first_word);
					DEBUG_MSG("func_complete_delete: deleted word from global list\n");
					word_removed = 1;
				}
			}
		}
		if (!word_removed) {
			tmp = g_completion_complete(main_v->props.completion_s,user_selection,NULL);
			/* note the session list wasnot sorted. if we remove an item, add it again to
			the session list (by `func_complete_eat', the list is unordered. so we have to
			search through the session_list. we donot use the (g_list_find_custom) as
			tmp is *GCompletion --- !!!! */
			/* TODO: optimized */
			first_word = g_list_last(tmp);
			gint count=0;
			while (first_word && (strcmp((gchar*)first_word->data, user_selection) != 0)) {
				first_word = first_word->prev;
				count ++;
			}
			DEBUG_MSG("func_complete_delete: passed %d words\n",count);
			/* we must ensure the the first item == user_selection */
			if ( first_word ) {
				g_list_remove_link(tmp,first_word);
				DEBUG_MSG("func_complete_delete: first word : %s\n",(gchar*)first_word->data);
				g_completion_remove_items(main_v->props.completion_s, first_word);
				DEBUG_MSG("func_complete_delete: ...selected word from session list\n");
				word_removed = 1;
			}
		}
		{
			gchar *tmpstr = NULL;
			switch(word_removed) {
				case 1: tmpstr = g_strdup_printf(_("deleted '%s'"), user_selection); break;
				case -1: tmpstr = g_strup(_("cannot delete from global list")); break;
				default: break;
			}
			DEBUG_MSG("func_complete_delete: %s\n", tmpstr);
			/* statusbar_message(doc->bfwin, tmpstr, 2000); */
			if (tmpstr) g_free(tmpstr);
		}
		g_free(user_selection);
		func_complete_show(widget, NULL, bfwin, FUNC_FROM_OTHER);/* rebuilt the list; TODO: should forced the result... */
	}else{
		func_complete_hide(bfwin);
	}
	return 1;
}

gint func_complete_move(GdkEventKey *kevent, Tbfwin *bfwin) {
	DEBUG_MSG("func_complete_move: started\n");

	GtkTreeModel *model;
	gint maxnode, index;
	Tcompletion *cpl = bfwin->completion;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(cpl->treeview));
	maxnode = gtk_tree_model_iter_n_children(model, NULL);
	cpl->show = SNOOPER_COMPLETION_MOVE_TYPE(kevent->keyval);

	if (maxnode ==1) {
		func_complete_hide( bfwin );
		return 0;
	}
	GtkTreePath *treepath = NULL;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(cpl->treeview), &treepath, NULL);

	/* TODO: remove this check */
	if (treepath) {
		{
			gchar *path;
			path = gtk_tree_path_to_string(treepath);
			index = atoi(path);
			g_free(path);
		}
		if (cpl->show == COMPLETION_WINDOW_UP) {
			index --;
		} else if (cpl->show == COMPLETION_WINDOW_PAGE_UP) {
			index = index - 5;
		} else if (cpl->show == COMPLETION_WINDOW_DOWN) {
			index ++;
		} else {
			index = index + 5;
		}

		if (index < 0) {
			index = 0;
		} else if (index >= maxnode) {
			index = maxnode -1;
		}
		{
			gchar *tmpstr = g_strdup_printf("%d", index);
			treepath = gtk_tree_path_new_from_string(tmpstr);
			g_free(tmpstr);
		}
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(cpl->treeview), treepath, NULL, FALSE);
		gtk_tree_path_free(treepath);
	}
	cpl->show = COMPLETION_WINDOW_SHOW;
	return 1;
}

gint func_complete_do(Tbfwin *bfwin) {
	DEBUG_MSG("func_complete_do: started\n");
	/* orignal: key-release event */
	func_complete_hide(bfwin);

	/* inserting staff */
	/* get user's selection */
	/* get path and iter */
	GtkTreePath *treepath = NULL;
	GtkTreeModel *model;
	Tdocument *doc = bfwin->current_document;
	Tcompletion *cpl = bfwin->completion;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(cpl->treeview));
	/* lucky, model is*NOT* NULL -- we skip a check :) */

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(cpl->treeview), &treepath, NULL);
	if (treepath) {
		GtkTreeIter iter;
		gchar *user_selection = NULL;
		GValue *val = NULL;
		gint i, len, cache_len;

		gtk_tree_model_get_iter(model, &iter, treepath);
		gtk_tree_path_free(treepath);
#ifdef ENABLE_FIX_UNIKEY_GTK
		GtkTextIter cursor_iter, tmp_iter;
		GtkTextMark *imark;
		gchar *test_buf;
		imark = gtk_text_buffer_get_insert( doc->buffer );
		gtk_text_buffer_get_iter_at_mark( doc->buffer, &cursor_iter, imark );
		tmp_iter = cursor_iter;
		gtk_text_iter_backward_chars(&tmp_iter, 1);
		test_buf = gtk_text_buffer_get_text(doc->buffer, &tmp_iter, &cursor_iter, FALSE);
		if (test_buf[0] == ' ') {
			gtk_text_buffer_delete( doc->buffer, &tmp_iter, &cursor_iter);
		}
#endif /* ENABLE_FIX_UNIKEY_GTK */
		val = g_new0(GValue, 1);
		gtk_tree_model_get_value(model, &iter, 0, val);
		user_selection = g_strdup((gchar *) (g_value_peek_pointer(val)));
		g_value_unset (val);
		g_free (val);
		/* inserting */
		cache_len = strlen(cpl->cache);
		len = strlen(user_selection);
		if (len > cache_len ){
			len = len - cache_len;
			gchar *retval = g_malloc((len+1) * sizeof(char));
			for (i=0; i< len; i++) {
				retval[i] = user_selection[cache_len + i];
			}
			retval[len] = '\0';
			GtkTextIter iter;
			GtkTextMark *imark;
			imark = gtk_text_buffer_get_insert( doc->buffer );
			gtk_text_buffer_get_iter_at_mark( doc->buffer, &iter, imark );
			gtk_text_buffer_insert( doc->buffer, &iter, retval, -1 );
			g_free(retval);
		}
		g_free(user_selection);
	}
	func_complete_hide(bfwin);
	return 1;
}

/*
* kyanh <kyanh@o2.pl>
* add user command to autocompletion list.
* this causes the list be sorted again ==> slower performance*
*/

gint func_complete_eat( GtkWidget *widget, GdkEventKey *kevent, Tbfwin *bfwin, gint opt ) {
	DEBUG_MSG("func_complete_eat: started, FROM-OTHER = %d\n", opt & FUNC_FROM_OTHER);

	Tdocument *doc = bfwin->current_document;
	gchar *buf=NULL;
	GtkTextMark * imark;
	GtkTextIter itstart, iter, maxsearch;
	gint ovector[ 3 ], ret;
	GList *tmplist = NULL;

	if (!doc) return 0;

	if (opt & FUNC_FROM_OTHER) {
		/* only when user press a delimiters that we can start game; */
		guint32 character = gdk_keyval_to_unicode( kevent->keyval );
		if ( (kevent->keyval != GDK_Return) && (character==0 || !strstr(DELIMITERS, kevent->string)) ) {
			DEBUG_MSG("func_complete_eat: returned\n");
			return 0;
		}
	}

	imark = gtk_text_buffer_get_insert( doc->buffer );
	gtk_text_buffer_get_iter_at_mark( doc->buffer, &iter, imark );

	itstart = iter;
	if (opt & FUNC_FROM_OTHER) gtk_text_iter_backward_chars(&itstart, 1);

	iter = itstart;
	maxsearch = itstart;
	ret = gtk_text_iter_get_line_offset(&iter);
	gtk_text_iter_backward_chars( &maxsearch, MIN(ret, COMMAND_MAX_LENGTH) );

	if ( gtk_text_iter_backward_find_char( &itstart, ( GtkTextCharPredicate ) find_char, GINT_TO_POINTER( "\\" ), &maxsearch ) ) {
		maxsearch = iter; /* reuse maxsearch */
		buf = gtk_text_buffer_get_text( doc->buffer, &itstart, &maxsearch, FALSE );
		ret = pcre_exec( main_v->anycommand_regc, NULL, buf, strlen( buf ), 0, PCRE_ANCHORED, ovector, 3 );
		if ( ret <=0 ) {
			DEBUG_MSG("func_complete_eat: \\ not found; return\n");
			g_free(buf);
			return 0;
		}
		tmplist = g_list_find_custom(main_v->props.completion->items, buf, (GCompareFunc)strcmp);
		if ( !tmplist ) {
			tmplist = g_list_find_custom(main_v->props.completion_s->items, buf, (GCompareFunc)strcmp);
			if (!tmplist) {
				gchar *tmpstr = g_strdup(buf);
				tmplist = g_list_append(tmplist, tmpstr);
				/* Cannot use: g_list_append(tmplist, buf); as buf is freed. See below. */
				g_completion_add_items(main_v->props.completion_s, tmplist);
				g_list_free(tmplist);
				DEBUG_MSG("func_complete_eat: new command added: [%s]\n", buf);
			}
		}
	}
	g_free(buf);
	return 1;
}

gint func_complete_eat_env( GtkWidget *widget, GdkEventKey *kevent, Tbfwin *bfwin, gint opt ) {
	DEBUG_MSG("func_complete_eat_env: started with opt = %d\n", opt);

	Tsnooper *snooper = bfwin->snooper;
	Tdocument *doc = bfwin->current_document;
	GtkTextMark * imark;
	GtkTextIter itstart, iter, maxsearch;
	gchar * buf;
	gint ovector[ 12 ], ret;
	gchar * tagname, *toinsert, *indent = NULL;

	if (!doc) return 0;

	if ( ! ( ( kevent->keyval == GDK_braceright )  || (snooper->last_event && ( kevent->hardware_keycode == ((GdkEventKey *)snooper->last_event)->hardware_keycode && ((GdkEventKey *)snooper->last_event)->keyval == GDK_braceright )) ) )
		return 0;

	imark = gtk_text_buffer_get_insert( doc->buffer );
	gtk_text_buffer_get_iter_at_mark( doc->buffer, &iter, imark );

	itstart = iter;
	maxsearch = iter;

	ret = gtk_text_iter_get_line_offset(&iter);
	gtk_text_iter_backward_chars( &maxsearch, MIN(ret, COMMAND_MAX_LENGTH + 5) );
	if ( ! gtk_text_iter_backward_find_char( &itstart, ( GtkTextCharPredicate ) find_char, GINT_TO_POINTER( "\\" ), &maxsearch ) ) {
		DEBUG_MSG("func_complete_eat_env: \\ not found; return\n");
		return 0;
	}

	/* we use a regular expression to check if the tag is valid, AND to parse the tagname from the string */
	maxsearch = iter; /* re-use maxsearch */
	buf = gtk_text_buffer_get_text( doc->buffer, &itstart, &maxsearch, FALSE );
	ret = pcre_exec( main_v->autoclosingtag_regc, NULL, buf, strlen( buf ), 0, PCRE_ANCHORED, ovector, 12 );
	DEBUG_MSG( "func_complete_eat_env: buf='%s'; pcre_exec return %d\n", buf , ret);
	if ( ret <=0 ) {
		g_free(buf);
		return 0;
	}
	tagname = g_strndup( &buf[ ovector[ 2 ] ], ovector[ 3 ] - ovector[ 2 ] );
	DEBUG_MSG( "func_complete_eat_env: tagname='%s'; starts at ovector[2]=%d, ovector[3]=%d\n", tagname, ovector[ 2 ], ovector[ 3 ] );

	{
		gchar *tmpstr = g_strconcat("\\begin{",tagname,NULL);
		GList *search = NULL;
		
		search = g_list_find_custom(main_v->props.completion->items,tmpstr, (GCompareFunc)strcmp);
		if (!search) {
			search = g_list_find_custom(main_v->props.completion_s->items,tmpstr, (GCompareFunc)strcmp);
			if (!search) {
				DEBUG_MSG("func_complete_eat_env: new environment captured: %s\n", tmpstr);
				GList *tmplist = NULL;
				tmplist = g_list_append(tmplist, tmpstr);
				g_completion_add_items(main_v->props.completion_s,tmplist);
				g_list_free(tmplist);
			}
		}
		/* search cannot be freed */
	}
	/* inserting stuff; count the autoindent */
	if (main_v->props.view_bars & MODE_AUTO_INDENT) {
		itstart = iter;
		gtk_text_iter_set_line_index(&itstart, 0);
		indent = gtk_text_buffer_get_text(doc->buffer, &itstart, &iter, FALSE);
		if (indent) {
			gchar *indenting = indent;
			while (*indenting == '\t' || *indenting == ' ' ) {
				indenting++;
			}
			*indenting = '\0';
			if (strlen( indent )) {
				toinsert = g_strconcat( "\n", indent, "\n", indent, "\\end{", tagname, "}", NULL);
			}else{
				toinsert = g_strconcat( "\n\n\\end{", tagname, "}", NULL );
			}
		}else{
			toinsert = g_strconcat( "\n\n\\end{", tagname, "}", NULL );
		}
	}else{
		toinsert = g_strconcat( "\n\n\\end{", tagname, "}", NULL );
	}
	if ( toinsert ) {
		/* we re-use the maxsearch iter now */
		gtk_text_buffer_insert( doc->buffer, &maxsearch, toinsert, -1 );
		/* now we set the cursor back to its previous location, re-using itstart */
		gtk_text_buffer_get_iter_at_mark( doc->buffer, &itstart, gtk_text_buffer_get_insert( doc->buffer ) );
		gint tmpint = strlen( tagname ) + 7; /* NOTE: utf8 tagname ==> use g_utf8_strlen */
		if (indent) {
			gtk_text_iter_backward_chars( &itstart, tmpint + strlen(indent));
			g_free(indent);
		}else{
			gtk_text_iter_backward_chars( &itstart, tmpint);
		}
		/* gint startoffset = gtk_text_iter_get_offset(&itstart); */
		gtk_text_buffer_place_cursor( doc->buffer, &itstart );
		/* doc_unre_add(doc, toinsert, startoffset, startoffset+tmpint, UndoInsert); */
		g_free( toinsert );
	}
	g_free( tagname );
	g_free( buf );
	return 1;
}
