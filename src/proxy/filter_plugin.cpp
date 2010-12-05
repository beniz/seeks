/*
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
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

#include "filter_plugin.h"

namespace sp
{
  // static members.
  std::vector<std::string> filter_plugin::init_always()
  {
    std::vector<std::string> alw;
    alw.push_back("*.*.*");  // DANGEROUS.
    return alw;
  }

  std::vector<std::string> filter_plugin::_always
  = filter_plugin::init_always();

  filter_plugin::filter_plugin(const std::vector<url_spec*> &pos_patterns,
                               const std::vector<url_spec*> &neg_patterns,
                               plugin *parent)
      : plugin_element(pos_patterns, neg_patterns, parent)
  {
  }

  filter_plugin::filter_plugin(const std::vector<std::string> &pos_patterns,
                               const std::vector<std::string> &neg_patterns,
                               plugin *parent)
      : plugin_element(pos_patterns, neg_patterns, parent)
  {
  }

  filter_plugin::filter_plugin(const char *filename,
                               plugin *parent)
      : plugin_element(filename, parent)
  {
  }

  filter_plugin::filter_plugin(plugin *parent)
      : plugin_element(filter_plugin::_always,
                       std::vector<std::string>(), parent)
  {
  }

  filter_plugin::filter_plugin(const std::vector<std::string> &pos_patterns,
                               const std::vector<std::string> &neg_patterns,
                               const char *code_filename,
                               const bool &pcrs, const bool &perl,
                               plugin *parent)
      : plugin_element(pos_patterns,neg_patterns,
                       code_filename,pcrs,perl,parent)
  {
  }

  filter_plugin:: filter_plugin(const char *pattern_filename,
                                const char *code_filename,
                                const bool &pcrs, const bool &perl,
                                plugin *parent)
      : plugin_element(pattern_filename,code_filename,
                       pcrs,perl,parent)
  {
  }

  filter_plugin::filter_plugin(const char *code_filename,
                               const bool &pcrs, const bool &perl,
                               plugin *parent)
      : plugin_element(filter_plugin::_always,
                       std::vector<std::string>(),
                       code_filename,pcrs,perl,parent)
  {
  }

  filter_plugin::~filter_plugin()
  {
  }


} /* end of namespace. */
