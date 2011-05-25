/**
 * The Locality Sensitive Hashing (LSH) library is part of the SEEKS project and
 * does provide several locality sensitive hashing schemes for pattern matching over
 * continuous and discrete spaces.
 * Copyright (C) 2009, 2010 Emmanuel Benazera, juban@free.fr
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

#ifndef MRF_H
#define MRF_H

#include "stl_hash.h"
#include "rmd160.h"
#include "DHTKey.h"

#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>
#include <queue>
#include <ostream>
#include <iostream>

using dht::DHTKey;

namespace lsh
{

  class f160r
  {
    public:
      f160r(char *feat, const int &radius)
        :_feat(feat),_radius(radius)
      {};
      ~f160r() {};

    public:
      char *_feat;
      int _radius;
  };

  uint32_t SuperFastHash(const char *data, uint32_t len);

  template<typename feat>
  void mrf_hash_m(const char *data, uint32_t len, feat &f)
  { };

  template<>
  void mrf_hash_m<uint32_t>(const char *data, uint32_t len, uint32_t &f);

  template<>
  void mrf_hash_m<f160r>(const char *data, uint32_t len, f160r &f);

  template<typename feat>
  void set_skip_token(feat &f)
  {};

  template<>
  void set_skip_token<uint32_t>(uint32_t &f);

  template<>
  void set_skip_token<f160r>(f160r &f);

  /*- str_chain -*/
  class str_chain
  {
    public:
      str_chain();

      str_chain(const std::string &str,
                const int &radius);

      str_chain(const std::string &str,
                const int &radius,
                const bool &tokenize);

      str_chain(const str_chain &sc);

      ~str_chain() {};

      void add_token(const std::string &str);
      void remove_token(const size_t &i);
      void check_skip();

      void incr_radius()
      {
        _radius += 1;
      };
      void decr_radius()
      {
        _radius -= 1;
      };
      std::vector<std::string> get_chain() const
      {
        return _chain;
      }
      std::vector<std::string>& get_chain_noconst()
      {
        return _chain;
      }
      int get_radius() const
      {
        return _radius;
      }
      size_t size() const
      {
        return _chain.size();
      }
      bool empty() const
      {
        return _chain.empty();
      }
      std::string at(const int &i) const
      {
        return _chain.at(i);
      }
      bool has_skip() const
      {
        return _skip;
      }
      void set_skip()
      {
        _skip = true;
      }
      str_chain rank_alpha() const;
      str_chain intersect(const str_chain &stc) const;
      uint32_t intersect_size(const str_chain &stc) const;
      std::string print_str() const;
      std::ostream& print(std::ostream &output) const;

    private:
      std::vector<std::string> _chain;
      int _radius;
      bool _skip;
  };

  template<typename feat>
  feat mrf_hash_c(const str_chain &chain)
  {
  };

  template<>
  uint32_t mrf_hash_c<uint32_t>(const str_chain &chain);

  template<>
  f160r mrf_hash_c<f160r>(const str_chain &chain);

  class mrf
  {
    public:
      static void tokenize(const std::string &str,
                           std::vector<std::string> &tokens,
                           const std::string &delim);

      static void unique_features(std::vector<uint32_t> &sorted_features);

      // straight hash of a query string.
      static uint32_t mrf_single_feature(const std::string &str);

      template<typename feat>
      static void mrf_features_query(const std::string &str,
                                     std::vector<feat> &features,
                                     const int &min_radius,
                                     const int &max_radius,
                                     const uint32_t &window_length_default=5)
      {
        std::vector<std::string> tokens;
        mrf::tokenize(str,tokens,mrf::_default_delims);
        uint32_t window_length = std::min((int)tokens.size(),(int)window_length_default);

        int gen_radius = 0;
        while (!tokens.empty())
          {
            mrf::mrf_build(tokens,features,
                           min_radius,max_radius,
                           gen_radius, window_length);
            tokens.erase(tokens.begin());
            ++gen_radius;
          }
      }

      static void mrf_features(std::vector<std::string> &tokens,
                               std::vector<uint32_t> &features,
                               const int &radius,
                               const int &step,
                               const uint32_t &window_length_default=5);

      static void tokenize_and_mrf_features(const std::string &str,
                                            const std::string &delim,
                                            std::vector<uint32_t> &features,
                                            const int &radius,
                                            const int &step,
                                            const uint32_t &window_length_default=5);

      template<typename feat>
      static void mrf_build(const std::vector<std::string> &tokens,
                            std::vector<feat> &features,
                            const int &min_radius,
                            const int &max_radius,
                            const int &gen_radius,
                            const uint32_t &window_length)
      {
        int tok = 0;
        std::queue<str_chain> chains;
        mrf::mrf_build<feat>(tokens,tok,chains,features,
                             min_radius,max_radius,gen_radius,window_length);
      }

      template<typename feat>
      static void mrf_build(const std::vector<std::string> &tokens,
                            int &tok,
                            std::queue<str_chain> &chains,
                            std::vector<feat> &features,
                            const int &min_radius,
                            const int &max_radius,
                            const int &gen_radius,
                            const uint32_t &window_length)
      {
        if (chains.empty())
          {
            int radius_chain = window_length - 1;
            str_chain chain(tokens.at(tok),radius_chain);

#ifdef DEBUG
            std::cout << "gen_radius: " << gen_radius << std::endl;
            std::cout << "radius_chain: " << radius_chain << std::endl;
#endif

            if (radius_chain >= min_radius
                && radius_chain <= max_radius)
              {
                //hash chain and add it to features set.
                feat h = mrf_hash_c<feat>(chain);
                features.push_back(h);

#ifdef DEBUG
                //debug
                std::cout << tokens.at(tok) << std::endl;
                //std::cout << std::hex << h << std::endl;
                std::cout << "radius: " << radius_chain << std::endl;
                std::cout << std::endl;
                //debug
#endif
              }
            chains.push(chain);

            mrf::mrf_build<feat>(tokens,tok,chains,features,
                                 min_radius,max_radius,gen_radius,window_length);
          }
        else
          {
            ++tok;
            std::queue<str_chain> nchains;
            while (!chains.empty())
              {
                str_chain chain = chains.front();
                chains.pop();

                if (chain.size() <  std::min((uint32_t)tokens.size(),window_length))
                  {
                    // first generated chain: add a token.
                    str_chain chain1(chain);
                    chain1.add_token(tokens.at(tok));
                    chain1.decr_radius();

                    if (chain1.get_radius() >= min_radius
                        && chain1.get_radius() <= max_radius)
                      {
                        // hash it and add it to features.
                        feat h = mrf_hash_c<feat>(chain1);
                        features.push_back(h);

#ifdef DEBUG
                        //debug
                        chain1.print(std::cout);
                        //std::cout << std::hex << h << std::endl;
                        std::cout << "radius: " << chain1.get_radius() << std::endl;
                        std::cout << std::endl;
                        //debug
#endif
                      }

                    // second generated chain: add a 'skip' token.
                    str_chain chain2 = chain;
                    chain2.add_token("<skip>");

                    nchains.push(chain1);
                    nchains.push(chain2);
                  }
              }

            if (!nchains.empty())
              mrf::mrf_build<feat>(tokens,tok,nchains,features,
                                   min_radius,max_radius,gen_radius,window_length);
          }
      }

      /*- tf/idf -*/
      static void tokenize_and_mrf_features(const std::string &str,
                                            const std::string &delim,
                                            hash_map<uint32_t,float,id_hash_uint> &wfeatures,
                                            hash_map<uint32_t,std::string,id_hash_uint> *bow,
                                            const int &radius,
                                            const int &step,
                                            const uint32_t &window_length_default=5,
                                            const std::string &lang="");

      static void mrf_build(const std::vector<std::string> &tokens,
                            hash_map<uint32_t,float,id_hash_uint> &wfeatures,
                            hash_map<uint32_t,std::string,id_hash_uint> *bow,
                            const int &min_radius,
                            const int &max_radius,
                            const int &gen_radius,
                            const uint32_t &window_length_default);

      static void mrf_build(const std::vector<std::string> &tokens,
                            int &tok,
                            std::queue<str_chain> &chains,
                            hash_map<uint32_t,float,id_hash_uint> &wfeatures,
                            hash_map<uint32_t,std::string,id_hash_uint> *bow,
                            const int &min_radius,
                            const int &max_radius,
                            const int &gen_radius,
                            const uint32_t &window_length);

      static void compute_tf_idf(std::vector<hash_map<uint32_t,float,id_hash_uint>*> &bags);

      /* hashing. */
      static uint32_t mrf_hash(const std::string &token);

      /* measuring similarity. */
      static int hash_compare(const uint32_t &a, const uint32_t &b);

      static double radiance(const std::string &query1,
                             const std::string &query2,
                             const uint32_t &window_length_default = 5);

      static double radiance(const std::string &query1,
                             const std::string &query2,
                             const int &min_radius,
                             const int &max_radius);

      static double radiance(const std::vector<uint32_t> &sorted_features1,
                             const std::vector<uint32_t> &sorted_features2,
                             uint32_t &common_features);

      static double distance(const std::vector<uint32_t> &sorted_features1,
                             const std::vector<uint32_t> &sorted_features2,
                             uint32_t &common_features);


    public:
      static std::string _default_delims;
    public:
      static uint32_t _skip_token_32;
      static char *_skip_token_160;
    public:
      static uint32_t _hctable[];
    public:
      static double _epsilon;
    private:
      static float _feature_weights[];
      static short _array_size;

      static std::string _stop_word_token;
  };

} /* end of namespace. */

#endif
