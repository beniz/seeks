/**
 * The Locality Sensitive Hashing (LSH) library is part of the SEEKS project and
 * does provide several locality sensitive hashing schemes for pattern matching over
 * continuous and discrete spaces.
 * Copyright (C) 2009 Emmanuel Benazera, juban@free.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#ifndef MRF_H
#define MRF_H

#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <ostream>

namespace lsh
{
  class str_chain
  {
  public:
    str_chain(const std::string &str,
	      const int &radius);
    
    str_chain(const str_chain &sc);

    ~str_chain() {};
    
    void add_token(const std::string &str);
    
    void incr_radius() { _radius += 1; };
    void decr_radius() { _radius -= 1; };
    
    std::vector<std::string> get_chain() const {return _chain; }
    std::vector<std::string>& get_chain_noconst() { return _chain; }
    int get_radius() const { return _radius; }
    size_t size() const { return _chain.size(); }
    std::string at(const int &i) const { return _chain.at(i); }
    bool has_skip() const { return _skip; }
    void set_skip() { _skip = true; }
    str_chain rank_alpha() const;
    
    std::ostream& print(std::ostream &output) const;

  private:
    std::vector<std::string> _chain;
    int _radius;
    bool _skip;
  };

  class mrf
  {
  public:
    static void tokenize(const std::string &str,
			 std::vector<std::string> &tokens,
			 const std::string &delim);

    // straight hash of a query string.
    static uint32_t mrf_single_feature(const std::string &str);
     
    static void mrf_features_query(const std::string &str,
				   std::vector<uint32_t> &features,
				   const int &min_radius,
				   const int &max_radius);
    
     static void mrf_features(std::vector<std::string> &tokens,
			      std::vector<uint32_t> &features,
			      const int &step);
     
    static void mrf_build(const std::vector<std::string> &tokens,
			  std::vector<uint32_t> &features,
			  const int &min_radius,
			  const int &max_radius,
			  const int &gen_radius);

    static void mrf_build(const std::vector<std::string> &tokens,
			 int &tok,
			 std::queue<str_chain> &chains,
			  std::vector<uint32_t> &features,
			  const int &min_radius,
			  const int &max_radius,
			  const int &gen_radius);

    /* hashing. */
    static uint32_t mrf_hash(const str_chain &chain);

    static uint32_t mrf_hash(const std::vector<std::string> &tokens);
     
    static uint32_t SuperFastHash(const char *data, uint32_t len);

    /* measuring similarity. */
    static int hash_compare(const uint32_t &a, const uint32_t &b);
    
    static double radiance(const std::string &query1,
			   const std::string &query2);

    static double radiance(const std::string &query1, 
			   const std::string &query2,
			   const int &min_radius,
			   const int &max_radius);
    
    static double radiance(const std::vector<uint32_t> &sorted_features1,
			   const std::vector<uint32_t> &sorted_features2,
			   uint32_t &common_features);

  public:
    static std::string _default_delims;
   private:
    static uint32_t _skip_token;
  public:
    static uint32_t _window_length_default;
  private:
    static uint32_t _window_length;
    static uint32_t _hctable[];
   public:
    static double _epsilon;
  };

} /* end of namespace. */

#endif
