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

#ifndef DB_RECORD_H
#define DB_RECORD_H

#include "db_record_msg.pb.h"

#include <time.h>
#include <string>

#include <ostream>

namespace sp
{

  class db_record
  {
    public:
      /**
       * \brief constructor with time and plugin name.
       */
      db_record(const time_t &creation_time,
                const std::string &plugin_name);

      /**
       * \brief constructor with time set to 'now'.
       */
      db_record(const std::string &plugin_name);

      /**
       * \brief constructor, empty, for storing deserialized object.
       */
      db_record();

      /**
       * destructor.
       */
      virtual ~db_record();

      /**
       * operator.
       */
      std::ostream& operator<<(std::ostream &output) const;

      /**
       * \brief update creation time.
       */
      void update_creation_time();

      /**
       * \brief serializes the object.
       */
      virtual int serialize(std::string &msg) const;

      /**
       * \brief fill up the object from a serialized version of it.
       */
      virtual int deserialize(const std::string &msg);

      /**
       * \brief base serialization.
       */
      int serialize_base_record(std::string &msg) const;

      /**
       * \brief base deserialization.
       */
      int deserialize_base_record(const std::string &msg);

      /**
       * \brief create base message object, for serialization.
       */
      void create_base_record(sp::db::record &r) const;

      /**
       * \brief reads base message object, after deserialization.
       */
      void read_base_record(const sp::db::record &r);

      /**
       * \brief merges two records.
       * @return < 0 if error, 0 otherwise (-2 is reserved error).
       */
      virtual int merge_with(const db_record &dbr)
      {
        return 0;
      };
      
      /**
       * \brief free to fill function for generic access and operation over
       *        the db records.
       */
      virtual int do_smthg(void *data) 
      {
	return 0;
      };
      
      /**
       * prints the record out.
       */
      std::ostream& print(std::ostream &output) const;

      /**
       * export the record.
       */

      virtual std::ostream& json_export_record(const std::string &msg, std::ostream &output) const;
      virtual std::ostream& xml_export_record(const std::string &msg, std::ostream &output) const;
      virtual std::ostream& text_export_record(const std::string &msg, std::ostream &output) const;

    public:
      time_t _creation_time;
      std::string _plugin_name;
  };

} /* end of namespace. */

#endif
