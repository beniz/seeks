/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, <ebenazer@seeks-project.info>
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

#include "feeds.h"
#include "websearch_configuration.h"
#include "errlog.h"

#include <iostream>

using sp::errlog;

namespace seeks_plugins
{

  /*- feed_parser -*/
  feed_parser::feed_parser(const std::string &name,
                           const std::string &url)
    :_name(name)
  {
    add_url(url);
  }

  feed_parser::feed_parser(const std::string &name)
    :_name(name)
  {
  }

  feed_parser::feed_parser(const feed_parser &fp)
    :_name(fp._name),_urls(fp._urls)
  {
  }

  feed_parser::feed_parser(const std::string &name,
                           const std::set<std::string> &urls)
    :_name(name),_urls(urls)
  {
  }

  feed_parser::feed_parser()
  {
  }

  feed_parser::~feed_parser()
  {
  }

  bool feed_parser::empty() const
  {
    return _urls.empty();
  }

  void feed_parser::add_url(const std::string &url)
  {
    _urls.insert(url);
  }

  std::string feed_parser::get_url() const
  {
    if (_urls.empty())
      {
        errlog::log_error(LOG_LEVEL_ERROR,"feed parser %s has no url attached",_name.c_str());
        return "";
      }
    if (_urls.size()>1)
      errlog::log_error(LOG_LEVEL_INFO,"getting top url from feed parser %s that applies to several urls",
                        _name.c_str());
    return (*_urls.begin());
  }

  size_t feed_parser::size() const
  {
    return _urls.size();
  }

  feed_parser feed_parser::diff(const feed_parser &fp) const
  {
    std::set<std::string> output_diff;
    std::set_symmetric_difference(_urls.begin(),_urls.end(),
                                  fp._urls.begin(),fp._urls.end(),
                                  std::inserter(output_diff,output_diff.begin()));
    feed_parser fps(_name,output_diff);
    return fps;
  }

  feed_parser feed_parser::diff_nosym(const feed_parser &fp) const
  {
    std::set<std::string> output_diff;
    std::set_difference(_urls.begin(),_urls.end(),
                        fp._urls.begin(),fp._urls.end(),
                        std::inserter(output_diff,output_diff.begin()));
    feed_parser fps(_name,output_diff);
    return fps;
  }

  feed_parser feed_parser::sunion(const feed_parser &fp) const
  {
    std::set<std::string> output_union;
    std::set_union(_urls.begin(),_urls.end(),
                   fp._urls.begin(),fp._urls.end(),
                   std::inserter(output_union,output_union.begin()));
    feed_parser fps(_name,output_union);
    return fps;
  }

  feed_parser feed_parser::inter(const feed_parser &fp) const
  {
    std::set<std::string> output_inter;
    std::set_intersection(_urls.begin(),_urls.end(),
                          fp._urls.begin(),fp._urls.end(),
                          std::inserter(output_inter,output_inter.begin()));
    feed_parser fps(_name,output_inter);
    return fps;
  }

  /*- feeds -*/
  feeds::feeds()
  {
  }

  feeds::feeds(std::set<feed_parser,feed_parser::lxn> &feedset)
  {
    std::set<feed_parser,feed_parser::lxn>::const_iterator it
    = feedset.begin();
    while(it!=feedset.end())
      {
        add_feed((*it));
        ++it;
      }
  }

  feeds::feeds(const std::string &name)
  {
    add_feed(feed_parser(name));
  }

  feeds::feeds(const std::string &name,
               const std::string &url)
  {
    add_feed(feed_parser(name,url));
  }

  feeds::feeds(const feeds &f)
  {
    std::set<feed_parser,feed_parser::lxn>::const_iterator it
    = f._feedset.begin();
    while(it!=f._feedset.end())
      {
        add_feed((*it));
        ++it;
      }
  }

  feeds::~feeds()
  {
  }

  bool feeds::add_feed(const std::string &name,
                       const std::string &url)
  {
    return add_feed(feed_parser(name,url));
  }

  bool feeds::add_feed(const std::string &name)
  {
    return add_feed(feed_parser(name));
  }

  bool feeds::add_feed(const feed_parser &f)
  {
    if (f.empty())
      return false;
    std::pair<std::set<feed_parser,feed_parser::lxn>::iterator,bool> ret
    = _feedset.insert(f);
    if (!ret.second)
      {
        feed_parser fp = find_feed(f._name);
        feed_parser fdiff = f.diff_nosym(fp);
        if (!fdiff.empty())
          {
            feed_parser funion = fp.sunion(f);
            if (funion.size() == f.size())
              remove_feed(f._name);
            ret.second = add_feed(funion);
          }
      }
    return ret.second;
  }

  bool feeds::add_feed(const std::string &name,
                       websearch_configuration *wconfig)
  {
    if (!wconfig)
      return add_feed(name);
    feed_parser fp(name);
    std::set<feed_parser,feed_parser::lxn>::iterator it
    = wconfig->_se_enabled._feedset.find(fp);
    if (it == wconfig->_se_enabled._feedset.end())
      {
        errlog::log_error(LOG_LEVEL_ERROR,"Cannot find feed parser %s in configuration",
                          name.c_str());
        return false;
      }
    // copy and feed_parser object.
    feed_parser fp_ptr((*it));
    return add_feed(fp_ptr);
  }

  bool feeds::remove_feed(const std::string &name)
  {
    feed_parser fp(name);
    std::set<feed_parser,feed_parser::lxn>::iterator it;
    if ((it = _feedset.find(fp))!=_feedset.end())
      {
        _feedset.erase(it);
        return true;
      }
    return false;
  }

  feed_parser feeds::find_feed(const std::string &name) const
  {
    feed_parser fp(name);
    std::set<feed_parser,feed_parser::lxn>::const_iterator it;
    it = _feedset.find(fp);
    if (it != _feedset.end())
      return (*it);
    else return feed_parser("");
  }

  bool feeds::has_feed(const std::string &name) const
  {
    feed_parser fp(name);
    std::set<feed_parser,feed_parser::lxn>::const_iterator it;
    it = _feedset.find(fp);
    if (it != _feedset.end())
      return true;
    else return false;
  }

  size_t feeds::size() const
  {
    size_t total_urls = 0;
    std::set<feed_parser,feed_parser::lxn>::const_iterator it
    = _feedset.begin();
    while(it!=_feedset.end())
      {
        total_urls += (*it).size();
        ++it;
      }
    return total_urls;
  }

  size_t feeds::count() const
  {
    return _feedset.size();
  }

  bool feeds::empty() const
  {
    return _feedset.empty();
  }

  feeds feeds::diff(const feeds &f) const
  {
    //feeds fds;
    /*const feeds *f1 = this, *f2 = &f;
    if (f1->count() < f2->count())
      {
    	f1 = &f;
    	f2 = this;
      }
    std::set<feed_parser,feed_parser::lxn>::const_iterator it
      = f1->_feedset.begin();
    while(it!=f1->_feedset.end())
      {
    	feed_parser fd = f2->find_feed((*it)._name);
    	if (!fd._name.empty())
    {
      feed_parser fdiff = (*it).diff(fd);
      fds.add_feed(fdiff);
    }
    	++it;
    	}*/
    std::set<feed_parser,feed_parser::lxn> output_diff;
    std::set_symmetric_difference(_feedset.begin(),_feedset.end(),
                                  f._feedset.begin(),f._feedset.end(),
                                  std::inserter(output_diff,output_diff.begin()),
                                  feed_parser::lxn());
    feeds fds(output_diff);

    // intersection + diff + add it up to fds.
    feeds common = inter_gen(f);
    std::vector<feed_parser> to_add;
    std::set<feed_parser,feed_parser::lxn>::const_iterator it
    = common._feedset.begin();
    while(it!=common._feedset.end())
      {
        feed_parser fdo = f.find_feed((*it)._name);
        feed_parser diffd = (*it).diff(fdo);
        if (!fds.add_feed(diffd))
          {
            // should never happen ?
            fds.remove_feed((*it)._name);
            to_add.push_back(diffd);
          }
        ++it;
      }
    for (size_t i=0; i<to_add.size(); i++)
      fds.add_feed(to_add.at(i));

    return fds;
  }

  feeds feeds::sunion(const feeds &f) const
  {
    std::set<feed_parser,feed_parser::lxn> output_union;
    std::set_union(_feedset.begin(),_feedset.end(),
                   f._feedset.begin(),f._feedset.end(),
                   std::inserter(output_union,output_union.begin()),
                   feed_parser::lxn());
    feeds fds(output_union);

    // intersection + union + add it up to fds.
    feeds common = inter_gen(f);
    std::vector<feed_parser> to_add;
    std::set<feed_parser,feed_parser::lxn>::const_iterator it
    = common._feedset.begin();
    while(it!=common._feedset.end())
      {
        feed_parser fdo = f.find_feed((*it)._name);
        feed_parser uniond = (*it).sunion(fdo);
        if (!fds.add_feed(uniond))
          {
            fds.remove_feed((*it)._name);
            to_add.push_back(uniond);
          }
        ++it;
      }
    for (size_t i=0; i<to_add.size(); i++)
      fds.add_feed(to_add.at(i));
    return fds;
  }

  feeds feeds::inter(const feeds &f) const
  {
    std::set<feed_parser,feed_parser::lxn> output_inter;
    std::set_intersection(_feedset.begin(),_feedset.end(),
                          f._feedset.begin(),f._feedset.end(),
                          std::inserter(output_inter,output_inter.begin()),
                          feed_parser::lxn());
    feeds fds(output_inter);

    // intersection + inter + add it up to output_inter.
    std::vector<feed_parser> to_add;
    std::set<feed_parser,feed_parser::lxn>::iterator it
    = fds._feedset.begin();
    while(it!=fds._feedset.end())
      {
        feed_parser fdo = f.find_feed((*it)._name);
        const feed_parser interd((*it).inter(fdo));
        if (interd.empty())
          {
            std::set<feed_parser,feed_parser::lxn>::iterator it1 = it;
            ++it;
            fds._feedset.erase(it1);
          }
        else
          {
            to_add.push_back(interd);
            ++it;
          }
      }
    for (size_t i=0; i<to_add.size(); i++)
      fds.add_feed(to_add.at(i));
    return fds;
  }

  feeds feeds::inter_gen(const feeds &f) const
  {
    std::set<feed_parser,feed_parser::lxn> output_inter;
    std::set_intersection(_feedset.begin(),_feedset.end(),
                          f._feedset.begin(),f._feedset.end(),
                          std::inserter(output_inter,output_inter.begin()),
                          feed_parser::lxn());
    feeds fds(output_inter);
    return fds;
  }

  bool feeds::equal(const feeds &f) const
  {
    /*feeds intersect = inter(f);
      return (intersect.size() == f.size());*/
    if (size() != f.size()
        || count() != f.count())
      return false;
    feeds intersect = inter(f);
    if (intersect.size() == f.size()
        && intersect.size() == size()
        && intersect.count() == f.count()
        && intersect.count() == count())
      return true;
    else return false;
  }

} /* end of namespace. */
