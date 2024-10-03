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
 * namespace). So it has to be non rooted and without walking path
 * directives ("." or ".."). See pathsearch(7).
 * Forbidding any component of the identifer to end with a dot will
 * make the test more easier. But, for now, don't restrict
 * identifiers.
 * We try in this to succeed or fail early for traditional last
 * leaf identifiers.
 */
#define PATH_SEARCH_IS_QFILENAME(name, name_len) \
( (name) != NULL && (name_len) != 0 \
  && (name)[0] != '/' \
  && (name)[name_len-1] != '/' \
  && ( \
    (name_len == 1 && (name)[0] != '.') \
    || ( name_len == 2 && ( (name)[0] != '.' || (name)[1] != '.' ) ) \
    || ( name_len >= 3 \
         && ( \
           ( strstr(name, "/") == NULL || strstr(name, ".") == NULL ) \
           || ( \
              strncmp(name, "./", 2) != 0 \
              && strncmp(name, "../", 3) != 0 \
              && strstr(name, "/./") == NULL \
              && strstr(name, "/../") == NULL \
              && ( \
                 (name)[name_len-1] != '.' \
                   || (  (name)[name_len-2] != '/' \
                      && ( \
                           (name)[name_len-2] != '.' \
                           || (name)[name_len-3] != '/' \
                         ) \
                      ) \
                 ) \
	      ) \
           ) \
       ) \
     ) \
)

#endif /* !_PATHSEARCH_H_ */

