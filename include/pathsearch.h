/*
 * Written by Thierry Laronde <tlaronde@kergis.com> 2024-09
 * Public domain.
 * See pathsearch(7).
 */

#ifndef _PATHSEARCH_H_
#define	_PATHSEARCH_H_

#include <string.h>

/*
 * For the feature testing macros, the parameters are environment
 * variable values of the same but upper case name.
 */
#define PATH_SEARCH_OPT_QFILENAME 'q'
#define PATH_SEARCH_OPT_QFILENAME_FORCED 'Q'
#define	PATH_SEARCH_QFILENAME_ON(path_search_opt) \
( (path_search_opt) != NULL \
	&& strchr(path_search_opt, PATH_SEARCH_OPT_QFILENAME_FORCED) != NULL )

/*
 * A qualified filename should be considered a Resource Identifier
 * to be contrasted to a Locator (where a resource lies in the
 * namespace). See pathsearch(7).
 */
#define PATH_SEARCH_IS_QFILENAME(name, name_len) \
	( (name) != NULL && (name_len) != 0 \
		&& (name)[0] != '/' && strstr(name, "./") == NULL \
		&& (name)[(name_len)-1] != '/' \
		&& (name)[(name_len)-1] != '.' )

#endif /* !_PATHSEARCH_H_ */

