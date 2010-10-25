/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, ebenazer@seeks-project.info
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
     };
      
} /* end of namespace. */

#endif
