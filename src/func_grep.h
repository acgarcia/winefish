/* $Id$ */

#ifndef __FUNC_GREP_H_
#define __FUNC_GREP_H_

#ifdef EXTERNAL_GREP
#ifdef EXTERNAL_FIND

void file_open_advanced_cb( Tbfwin *bfwin, gboolean open_files);
void open_advanced_from_filebrowser( Tbfwin *bfwin, gchar *path );

#endif /* EXTERNAL_GREP */
#endif /* EXTERNAL_FIND */

#endif /* __FUNC_GREP_H_ */
