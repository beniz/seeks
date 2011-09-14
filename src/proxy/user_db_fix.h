/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010-2011 Emmanuel Benazera, ebenazer@seeks-project.info
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef USER_DB_FIX_H
#define USER_DB_FIX_H

#include <string>
#include <vector>
#include <list>

namespace sp
{

  class user_db_fix
  {
    public:
      /**
       * \brief fix bug in hash generation on non 32bit machines, by converting the
       *        whole DB.
       * XXX: this function is expected to be removed soon in the future once the
       *      existing user DB can be considered safely converted.
       */
      static int fix_issue_169();

      /**
       * \brief fix bad chomped queries in records stored by 'query_capture' plugin.
       */
      static int fix_issue_263();

      /**
       * \brief fix trailing '/' in query captured records' URLs.
       */
      static int fix_issue_281();

      /**
       * \brief fix for non UTF-8 encoded queries.
       */
      static int fix_issue_154();

      /**
       * \brief fill up uri titles.
       */
      static int fill_up_uri_titles(const long &timeout,
                                    const std::vector<std::list<const char*>*> *headers);

      /**
       * \brief merge db into.
       */
      static int merge_with(const std::string &db_to_merge);

      /**
       * \brief fix case-sensitive queries.
       */
      static int fix_issue_575();
  };

} /* end of namespace. */

#endif
