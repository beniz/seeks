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

#include "db_record.h"
#include "errlog.h"

#include <sys/time.h>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include "protobuf_export_format/json_format.h"
#include "protobuf_export_format/xml_format.h"

#include <iostream>
#include <fstream>

namespace sp
{
  db_record::db_record(const time_t &creation_time,
                       const std::string &plugin_name)
    :_creation_time(creation_time),_plugin_name(plugin_name)
  {
  }

  db_record::db_record(const std::string &plugin_name)
    :_plugin_name(plugin_name)
  {
    update_creation_time();
  }

  db_record::db_record()
    :_creation_time(0)
  {
  }

  db_record::~db_record()
  {
  }

  void db_record::update_creation_time()
  {
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    _creation_time = tv_now.tv_sec;
  }

  int db_record::serialize(std::string &msg) const
  {
    return serialize_base_record(msg);
  }

  int db_record::deserialize(const std::string &msg)
  {
    return deserialize_base_record(msg);
  }

  // XXX: we do not use compression for this amount of data.
  int db_record::serialize_compressed(std::string &msg) const
  {
    return serialize_base_record(msg);
  }

  // XXX: we do not usecompression forthis amount of data.
  int db_record::deserialize_compressed(const std::string &msg)
  {
    return deserialize_base_record(msg);
  }

  int db_record::serialize_base_record(std::string &msg) const
  {
    sp::db::record r;
    create_base_record(r);
    if (!r.SerializeToString(&msg))
      {
        errlog::log_error(LOG_LEVEL_ERROR,"Error serializing user db_record");
        return 1; // error.
      }
    else return 0;
  }

  int db_record::deserialize_base_record(const std::string &msg)
  {
    sp::db::record r;
    if (!r.ParseFromString(msg))
      {
        errlog::log_error(LOG_LEVEL_ERROR,"Error deserializing user db_record");
        return 1; // error.
      }
    read_base_record(r);
    return 0;
  }

  void db_record::create_base_record(sp::db::record &r) const
  {
    r.set_creation_time(_creation_time);
    r.set_plugin_name(_plugin_name);
  }

  void db_record::read_base_record(const sp::db::record &r)
  {
    _creation_time = r.creation_time();
    _plugin_name = r.plugin_name();
  }

  std::ostream& db_record::print(std::ostream &output) const
  {
    std::string msg;
    // Call the virtual one
    serialize(msg);
    return text_export_record(msg, output);
  }

  std::ostream& db_record::operator<<(std::ostream &output) const
  {
    return print(output);
  }

  /**
   * XXX: This fonction use a hack version of class google::protobuf::TextFormat
   *      to produce JSON/XML/TEXT output
   *      All files copied and modified from protobuf are under protobuf_export_format/
   *      Parser are under sp::protobuf_format namespace and depend on protobuf
   *      To build a other export format duplicate json file and mod key, value decoration
   */
  std::ostream& db_record::json_export_record(const std::string &msg, std::ostream &output,
					      const bool &single_line_mode,
					      const bool &utf8_string_escape) const
  {
    sp::db::record r;
    r.ParseFromString(msg);
    google::protobuf::io::ZeroCopyOutputStream* fos = new google::protobuf::io::OstreamOutputStream(&output, 0);
    sp::protobuf_format::JSONFormat::Printer p;
    p.SetSingleLineMode(single_line_mode);
    p.SetUseUtf8StringEscaping(utf8_string_escape);
    p.Print(r,fos);
    delete fos;
    return output;
  }

  std::ostream& db_record::xml_export_record(const std::string &msg, std::ostream &output) const
  {
    sp::db::record r;
    r.ParseFromString(msg);
    google::protobuf::io::ZeroCopyOutputStream* fos = new google::protobuf::io::OstreamOutputStream(&output, 0);
    sp::protobuf_format::XMLFormat::Print(r, fos);
    delete fos;
    return output;
  }

  std::ostream& db_record::text_export_record(const std::string &msg, std::ostream &output) const
  {
    sp::db::record r;
    r.ParseFromString(msg);
    google::protobuf::io::ZeroCopyOutputStream* fos = new google::protobuf::io::OstreamOutputStream(&output, 0);
    google::protobuf::TextFormat::Print(r, fos);
    delete fos;
    return output;
  }

} /* end of namespace. */
