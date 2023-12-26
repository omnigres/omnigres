/**
 * This file is extracted from Postgres 15 to provide an implementation of make_absolute_path
 * for older versions. Context:
 * https://git.postgresql.org/gitweb/?p=postgresql.git;a=commitdiff;h=c10f830c511f0ba3e6f4c9d99f444d39e30440c8^
 *
 * Licensed under the terms of PostgreSQL license
 */
#include "postgres.h"

#if PG_MAJORVERSION_NUM < 15
// clang-format off
#include <ctype.h>
#include <sys/stat.h>
#ifdef WIN32
#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0500
#ifdef near
#undef near
#endif
#define near
#include <shlobj.h>
#else
#include <unistd.h>
#endif


#ifndef WIN32
#define IS_PATH_VAR_SEP(ch) ((ch) == ':')
#else
#define IS_PATH_VAR_SEP(ch) ((ch) == ';')
#endif

static char *trim_directory(char *path);
static void trim_trailing_separator(char *path);
static char *append_subdir_to_path(char *path, char *subdir);


/*
* skip_drive
*
* On Windows, a path may begin with "C:" or "//network/".  Advance over
* this and point to the effective start of the path.
*/
#ifdef WIN32

static char *
skip_drive(const char *path)
{
 if (IS_DIR_SEP(path[0]) && IS_DIR_SEP(path[1]))
 {
   path += 2;
   while (*path && !IS_DIR_SEP(*path))
     path++;
 }
 else if (isalpha((unsigned char) path[0]) && path[1] == ':')
 {
   path += 2;
 }
 return (char *) path;
}
#else

#define skip_drive(path)	(path)
#endif

/* State-machine states for canonicalize_path */
typedef enum
{
 ABSOLUTE_PATH_INIT,			/* Just past the leading '/' (and Windows
                                                                * drive name if any) of an absolute path */
 ABSOLUTE_WITH_N_DEPTH,		/* We collected 'pathdepth' directories in an
                                                                * absolute path */
 RELATIVE_PATH_INIT,			/* At start of a relative path */
 RELATIVE_WITH_N_DEPTH,		/* We collected 'pathdepth' directories in a
                                                                * relative path */
 RELATIVE_WITH_PARENT_REF	/* Relative path containing only double-dots */
} canonicalize_state;

/*
*	Clean up path by:
*		o  make Win32 path use Unix slashes
*		o  remove trailing quote on Win32
*		o  remove trailing slash
*		o  remove duplicate (adjacent) separators
*		o  remove '.' (unless path reduces to only '.')
*		o  process '..' ourselves, removing it if possible
*/
void
canonicalize_path_15(char *path)
{
 char	   *p,
     *to_p;
 char	   *spath;
 char	   *parsed;
 char	   *unparse;
 bool		was_sep = false;
 canonicalize_state state;
 int			pathdepth = 0;	/* counts collected regular directory names */

#ifdef WIN32

 /*
        * The Windows command processor will accept suitably quoted paths with
        * forward slashes, but barfs badly with mixed forward and back slashes.
  */
 for (p = path; *p; p++)
 {
   if (*p == '\\')
     *p = '/';
 }

 /*
        * In Win32, if you do: prog.exe "a b" "\c\d\" the system will pass \c\d"
        * as argv[2], so trim off trailing quote.
  */
 if (p > path && *(p - 1) == '"')
   *(p - 1) = '/';
#endif

 /*
        * Removing the trailing slash on a path means we never get ugly double
        * trailing slashes. Also, Win32 can't stat() a directory with a trailing
        * slash. Don't remove a leading slash, though.
  */
 trim_trailing_separator(path);

 /*
        * Remove duplicate adjacent separators
  */
 p = path;
#ifdef WIN32
 /* Don't remove leading double-slash on Win32 */
 if (*p)
   p++;
#endif
 to_p = p;
 for (; *p; p++, to_p++)
 {
   /* Handle many adjacent slashes, like "/a///b" */
   while (*p == '/' && was_sep)
     p++;
   if (to_p != p)
     *to_p = *p;
   was_sep = (*p == '/');
 }
 *to_p = '\0';

 /*
        * Remove any uses of "." and process ".." ourselves
        *
        * Note that "/../.." should reduce to just "/", while "../.." has to be
        * kept as-is.  Also note that we want a Windows drive spec to be visible
        * to trim_directory(), but it's not part of the logic that's looking at
        * the name components; hence distinction between path and spath.
        *
        * This loop overwrites the path in-place.  This is safe since we'll never
        * make the path longer.  "unparse" points to where we are reading the
        * path, "parse" to where we are writing.
  */
 spath = skip_drive(path);
 if (*spath == '\0')
   return;					/* empty path is returned as-is */

 if (*spath == '/')
 {
   state = ABSOLUTE_PATH_INIT;
   /* Skip the leading slash for absolute path */
   parsed = unparse = (spath + 1);
 }
 else
 {
   state = RELATIVE_PATH_INIT;
   parsed = unparse = spath;
 }

 while (*unparse != '\0')
 {
   char	   *unparse_next;
   bool		is_double_dot;

   /* Split off this dir name, and set unparse_next to the next one */
   unparse_next = unparse;
   while (*unparse_next && *unparse_next != '/')
     unparse_next++;
   if (*unparse_next != '\0')
     *unparse_next++ = '\0';

   /* Identify type of this dir name */
   if (strcmp(unparse, ".") == 0)
   {
     /* We can ignore "." components in all cases */
     unparse = unparse_next;
     continue;
   }

   if (strcmp(unparse, "..") == 0)
     is_double_dot = true;
   else
   {
     /* adjacent separators were eliminated above */
     Assert(*unparse != '\0');
     is_double_dot = false;
   }

   switch (state)
   {
   case ABSOLUTE_PATH_INIT:
     /* We can ignore ".." immediately after / */
     if (!is_double_dot)
     {
       /* Append first dir name (we already have leading slash) */
       parsed = append_subdir_to_path(parsed, unparse);
       state = ABSOLUTE_WITH_N_DEPTH;
       pathdepth++;
     }
     break;
   case ABSOLUTE_WITH_N_DEPTH:
     if (is_double_dot)
     {
       /* Remove last parsed dir */
       /* (trim_directory won't remove the leading slash) */
       *parsed = '\0';
       parsed = trim_directory(path);
       if (--pathdepth == 0)
         state = ABSOLUTE_PATH_INIT;
     }
     else
     {
       /* Append normal dir */
       *parsed++ = '/';
       parsed = append_subdir_to_path(parsed, unparse);
       pathdepth++;
     }
     break;
   case RELATIVE_PATH_INIT:
     if (is_double_dot)
     {
       /* Append irreducible double-dot (..) */
       parsed = append_subdir_to_path(parsed, unparse);
       state = RELATIVE_WITH_PARENT_REF;
     }
     else
     {
       /* Append normal dir */
       parsed = append_subdir_to_path(parsed, unparse);
       state = RELATIVE_WITH_N_DEPTH;
       pathdepth++;
     }
     break;
   case RELATIVE_WITH_N_DEPTH:
     if (is_double_dot)
     {
       /* Remove last parsed dir */
       *parsed = '\0';
       parsed = trim_directory(path);
       if (--pathdepth == 0)
       {
         /*
                                                * If the output path is now empty, we're back to the
                                                * INIT state.  However, we could have processed a
                                                * path like "../dir/.." and now be down to "..", in
                                                * which case enter the correct state for that.
          */
         if (parsed == spath)
           state = RELATIVE_PATH_INIT;
         else
           state = RELATIVE_WITH_PARENT_REF;
       }
     }
     else
     {
       /* Append normal dir */
       *parsed++ = '/';
       parsed = append_subdir_to_path(parsed, unparse);
       pathdepth++;
     }
     break;
   case RELATIVE_WITH_PARENT_REF:
     if (is_double_dot)
     {
       /* Append next irreducible double-dot (..) */
       *parsed++ = '/';
       parsed = append_subdir_to_path(parsed, unparse);
     }
     else
     {
       /* Append normal dir */
       *parsed++ = '/';
       parsed = append_subdir_to_path(parsed, unparse);

       /*
                                        * We can now start counting normal dirs.  But if later
                                        * double-dots make us remove this dir again, we'd better
                                        * revert to RELATIVE_WITH_PARENT_REF not INIT state.
        */
       state = RELATIVE_WITH_N_DEPTH;
       pathdepth = 1;
     }
     break;
   }

   unparse = unparse_next;
 }

 /*
        * If our output path is empty at this point, insert ".".  We don't want
        * to do this any earlier because it'd result in an extra dot in corner
        * cases such as "../dir/..".  Since we rejected the wholly-empty-path
        * case above, there is certainly room.
  */
 if (parsed == spath)
   *parsed++ = '.';

 /* And finally, ensure the output path is nul-terminated. */
 *parsed = '\0';
}

/*
* make_absolute_path from Postgres 15
*
* If the given pathname isn't already absolute, make it so, interpreting
* it relative to the current working directory.
*
* Also canonicalizes the path.  The result is always a malloc'd copy.
*
* In backend, failure cases result in ereport(ERROR); in frontend,
* we write a complaint on stderr and return NULL.
*
* Note: interpretation of relative-path arguments during postmaster startup
* should happen before doing ChangeToDataDir(), else the user will probably
* not like the results.
*/
char *
make_absolute_path_15(const char *path)
{
 char	   *new;

 /* Returning null for null input is convenient for some callers */
 if (path == NULL)
   return NULL;

 if (!is_absolute_path(path))
 {
   char	   *buf;
   size_t		buflen;

   buflen = MAXPGPATH;
   for (;;)
   {
     buf = malloc(buflen);
     if (!buf)
     {
#ifndef FRONTEND
       ereport(ERROR,
               (errcode(ERRCODE_OUT_OF_MEMORY),
                errmsg("out of memory")));
#else
       fprintf(stderr, _("out of memory\n"));
       return NULL;
#endif
     }

     if (getcwd(buf, buflen))
       break;
     else if (errno == ERANGE)
     {
       free(buf);
       buflen *= 2;
       continue;
     }
     else
     {
       int			save_errno = errno;

       free(buf);
       errno = save_errno;
#ifndef FRONTEND
       elog(ERROR, "could not get current working directory: %m");
#else
       fprintf(stderr, _("could not get current working directory: %s\n"),
               strerror(errno));
       return NULL;
#endif
     }
   }

   new = malloc(strlen(buf) + strlen(path) + 2);
   if (!new)
   {
     free(buf);
#ifndef FRONTEND
     ereport(ERROR,
             (errcode(ERRCODE_OUT_OF_MEMORY),
              errmsg("out of memory")));
#else
     fprintf(stderr, _("out of memory\n"));
     return NULL;
#endif
   }
   sprintf(new, "%s/%s", buf, path);
   free(buf);
 }
 else
 {
   new = strdup(path);
   if (!new)
   {
#ifndef FRONTEND
     ereport(ERROR,
             (errcode(ERRCODE_OUT_OF_MEMORY),
              errmsg("out of memory")));
#else
     fprintf(stderr, _("out of memory\n"));
     return NULL;
#endif
   }
 }

 /* Make sure punctuation is canonical, too */
 canonicalize_path_15(new);

 return new;
}

/*
*	trim_directory
*
*	Trim trailing directory from path, that is, remove any trailing slashes,
*	the last pathname component, and the slash just ahead of it --- but never
*	remove a leading slash.
*
* For the convenience of canonicalize_path, the path's new end location
* is returned.
*/
static char *
trim_directory(char *path)
{
 char	   *p;

 path = skip_drive(path);

 if (path[0] == '\0')
   return path;

 /* back up over trailing slash(es) */
 for (p = path + strlen(path) - 1; IS_DIR_SEP(*p) && p > path; p--)
   ;
 /* back up over directory name */
 for (; !IS_DIR_SEP(*p) && p > path; p--)
   ;
 /* if multiple slashes before directory name, remove 'em all */
 for (; p > path && IS_DIR_SEP(*(p - 1)); p--)
   ;
 /* don't erase a leading slash */
 if (p == path && IS_DIR_SEP(*p))
   p++;
 *p = '\0';
 return p;
}


/*
*	trim_trailing_separator
*
* trim off trailing slashes, but not a leading slash
*/
static void
trim_trailing_separator(char *path)
{
 char	   *p;

 path = skip_drive(path);
 p = path + strlen(path);
 if (p > path)
   for (p--; p > path && IS_DIR_SEP(*p); p--)
     *p = '\0';
}

/*
*	append_subdir_to_path
*
* Append the currently-considered subdirectory name to the output
* path in canonicalize_path.  Return the new end location of the
* output path.
*
* Since canonicalize_path updates the path in-place, we must use
* memmove not memcpy, and we don't yet terminate the path with '\0'.
*/
static char *
append_subdir_to_path(char *path, char *subdir)
{
 size_t		len = strlen(subdir);

 /* No need to copy data if path and subdir are the same. */
 if (path != subdir)
   memmove(path, subdir, len);

 return path + len;
}
// clang-format on
#endif