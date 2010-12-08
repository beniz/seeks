/**
 * Copyright   :  Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
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

#ifndef MISCUTIL_H
#define MISCUTIL_H

#include "sp_err.h"
#include "stl_hash.h"
#include <inttypes.h>
#include <list>
#include <string>
#include <sstream>

#ifndef HAVE_STRNDUP
char *strndup(char *str, size_t len);
#endif

namespace sp
{

  class miscutil
  {
    public:

#if !defined(HAVE_STRLCPY)
      static size_t seeks_strlcpy(char *destination, const char *source, size_t size);
#define strlcpy miscutil::seeks_strlcpy
#define USE_SEEKS_STRLCPY 1
#define HAVE_STRLCPY 1
#endif /* ndef HAVE_STRLCPY*/

#ifndef HAVE_STRLCAT
      static size_t seeks_strlcat(char *destination, const char *source, size_t size);
#define strlcat miscutil::seeks_strlcat
#endif /* ndef HAVE_STRLCAT */

      static int strcmpic(const char *s1, const char *s2);
      static int strncmpic(const char *s1, const char *s2, size_t n);

#define strcmpic miscutil::strcmpic
#define strncmpic miscutil::strncmpic

      /* operations on maps. */
      static sp_err add_map_entry(hash_map<const char*,const char*,hash<const char*>,eqstr> *the_map,
                                  const char *name, int name_needs_copying,
                                  const char *value, int value_needs_copying);
      static sp_err unmap(hash_map<const char*,const char*,hash<const char*>,eqstr> *the_map,
                          const char *name);
      static void free_map(hash_map<const char*,const char*,hash<const char*>,eqstr> *&the_map);
      static hash_map<const char*,const char*,hash<const char*>,eqstr>* copy_map(const hash_map<const char*,const char*,hash<const char*>,eqstr> *the_map);
      static const char* lookup(const hash_map<const char*,const char*,hash<const char*>,eqstr> *the_map,
                                const char* name);

      /* operations on lists. */
      static sp_err enlist(std::list<const char*> *the_list, const char *str);
      static sp_err enlist_first(std::list<const char*> *the_list, const char *str);
      static sp_err enlist_unique(std::list<const char*> *the_list, const char *str,
                                  size_t num_significant_chars);
      static sp_err enlist_unique_header(std::list<const char*> *the_list,
                                         const char *name, const char *value);
      static int list_remove_item(std::list<const char*> *the_list, const char *str);
      static int list_remove_list(std::list<const char*> *dest, const std::list<const char*> *src);
      static void list_remove_all(std::list<const char*> *the_list);
      static char* list_to_text(const std::list<const char*> *the_list);
      static int list_contains_item(const std::list<const char*> *the_list,
                                    const char *str);
      static int list_duplicate(std::list<const char*> *dest,
                                const std::list<const char*> *src);
      static int list_append_list_unique(std::list<const char*> *dest,
                                         const std::list<const char*> *src);

      /*-- C string --*/
      /* a function to split a string at specified delimiters. */
      static int ssplit(char  *str, const char *delim, char *vec[], int vec_len,
                        int dont_save_empty_fields, int ignore_leading);
      static sp_err string_append(char **target_string, const char *text_to_append); // compatibility only.
      // should use std::string instead...

      static sp_err string_join(char **target_string, char *text_to_append);
      static char* string_toupper(const char *string);
      static char* bindup(const char *string, size_t len);
      static char* chomp(char *string);

      /*-- C++ string --*/
      template <class T>
      static std::string to_string (const T& t)
      {
        std::stringstream ss;
        ss << t;
        return ss.str();
      };

      static std::string chomp_cpp(std::string &s);

      /**
       * \brief replaces pattern in str with repl.
       * returns a positive value if changes were made to the argument string.
       */
      static size_t replace_in_string(std::string &str, const std::string &pattern,
                                      const std::string &repl);

      static void tokenize(const std::string &str,
                           std::vector<std::string> &tokens,
                           const std::string &delim);

      static bool ci_equal(char ch1, char ch2);

      static size_t ci_find(const std::string& str1, const std::string& str2,
                            std::string::const_iterator &it);

      static size_t ci_replace_in_string(std::string &str, const std::string &pattern,
                                         const std::string &repl);

      /* others. */
      static uint32_t hash_string(const char *data, uint32_t len);
      static std::string join_string_list(const std::string& delim, const std::list<std::string>& l);
  };

} /* end of namespace. */

#endif
