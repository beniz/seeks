/**
 * Copyright   :  Modified by Emmanuel Benazera for the Seeks Project,
 *                2009, 2010, 2011
 *
 *                Written by and Copyright (C) 2001-2009 the SourceForge
 *                Privoxy team. http://www.privoxy.org/
 *
 *                Based on the Internet Junkbuster originally written
 *                by and Copyright (C) 1997 Anonymous Coders and
 *                Junkbusters Corporation.  http://www.junkbusters.com
 *
 *                This program is free software; you can redistribute it
 *                and/or modify it under the terms of the GNU General
 *                Public License as published by the Free Software
 *                Foundation; either version 2 of the License, or (at
 *                your option) any later version.
 *
 *                This program is distributed in the hope that it will
 *                be useful, but WITHOUT ANY WARRANTY; without even the
 *                implied warranty of MERCHANTABILITY or FITNESS FOR A
 *                PARTICULAR PURPOSE.  See the GNU General Public
 *                License for more details.
 *
 *                The GNU General Public License should be included with
 *                this file.  If not, you can view it at
 *                http://www.gnu.org/copyleft/gpl.html
 *                or write to the Free Software Foundation, Inc., 59
 *                Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *********************************************************************/

#include "miscutil.h"
#include "mem_utils.h"

#include <ctype.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include <algorithm>
#include <iostream>

#define TRUE 1
#define FALSE 0

#ifndef HAVE_STRNDUP
char *strndup(char *str, size_t len)
{
  char *dup= (char *)malloc( len+1 );
  if (dup)
    {
      strncpy(dup,str,len);
      dup[len]= '\0';
    }
  return dup;
}
#endif


namespace sp
{
#ifdef USE_SEEKS_STRLCPY
  /*********************************************************************
   *
   * Function    :  privoxy_strlcpy
   *
   * Description :  strlcpy(3) look-alike for those without decent libc.
   *
   * Parameters  :
   *          1  :  destination: buffer to copy into.
   *          2  :  source: String to copy.
   *          3  :  size: Size of destination buffer.
   *
   * Returns     :  The length of the string that privoxy_strlcpy() tried to create.
   *
   *********************************************************************/
  size_t miscutil::seeks_strlcpy(char *destination, const char *source, const size_t size)
  {
    if (0 < size)
      {
        snprintf(destination, size, "%s", source);
        /*
         * Platforms that lack strlcpy() also tend to have
         * a broken snprintf implementation that doesn't
         * guarantee nul termination.
         *
         * XXX: the configure script should detect and reject those.
         */
        destination[size-1] = '\0';
      }
    return strlen(source);
  }
#endif /* def USE_PRIVOXY_STRLCPY */

#ifndef HAVE_STRLCAT
  /*********************************************************************
   *
   * Function    :  privoxy_strlcat
   *
   * Description :  strlcat(3) look-alike for those without decent libc.
   *
   * Parameters  :
   *          1  :  destination: C string.
   *          2  :  source: String to copy.
   *          3  :  size: Size of destination buffer.
   *
   * Returns     :  The length of the string that privoxy_strlcat() tried to create.
   *
   *********************************************************************/
  size_t miscutil::seeks_strlcat(char *destination, const char *source, const size_t size)
  {
    const size_t old_length = strlen(destination);
    return old_length + strlcpy(destination + old_length, source, size - old_length);
  }
#endif /* ndef HAVE_STRLCAT */

  /**************************************************************
   *
   * Function    :  strcmpic
   *
   * Description :  Case insensitive string comparison
   *
   * Parameters  :
   *          1  :  s1 = string 1 to compare
   *          2  :  s2 = string 2 to compare
   *
   * Returns     :  0 if s1==s2, Negative if s1<s2, Positive if s1>s2
   *
   *********************************************************************/
  int miscutil::strcmpic(const char *s1, const char *s2)
  {
    if (!s1) s1 = "";
    if (!s2) s2 = "";

    while (*s1 && *s2)
      {
        if ( ( *s1 != *s2 ) && ( tolower(*s1) != tolower(*s2) ) )
          {
            break;
          }
        s1++, s2++;
      }
    return(tolower(*s1) - tolower(*s2));
  }

  /****************************************************************
   *
   * Function    :  strncmpic
   *
   * Description :  Case insensitive string comparison (upto n characters)
   *
   * Parameters  :
   *          1  :  s1 = string 1 to compare
   *          2  :  s2 = string 2 to compare
   *          3  :  n = maximum characters to compare
   *
   * Returns     :  0 if s1==s2, Negative if s1<s2, Positive if s1>s2
   *
   *********************************************************************/
  int miscutil::strncmpic(const char *s1, const char *s2, size_t n)
  {
    if (n <= (size_t)0) return(0);
    if (!s1) s1 = "";
    if (!s2) s2 = "";

    while (*s1 && *s2)
      {
        if ( ( *s1 != *s2 ) && ( tolower(*s1) != tolower(*s2) ) )
          {
            break;
          }
        if (--n <= (size_t)0) break;
        s1++, s2++;
      }
    return(tolower(*s1) - tolower(*s2));
  }


  /* replacement for list::map() in the original implementation. */
  sp_err miscutil::add_map_entry(hash_map<const char*,const char*,hash<const char*>,eqstr> *the_map,
                                 const char *name, int name_needs_copying,
                                 const char *value, int value_needs_copying)
  {
    if ( (NULL == value)
         || (NULL == name) )
      {
        if ((name != NULL) && (!name_needs_copying))
          {
            free_const(name);
          }
        if ((value != NULL) && (!value_needs_copying))
          {
            free_const(name);
          }
        return SP_ERR_MEMORY;
      }

    if (name_needs_copying)
      {
        if (NULL == (name = strdup(name)))
          {
            if (!value_needs_copying)
              {
                free_const(value);
              }
            return SP_ERR_MEMORY;
          }
      }

    if (value_needs_copying)
      {
        if (NULL == (value = strdup(value)))
          {
            free_const(name);
            return SP_ERR_MEMORY;
          }
      }

    the_map->insert(std::pair<const char*,const char*>(name,value));

    return SP_ERR_OK;
  }

  /* unmap. */
  sp_err miscutil::unmap(hash_map<const char*,const char*,hash<const char*>,eqstr> *the_map, const char *name)
  {
    hash_map<const char*,const char*,hash<const char*>,eqstr>::iterator mit;
    if ((mit=the_map->find(name)) != the_map->end())
      {
        const char *key = (*mit).first;
        const char *value = (*mit).second;
        the_map->erase(mit);
        free_const(key);
        free_const(value);
      }
    return SP_ERR_OK;
  }

  /* free map. */
  void miscutil::free_map(hash_map<const char*,const char*,hash<const char*>,eqstr> *&the_map)
  {
    hash_map<const char*,const char*,hash<const char*>,eqstr>::iterator mit
    = the_map->begin();
    while (mit!=the_map->end())
      {
        const char *key = (*mit).first;
        const char *value = (*mit).second;
        hash_map<const char*,const char*,hash<const char*>,eqstr>::iterator mitc = mit;
        ++mit;
        the_map->erase(mitc);
        free_const(key);
        if (value)
          free_const(value);
      }
    delete the_map;
    the_map = NULL;
  }

  /* copy map. */
  hash_map<const char*,const char*,hash<const char*>,eqstr>* miscutil::copy_map(const hash_map<const char*,const char*,hash<const char*>,eqstr> *the_map)
  {
    hash_map<const char*,const char*,hash<const char*>,eqstr> *cmap
    = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
    hash_map<const char*,const char*,hash<const char*>,eqstr>::const_iterator hit
    = the_map->begin();
    while (hit!=the_map->end())
      {
        miscutil::add_map_entry(cmap,(*hit).first,1,(*hit).second,1);
        ++hit;
      }
    return cmap;
  }

  /* lookup: beware this is a lookup for a single item only! */
  const char* miscutil::lookup(const hash_map<const char*,const char*,hash<const char*>,eqstr> *the_map,
                               const char* name)
  {
    assert(the_map);
    assert(name);

    hash_map<const char*,const char*,hash<const char*>,eqstr>::const_iterator mit;
    if ((mit = the_map->find(name)) != the_map->end())
      return (*mit).second;
    else return NULL;
  }

  /* operations on lists. */
  sp_err miscutil::enlist(std::list<const char*> *the_list, const char *str)
  {
    assert(the_list);

    char *nstr = NULL;
    if (str)
      {
        if (NULL == (nstr = strdup(str)))
          {
            return SP_ERR_MEMORY;
          }

        the_list->push_back(nstr);
      }

    return SP_ERR_OK;
  }

  sp_err miscutil::enlist_first(std::list<const char*> *the_list, const char *str)
  {
    assert(the_list);

    char *nstr = NULL;
    if (str)
      {
        if (NULL == (nstr = strdup(str)))
          {
            return SP_ERR_MEMORY;
          }

        the_list->push_front(nstr);
      }

    return SP_ERR_OK;
  }

  /*********************************************************************
   *
   * Function    :  enlist_unique
   *
   * Description :  Append a string into a specified string list,
   *                if & only if it's not there already.
   *                If the num_significant_chars argument is nonzero,
   *                only compare up to the nth character.
   *
   * Parameters  :
   *          1  :  the_list = pointer to list
   *          2  :  str = string to add to the list
   *          3  :  num_significant_chars = number of chars to use
   *                for uniqueness test, or 0 to require an exact match.
   *
   * Returns     :  SP_ERR_OK on success
   *                SP_ERR_MEMORY on out-of-memory error.
   *                On error, the_list will be unchanged.
   *                "Success" does not indicate whether or not the
   *                item was already in the list.
   *
   *********************************************************************/
  sp_err miscutil::enlist_unique(std::list<const char*> *the_list,
                                 const char *str, size_t num_significant_chars)
  {
    assert(the_list);
    assert(str);
    assert(num_significant_chars >= 0);
    assert(num_significant_chars <= strlen(str));

    if (num_significant_chars > 0)
      {
        std::list<const char*>::const_iterator lit = the_list->begin();
        while (lit!=the_list->end())
          {
            if ( ((*lit) != NULL)
                 && (0 == strncmp(str,(*lit),num_significant_chars)) )
              return SP_ERR_OK; // already there.
            ++lit;
          }
        return miscutil::enlist(the_list,str);
      }
    else
      {
        std::list<const char*>::const_iterator lit = the_list->begin();
        while (lit!=the_list->end())
          {
            if ( ((*lit) != NULL)
                 && (0 == strcmp(str,(*lit))) )
              return SP_ERR_OK; // already there.
            ++lit;
          }
        return miscutil::enlist(the_list,str);
      }
  }

  /*********************************************************************
   *
   * Function    :  enlist_unique_header
   *
   * Description :  Make a HTTP header from the two strings name and value,
   *                and append the result into a specified string list,
   *                if & only if there isn't already a header with that name.
   *
   * Parameters  :
   *          1  :  the_list = pointer to list
   *          2  :  name = HTTP header name (e.g. "Content-type")
   *          3  :  value = HTTP header value (e.g. "text/html")
   *
   * Returns     :  SP_ERR_OK on success
   *                SP_ERR_MEMORY on out-of-memory error.
   *                On error, the_list will be unchanged.
   *                "Success" does not indicate whether or not the
   *                header was already in the list.
   *
   *********************************************************************/
  sp_err miscutil::enlist_unique_header(std::list<const char*> *the_list,
                                        const char *name, const char *value)
  {
    sp_err result = SP_ERR_MEMORY;
    char *header;
    size_t header_size;

    assert(the_list);
    assert(name);
    assert(value);

    /* + 2 for the ': ', + 1 for the \0 */
    header_size = strlen(name) + 2 + strlen(value) + 1;
    header = (char*) zalloc(header_size);

    if (NULL != header)
      {
        const size_t bytes_to_compare = strlen(name) + 2;
        snprintf(header, header_size, "%s: %s", name, value);
        result = miscutil::enlist_unique(the_list, header, bytes_to_compare);
        freez(header);
      }
    return result;
  }

  int miscutil::list_remove_item(std::list<const char*> *the_list, const char *str)
  {
    assert(the_list);

    int count = 0;
    std::list<const char*>::iterator lit = the_list->begin();
    while (lit!=the_list->end())
      {
        if ((*lit) && strcmp((*lit),str)==0)
          {
            free_const((*lit));
            std::list<const char*>::iterator clit = lit;
            ++lit;
            the_list->erase(clit);
            count++;
          }
        else ++lit;
      }
    return count;
  }

  int miscutil::list_remove_list(std::list<const char*> *dest, const std::list<const char*> *src)
  {
    assert(src);
    assert(dest);

    int count = 0;
    std::list<const char*>::const_iterator lit = src->begin();
    while (lit!=src->end())
      {
        count += miscutil::list_remove_item(dest,(*lit));
        ++lit;
      }
    return count;
  }

  void miscutil::list_remove_all(std::list<const char*> *the_list)
  {
    assert(the_list);

    std::list<const char*>::iterator lit = the_list->begin();
    while (lit!=the_list->end())
      {
        if ((*lit))
          free_const((*lit));
        std::list<const char*>::iterator clit = lit;
        ++lit;
        the_list->erase(clit);
      }
    the_list->clear();
  }

  char* miscutil::list_to_text(const std::list<const char*> *the_list)
  {
    char *text;
    char *cursor;
    size_t bytes_left;

    assert(the_list);

    /*
     * Calculate the length of the final text.
     * '2' because of the '\r\n' at the end of
     * each string and at the end of the text.
     */
    size_t text_length = 2;
    std::list<const char*>::const_iterator lit = the_list->begin();
    while (lit!=the_list->end())
      {
        if ((*lit))
          {
            text_length += strlen((*lit))+2;
          }
        ++lit;
      }

    bytes_left = text_length + 1;

    text = (char*) std::malloc(bytes_left);
    if (text == NULL)
      {
        return NULL;
      }

    cursor = text;

    lit = the_list->begin();
    while (lit!=the_list->end())
      {
        if ((*lit))
          {
            const int written = snprintf(cursor, bytes_left, "%s\r\n", (*lit));

            assert(written > 0);
            assert(written < (int)bytes_left);

            bytes_left -= (size_t)written;
            cursor += (size_t)written;
          }
        ++lit;
      }

    assert(bytes_left == 3);

    *cursor++ = '\r';
    *cursor++ = '\n';
    *cursor   = '\0';

    assert((int)text_length == cursor - text);
    assert(text[text_length] == '\0');

    return text;
  }

  int miscutil::list_contains_item(const std::list<const char*> *the_list,
                                   const char *str)
  {
    assert(the_list);
    assert(str);

    std::list<const char*>::const_iterator lit = the_list->begin();
    while (lit!=the_list->end())
      {
        if ((*lit) == NULL)
          {
            /*
             * NULL pointers are allowed in some lists.
             * For example for csp->headers in case a
             * header was removed.
             */
            ++lit;
            continue;
          }

        if (0 == strcmp(str, (*lit)))
          {
            /* Item found */
            return TRUE;
          }

        ++lit;
      }

    return FALSE;
  }

  int miscutil::list_duplicate(std::list<const char*> *dest,
                               const std::list<const char*> *src)
  {
    assert(src);
    assert(dest);

    miscutil::list_remove_all(dest);

    std::list<const char*>::const_iterator lit = src->begin();
    while (lit!=src->end())
      {
        if ((*lit))
          {

            const char *str = strdup((*lit));
            dest->push_back(str);
          }
        ++lit;
      }
    return SP_ERR_OK;
  }

  int miscutil::list_append_list_unique(std::list<const char*> *dest,
                                        const std::list<const char*> *src)
  {
    assert(src);
    assert(dest);

    std::list<const char*>::const_iterator lit = src->begin();
    while (lit!=src->end())
      {
        if ((*lit))
          {
            if (miscutil::enlist_unique(dest,(*lit),0))
              return SP_ERR_MEMORY;
          }
        ++lit;
      }
    return SP_ERR_OK;
  }

  /* operations on strings. */

  /* Define this for lots of debugging information to stdout */
#undef SSPLIT_VERBOSE
  /* #define SSPLIT_VERBOSE 1 */

  /*********************************************************************
   *
   * Function    :  ssplit
   *
   * Description :  Split a string using delimiters in delim'.  Results
   *                go into vec'.
   *
   * Parameters  :
   *          1  :  str = string to split.  Will be split in place
   *                (i.e. do not free until you've finished with vec,
   *                previous contents will be trashed by the call).
   *          2  :  delim = array of delimiters (if NULL, uses " \t").
   *          3  :  vec[] = results vector (aka. array) [out]
   *          4  :  vec_len = number of usable slots in the vector (aka. array size)
   *          5  :  dont_save_empty_fields = zero if consecutive delimiters
   *                give a null output field(s), nonzero if they are just
   *                to be considered as single delimeter
   *          6  :  ignore_leading = nonzero to ignore leading field
   *                separators.
   *
   * Returns     :  -1 => Error: vec_len is too small to hold all the
   *                      data, or str == NULL.
   *                >=0 => the number of fields put in vec'.
   *                On error, vec and str may still have been overwritten.
   *
   *********************************************************************/
  int miscutil::ssplit(char *str, const char *delim, char *vec[], int vec_len,
                       int dont_save_empty_fields, int ignore_leading)
  {
    unsigned char is_delim[256];
    unsigned char char_type;
    int vec_count = 0;

    if (!str)
      {
        return(-1);
      }

    /* Build is_delim array */
    memset(is_delim, '\0', sizeof(is_delim));

    if (!delim)
      {
        delim = " \t";  /* default field separators */
      }

    while (*delim)
      {
        is_delim[(unsigned)(unsigned char)*delim++] = 1;   /* separator  */
      }

    is_delim[(unsigned)(unsigned char)'\0'] = 2;   /* terminator */
    is_delim[(unsigned)(unsigned char)'\n'] = 2;   /* terminator */

    /* Parse string */
    if (ignore_leading)
      {
        /* skip leading separators */
        while (is_delim[(unsigned)(unsigned char)*str] == 1)
          {
            str++;
          }
      }

    /* first pointer is the beginning of string */
    /* Check if we want to save this field */
    if ( (!dont_save_empty_fields)
         || (is_delim[(unsigned)(unsigned char)*str] == 0) )
      {
        /*
         * We want empty fields, or the first character in this
         * field is not a delimiter or the end of string.
         * So save it.
         */
        if (vec_count >= vec_len)
          {
            return(-1); /* overflow */
          }

        vec[vec_count++] = (char *) str;
      }

    while ((char_type = is_delim[(unsigned)(unsigned char)*str]) != 2)
      {
        if (char_type == 1)
          {
            /* the char is a separator */
            /* null terminate the substring */
            *str++ = '\0';

            /* Check if we want to save this field */
            if ( (!dont_save_empty_fields)
                 || (is_delim[(unsigned)(unsigned char)*str] == 0) )
              {
                /*
                * We want empty fields, or the first character in this
                * field is not a delimiter or the end of string.
                * So save it.
                */
                if (vec_count >= vec_len)
                  {
                    return(-1); /* overflow */
                  }

                vec[vec_count++] = (char *) str;
              }
          }
        else
          {
            str++;
          }
      }

    *str = '\0';     /* null terminate the substring */
#ifdef SSPLIT_VERBOSE
    {
      int i;
      printf("dump %d strings\n", vec_count);
      for (i = 0; i < vec_count; i++)
        {
          printf("%d '%s'\n", i, vec[i]);
        }
    }
#endif /* def SSPLIT_VERBOSE */

    return(vec_count);
  }

  /*********************************************************************
   *
   * Function    :  string_append
   *
   * Description :  Reallocate target_string and append text to it.
   *                This makes it easier to append to malloc'd strings.
   *                This is similar to the (removed) strsav(), but
   *                running out of memory isn't catastrophic.
   *
   *                Programming style:
   *
   *                The following style provides sufficient error
   *                checking for this routine, with minimal clutter
   *                in the source code.  It is recommended if you
   *                have many calls to this function:
   *
   *                char * s = strdup(...); // don't check for error
   *                string_append(&s, ...);  // don't check for error
   *                string_append(&s, ...);  // don't check for error
   *                string_append(&s, ...);  // don't check for error
   *                if (NULL == s)
   *                   {
   *                    ... handle error ...
   *                   }
   *
   *
   *                OR, equivalently:
   *
   *                char * s = strdup(...); // don't check for error
   *                string_append(&s, ...);  // don't check for error
   *                string_append(&s, ...);  // don't check for error
   *                if (string_append(&s, ...))
   *                     {
   *                         ... handle error ...
   *                     }
   *
   *
   * Parameters  :
   *          1  :  target_string = Pointer to old text that is to be
   *                extended.  *target_string will be free()d by this
   *                routine.  target_string must be non-NULL.
   *                If *target_string is NULL, this routine will
   *                do nothing and return with an error - this allows
   *                you to make many calls to this routine and only
   *                check for errors after the last one.
   *          2  :  text_to_append = Text to be appended to old.
   *                Must not be NULL.
   *
   * Returns     :  SP_ERR_OK on success, and sets *target_string
   *                   to newly malloc'ed appended string.  Caller
   *                   must free(*target_string).
   *                SP_ERR_MEMORY on out-of-memory.  (And free()s
   *                   *target_string and sets it to NULL).
   *                SP_ERR_MEMORY if *target_string is NULL.
   *
   *********************************************************************/
  sp_err miscutil::string_append(char **target_string, const char *text_to_append)
  {
    size_t old_len;
    char *new_string;
    size_t new_size;

    assert(target_string);
    assert(text_to_append);

    if (*target_string == NULL)
      {
        return SP_ERR_MEMORY;
      }

    if (*text_to_append == '\0')
      {
        return SP_ERR_OK;
      }

    old_len = strlen(*target_string);
    new_size = strlen(text_to_append) + old_len + 1;

    new_string = (char*) zalloc(new_size);

    if (new_string == NULL)
      return SP_ERR_MEMORY;

    std::copy(*target_string,*target_string+old_len,new_string); // copy into the longer string.
    freez(*target_string);

    strlcpy(new_string + old_len, text_to_append, new_size - old_len);

    *target_string = new_string;
    return SP_ERR_OK;
  }

  /********************************************************************
   *
   * Function    :  string_join
   *
   * Description :  Join two strings together.  Frees BOTH the original
   *                strings.  If either or both input strings are NULL,
   *                fails as if it had run out of memory.
   *
   *                For comparison, string_append requires that the
   *                second string is non-NULL, and doesn't free it.
   *
   *                Rationale: Too often, we want to do
   *                string_append(s, html_encode(s2)).  That assert()s
   *                if s2 is NULL or if html_encode() runs out of memory.
   *                It also leaks memory.  Proper checking is cumbersome.
   *                The solution: string_join(s, html_encode(s2)) is safe,
   *                and will free the memory allocated by html_encode().
   *
   * Parameters  :
   *          1  :  target_string = Pointer to old text that is to be
   *                extended.  *target_string will be free()d by this
   *                routine.  target_string must be non-NULL.
   *          2  :  text_to_append = Text to be appended to old.
   *
   * Returns     :  SP_ERR_OK on success, and sets *target_string
   *                   to newly malloc'ed appended string.  Caller
   *                   must free(*target_string).
   *                SP_ERR_MEMORY on out-of-memory, or if
   *                   *target_string or text_to_append is NULL.  (In
   *                   this case, frees *target_string and text_to_append,
   *                   sets *target_string to NULL).
   *
   *********************************************************************/
  sp_err miscutil::string_join(char **target_string, char *text_to_append)
  {
    sp_err err;

    assert(target_string);

    if (text_to_append == NULL)
      {
        freez(*target_string);
        *target_string = NULL;
        return SP_ERR_MEMORY;
      }

    err = miscutil::string_append(target_string, text_to_append);

    freez(text_to_append);
    text_to_append = NULL;

    return err;
  }

  /**********************************************************************
   *
   * Function    :  string_toupper
   *
   * Description :  Produce a copy of string with all convertible
   *                characters converted to uppercase.
   *
    * Parameters  :
   *          1  :  string = string to convert
   *
   * Returns     :  Uppercase copy of string if possible,
   *                NULL on out-of-memory or if string was NULL.
   *
   *********************************************************************/
  char* miscutil::string_toupper(const char *string)
  {
    char *result, *p;
    const char *q;

    if (!string
        || ((result = (char*) zalloc(strlen(string)+1)) == NULL))
      {
        return NULL;
      }

    q = string;
    p = result;

    while (*q != '\0')
      {
        *p++ = (char)toupper((int) *q++);
      }

    return result;
  }

  /*********************************************************************
   *
   * Function    :  bindup
   *
   * Description :  Duplicate the first n characters of a string that may
   *                contain '\0' characters.
   *
   * Parameters  :
   *          1  :  string = string to be duplicated
   *          2  :  len = number of bytes to duplicate
   *
   * Returns     :  pointer to copy, or NULL if failiure
   *
   *********************************************************************/
  char* miscutil::bindup(const char *string, size_t len)
  {
    char *duplicate;
    if (NULL == (duplicate = (char *)malloc(len)))
      {
        return NULL;
      }
    else
      {
        memcpy(duplicate, string, len);
      }
    return duplicate;
  }

  /*********************************************************************
   *
   * Function    :  chomp
   *
   * Description :  In-situ-eliminate all leading and trailing whitespace
   *                from a string.
   *
   * Parameters  :
   *          1  :  s : string to be chomped.
   *
   * Returns     :  chomped string
   *
   *********************************************************************/
  char *miscutil::chomp(char *string)
  {
    char *p, *q, *r;

    /*
     * strip trailing whitespace
     */
    p = string + strlen(string);
    while (p > string && isspace(*(p-1)))
      {
        p--;
      }
    *p = '\0';

    /*
     * find end of leading whitespace
     */
    q = r = string;
    while (*q && isspace(*q))
      {
        q++;
      }

    /*
     * if there was any, move the rest forwards
     */
    if (q != string)
      {
        while (q <= p)
          {
            *r++ = *q++;
          }
      }

    return string;
  }

  /*-- C++ string. */
  std::string miscutil::chomp_cpp(const std::string &s)
  {
    std::string sc = s;
    size_t p = 0;

    // strip leading whitespace.
    while(p < sc.length() && isspace(sc[p]))
      p++;
    sc.erase(0,p);

    // strip trailing whitespace.
    p = sc.length()-1;
    while(p > 0 && isspace(sc[p]))
      p--;
    sc.erase(p+1,sc.length()-p+1);

    return sc;
  }

  size_t miscutil::replace_in_string(std::string &str, const std::string &pattern,
                                     const std::string &repl)
  {
    size_t p = 0;
    while ((p = str.find(pattern,p)) != std::string::npos)
      {
        str.replace(p,pattern.size(),repl);
        p += repl.size(); // in case we're replacing with a string that contains the pattern itself.
      }
    return p;
  }

  bool miscutil::ci_equal(char ch1, char ch2)
  {
    return toupper((unsigned char)ch1) == toupper((unsigned char)ch2);
  }

  size_t miscutil::ci_find(const std::string& str1, const std::string& str2,
                           std::string::const_iterator &it)
  {
    std::string::const_iterator pos = std::search(it, str1.end(),
                                      str2.begin(), str2.end(),
                                      miscutil::ci_equal);
    if (pos == str1.end())
      return std::string::npos;
    else
      return pos - it;
  }

  size_t miscutil::ci_replace_in_string(std::string &str, const std::string &pattern,
                                        const std::string &repl)
  {
    size_t p1=0,p2=0;
    std::string::const_iterator it = str.begin();
    while ((p2 = miscutil::ci_find(str,pattern,it)) != std::string::npos)
      {
        str.replace(p1+p2,pattern.size(),repl);
        it = str.begin();
        it += p1+p2 + repl.size();
        p1 += p2 + repl.size();
      }
    return p1;
  }

  void miscutil::tokenize(const std::string &str,
                          std::vector<std::string> &tokens,
                          const std::string &delim)
  {

    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delim, 0);
    // Find first "non-delimiter".
    std::string::size_type pos = str.find_first_of(delim, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos)
      {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delim, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delim, lastPos);
      }
  }

  /* arithmetics. */

  bool miscutil::compare_d(const double &a, const double &b,
                           const double &epsilon)
  {
    return fabs(a-b) < epsilon;
  }

// Paul Hsieh's super fast hash function.
//
#include "stdint.h"
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
   || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
# define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
# define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
			  +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

  uint32_t miscutil::hash_string(const char * data, uint32_t len)
  {
    uint32_t hash = len, tmp;
    int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (; len > 0; len--)
      {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
      }

    /* Handle end cases */
    switch (rem)
      {
      case 3:
        hash += get16bits (data);
        hash ^= hash << 16;
        hash ^= data[sizeof (uint16_t)] << 18;
        hash += hash >> 11;
        break;
      case 2:
        hash += get16bits (data);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
      case 1:
        hash += *data;
        hash ^= hash << 10;
        hash += hash >> 1;
      }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
  }

  std::string miscutil::join_string_list(const std::string& delim, const std::list<std::string>& l)
  {
    std::string result;
    for (std::list<std::string>::const_iterator i = l.begin (), e = l.end (); i != e; ++i)
      {
        if (i != l.begin())
          result.append(delim);
        result.append(i->c_str());
      }
    return result;
  }

  std::string miscutil::join_string_list(const std::string& delim, const std::vector<std::string>& v)
  {
    std::string result;
    for (std::vector<std::string>::const_iterator i = v.begin (), e = v.end (); i != e; ++i)
      {
        if (i != v.begin())
          result.append(delim);
        result.append(i->c_str());
      }
    return result;
  }

} /* end of namespace. */

