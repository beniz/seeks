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

#include "mrf.h"
#include "miscutil.h"
#include "lsh_configuration.h"

#include <algorithm>
#include <iostream>
#include <cctype>

#include <math.h>
#include <assert.h>

using sp::miscutil;

//#define DEBUG

namespace lsh
{

  template<>
  void mrf_hash_m<uint32_t>(const char *data, uint32_t len, uint32_t &f)
  {
    f = SuperFastHash(data,len);
  };

  template<>
  void mrf_hash_m<char*>(const char *data, uint32_t len, char *&f)
  {
    //debug
    //std::cout << "data: " << data << std::endl;
    //debug

    byte *hashcode = NULL;
    DHTKey::RMD((byte*)data,hashcode);
    f = (char*)hashcode;

    //debug
    /* for (unsigned int j=0; j<20; j++)
      printf("%02x",hashcode[j]);
    std::cout << std::endl; */
    //debug
  };

  uint32_t skip_token_32 = 0xDEADBEEF;
  const char *skip_token_160 = ""; //TODO.

  template<>
  void set_skip_token<uint32_t>(uint32_t &f)
  {
    f = skip_token_32;
  };

  template<>
  uint32_t mrf_hash_c<uint32_t>(const str_chain &chain)
  {
    // rank chains which do not contain any skipped token (i.e. no
    // order preservation).
    str_chain cchain(chain);
    if (!chain.has_skip())
      cchain = chain.rank_alpha();

    uint32_t h = 0;
    size_t csize = std::min(10,(int)cchain.size());  // beware: hctable may be too small...
    for (size_t i=0; i<csize; i++)
      {
        std::string token = cchain.at(i);
        uint32_t hashed_token;
        set_skip_token<uint32_t>(hashed_token);
        if (token != "<skip>")
          {
            mrf_hash_m<uint32_t>(token.c_str(),token.size(),hashed_token);
          }
        //hashed_token= mrf::SuperFastHash(token.c_str(),token.size());

        // #ifdef DEBUG
        //debug
        //std::cout << "hashed token: " << hashed_token << std::endl;
        //debug
        // #endif

        h += hashed_token * mrf::_hctable[i]; // beware...
      }
    return h;
  };

  template<>
  f160r mrf_hash_c<f160r>(const str_chain &chain)
  {
    // rank chains which do not contain any skipped token (i.e.
    // no order preservation.
    str_chain cchain(chain);
    if (!chain.has_skip())
      cchain = chain.rank_alpha();

    std::string fchain;
    size_t csize = cchain.size();
    for (size_t i=0; i<csize; i++)
      {
        fchain += cchain.at(i);
        if (i!=csize-1)
          fchain += " "; // space as a pre-hashing delimiter of words.
      }
    char *hashed_token = NULL;
    mrf_hash_m<char*>(fchain.c_str(),fchain.size(),hashed_token);
    f160r fr(hashed_token,chain.get_radius());
    return fr;
  };

  /*-- str_chain --*/
  str_chain::str_chain(const std::string &str,
                       const int &radius)
      :_radius(radius),_skip(false)
  {
    add_token(str);
    if (str == "<skip>")
      _skip = true;
  }

  str_chain::str_chain(const str_chain &sc)
      :_chain(sc.get_chain()),_radius(sc.get_radius()),
      _skip(sc.has_skip())
  {
  }

  void str_chain::add_token(const std::string &str)
  {
    _chain.push_back(str);
  }

  str_chain str_chain::rank_alpha() const
  {
    str_chain cchain = *this;

#ifdef DEBUG
    /* std::cout << "cchain: ";
     cchain.print(std::cout); */
#endif

    std::sort(cchain.get_chain_noconst().begin(),cchain.get_chain_noconst().end());

#ifdef DEBUG
    /* std::cout << "sorted chain: ";
     cchain.print(std::cout); */
#endif

    return cchain;
  }

  std::ostream& str_chain::print(std::ostream &output) const
  {
    for (size_t i=0; i<_chain.size(); i++)
      output << _chain.at(i) << " ";
    output << std::endl;
    return output;
  }

  /*-- mrf --*/

  std::string mrf::_default_delims = "\n\t\f\r ,.;:`'!?)(-|><^·&\"\\/{}#$–"; // replaced by those in lsh-config when the library is initialized.
  uint32_t mrf::_hctable[] = { 1, 3, 5, 11, 23, 47, 97, 197, 397, 797 };
  double mrf::_epsilon = 1e-6;  // infinitesimal.
  //float mrf::_feature_weights[] = {16384, 8192, 4096, 2048, 1024, 256, 64, 16, 4, 1};
  short mrf::_array_size = 10;
  float mrf::_feature_weights[] = {1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0};
  std::string mrf::_stop_word_token = "S";

  void mrf::tokenize(const std::string &str,
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

        // debug
        /* if (lastPos != std::string::npos)
          assert(lastPos <= str.size()); */
        // debug

        // Find next "non-delimiter"
        pos = str.find_first_of(delim, lastPos);
      }
  }

  void mrf::unique_features(std::vector<uint32_t> &sorted_features)
  {
    if (sorted_features.size() == 1)
      return;

    std::vector<uint32_t> unique_features;
    std::vector<uint32_t>::const_iterator vit = sorted_features.begin();
    while (vit!=sorted_features.end())
      {
        uint32_t feat = (*vit);
        unique_features.push_back(feat);

        ++vit;
        while (vit != sorted_features.end()
               && ((*vit) == feat))
          {
            ++vit;
          }
      }

    sorted_features.clear();
    sorted_features = unique_features;
  }

  uint32_t mrf::mrf_single_feature(const std::string &str)
  {
    std::vector<std::string> tokens;
    mrf::tokenize(str,tokens,mrf::_default_delims);
    return mrf::mrf_hash(tokens);
  }

  uint32_t mrf::mrf_single_feature(const std::string &str,
                                   const std::string &delims)
  {
    std::vector<std::string> tokens;
    mrf::tokenize(str,tokens,delims);
    return mrf::mrf_hash(tokens);
  }

  void mrf::mrf_features(std::vector<std::string> &tokens,
                         std::vector<uint32_t> &features,
                         const int &max_radius,
                         const int &step,
                         const uint32_t &window_length_default)
  {
    while (!tokens.empty())
      {
        mrf::mrf_build(tokens,features,
                       0, max_radius,0,window_length_default);
        if ((int)tokens.size()>step)
          tokens.erase(tokens.begin(),tokens.begin()+step);
        else tokens.clear();
      }
    std::sort(features.begin(),features.end());
  }

  void mrf::tokenize_and_mrf_features(const std::string &str,
                                      const std::string &delim,
                                      std::vector<uint32_t> &features,
                                      const int &max_radius,
                                      const int &step,
                                      const uint32_t &window_length_default)
  {
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delim, 0);
    // Find first "non-delimiter".
    std::string::size_type pos = str.find_first_of(delim, lastPos);
    std::vector<std::string> tokens;

    while (true)
      {
        if ((int)tokens.size()>step)
          {
            tokens.erase(tokens.begin(),tokens.begin()+step);
          }
        else tokens.clear();

        while ((std::string::npos != pos || std::string::npos != lastPos)
               && tokens.size() < window_length_default)
          {
            // Found a token, add it to the vector.
            tokens.push_back(str.substr(lastPos, pos - lastPos));
            // Skip the delimiters.
            lastPos = str.find_first_not_of(delim,pos);
            // Find next "non-delimiter".
            pos = str.find_first_of(delim, lastPos);
          }

        if (tokens.empty() || tokens.size()<window_length_default-max_radius)
          break;

        // produce the features out of the tokens.
        mrf::mrf_build(tokens,features,0,max_radius,0,window_length_default);
      }
    std::sort(features.begin(),features.end());
  }

  /*- tf / idf. -*/
  void mrf::tokenize_and_mrf_features(const std::string &str,
                                      const std::string &delim,
                                      hash_map<uint32_t,float,id_hash_uint> &wfeatures,
                                      hash_map<uint32_t,std::string,id_hash_uint> *bow,
                                      const int &max_radius,
                                      const int &step,
                                      const uint32_t &window_length_default,
                                      const std::string &lang)
  {
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delim, 0);
    // Find first "non-delimiter".
    std::string::size_type pos = str.find_first_of(delim, lastPos);
    std::vector<std::string> tokens;

    stopwordlist *swl = NULL;
    if (!lang.empty())
      swl = lsh_configuration::_config->get_wordlist(lang);

    while (true)
      {
        if ((int)tokens.size()>step)
          {
            tokens.erase(tokens.begin(),tokens.begin()+step);
          }
        else tokens.clear();

        while ((std::string::npos != pos || std::string::npos != lastPos)
               && tokens.size() < window_length_default)
          {
            // Found a token, chomp it.
            std::string token = str.substr(lastPos, pos - lastPos);
            size_t cpos = 0;
            while (cpos<token.size() && isspace(token[cpos]))
              {
                ++cpos;
              }
            if (cpos>0)
              token = token.substr(cpos);

            // bypasse digit-beginning tokens, look into stop word list if available, & add to token list.
            if (!token.empty() && !isdigit(token.c_str()[0]))
              {
                std::transform(token.begin(),token.end(),token.begin(),tolower);
                bool sw = false;
                if (swl)
                  {
                    sw = swl->has_word(token);
                  }
                if (!sw)
                  {
                    tokens.push_back(token);
                  }
                else if (window_length_default > 1)
                  tokens.push_back(mrf::_stop_word_token);
              }
            // Skip the delimiters.
            lastPos = str.find_first_not_of(delim,pos);
            // Find next "non-delimiter".
            pos = str.find_first_of(delim, lastPos);
          }

        if (tokens.empty() || tokens.size()<window_length_default-max_radius)
          break;

        // produce the features out of the tokens.
        mrf::mrf_build(tokens,wfeatures,bow,0,max_radius,0,
                       window_length_default);
      }
  }

  void mrf::mrf_build(const std::vector<std::string> &tokens,
                      hash_map<uint32_t,float,id_hash_uint> &wfeatures,
                      hash_map<uint32_t,std::string,id_hash_uint> *bow,
                      const int &min_radius,
                      const int &max_radius,
                      const int &gen_radius,
                      const uint32_t &window_length_default)
  {
    int tok = 0;
    std::queue<str_chain> chains;
    mrf::mrf_build(tokens,tok,chains,wfeatures,bow,
                   min_radius,max_radius,gen_radius,window_length_default);
  }

  void mrf::mrf_build(const std::vector<std::string> &tokens,
                      int &tok,
                      std::queue<str_chain> &chains,
                      hash_map<uint32_t,float,id_hash_uint> &wfeatures,
                      hash_map<uint32_t,std::string,id_hash_uint> *bow,
                      const int &min_radius, const int &max_radius,
                      const int &gen_radius, const uint32_t &window_length)
  {
    short offset = mrf::_array_size-window_length;
    if (chains.empty())
      {
        int radius_chain = window_length - std::max(1,(int)(window_length-tokens.size()));
        str_chain chain(tokens.at(tok),radius_chain);

#ifdef DEBUG
        std::cout << "gen_radius: " << gen_radius << std::endl;
        std::cout << "radius_chain: " << radius_chain << std::endl;
#endif

        if (radius_chain >= min_radius
            && radius_chain <= max_radius)
          {
            //if (chain.size()>1 || chain.at(0) != mrf::_stop_word_token)
            if (chain.size() > 1 || chain.at(0).size() > 1)
              {
                //hash chain and add it to features set.
                uint32_t h = mrf_hash_c<uint32_t>(chain);

#ifdef DEBUG
                std::cout << "h: " << h << std::endl;
                chain.print(std::cout);
                std::cout << "radius: " << chain.get_radius() << std::endl;
#endif

                // count features, increment total, and sets an exponential weight.
                short rad_offset = chain.get_radius()+offset;
                hash_map<uint32_t,float,id_hash_uint>::iterator hit;
                if ((hit=wfeatures.find(h))!=wfeatures.end())
                  (*hit).second += mrf::_feature_weights[rad_offset];
                else wfeatures.insert(std::pair<uint32_t,float>(h,mrf::_feature_weights[rad_offset]));

                // populate the words, if required.
                if (bow && chain.size() == 1 && chain.at(0).size()>2)
                  {
                    hash_map<uint32_t,std::string,id_hash_uint>::iterator bit;
                    if ((bit=bow->find(h))==bow->end())
                      {
                        bow->insert(std::pair<uint32_t,std::string>(h,chain.at(0)));
                      }
                  }
              }
          }
        chains.push(chain);
        mrf::mrf_build(tokens,tok,chains,wfeatures,bow,
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

            if (chain.size() < std::min((uint32_t)tokens.size(),window_length))
              {
                // first generated chain: add a token.
                str_chain chain1(chain);
                chain1.add_token(tokens.at(tok));
                chain1.decr_radius();

                if (chain1.get_radius() >= min_radius
                    && chain1.get_radius() <= max_radius)
                  {
                    //if (chain1.size()>1 || chain1.at(0) != mrf::_stop_word_token)
                    if (chain1.size() > 1 || chain1.at(0).size() > 1)
                      {
                        // hash it and add it to features.
                        uint32_t h = mrf_hash_c<uint32_t>(chain1);
#ifdef DEBUG
                        std::cout << "h: " << h << std::endl;
                        chain1.print(std::cout);
                        std::cout << "radius: " << chain1.get_radius() << std::endl;
#endif

                        // count features, increment total, and sets an exponential weight.
                        hash_map<uint32_t,float,id_hash_uint>::iterator hit;
                        if ((hit=wfeatures.find(h))!=wfeatures.end())
                          (*hit).second += mrf::_feature_weights[chain1.get_radius()+offset];
                        else wfeatures.insert(std::pair<uint32_t,float>(h,mrf::_feature_weights[chain1.get_radius()+offset]));

                        // populate the words, if required.
                        if (bow && chain1.size() == 1 && chain1.at(0).size()>2)
                          {
                            hash_map<uint32_t,std::string,id_hash_uint>::iterator bit;
                            if ((bit=bow->find(h))==bow->end())
                              {
                                bow->insert(std::pair<uint32_t,std::string>(h,chain1.at(0)));
                              }
                          }
                      }
                  }
                // second generated chain: add a 'skip' token.
                str_chain chain2 = chain;
                chain2.add_token("<skip>");
                chain2.set_skip();

                nchains.push(chain1);
                nchains.push(chain2);
              }
          }
        if (!nchains.empty())
          mrf::mrf_build(tokens,tok,nchains,wfeatures,bow,
                         min_radius,max_radius,gen_radius,window_length);
      }
  }

  void mrf::compute_tf_idf(std::vector<hash_map<uint32_t,float,id_hash_uint>*> &bags)
  {
    size_t nbags = bags.size();
    hash_map<uint32_t,uint32_t,id_hash_uint> cached_df;
    hash_map<uint32_t,uint32_t,id_hash_uint>::const_iterator cache_hit;
    hash_map<uint32_t,float,id_hash_uint>::const_iterator chit;
    for (size_t i=0; i<nbags; i++)
      {
        float norm = 0.0;
        hash_map<uint32_t,float,id_hash_uint>::iterator hit = bags.at(i)->begin();
        while (hit!=bags.at(i)->end())
          {
            // compute df.
            uint32_t df = 0;
            if ((cache_hit = cached_df.find((*hit).first))!=cached_df.end())
              df = (*cache_hit).second;
            else
              {
                for (size_t j=0; j<nbags; j++)
                  {
                    if ((chit=bags.at(j)->find((*hit).first))!=bags.at(j)->end())
                      if ((*chit).second != 0.0) // secure...
                        df++;
                  }
                cached_df.insert(std::pair<uint32_t,uint32_t>((*hit).first,df)); // df in cache.
              }

            // idf.
            float idf = logf(static_cast<float>(nbags) / (static_cast<float>(df)));

            // tf-idf.
            (*hit).second *= idf;

            // norm.
            //norm += (*hit).second * (*hit).second;
            norm += (*hit).second;

            ++hit;
          }
        // normalize.
        if (norm == 0.0)
          continue;
        hit = bags.at(i)->begin();
        while (hit!=bags.at(i)->end())
          {
            (*hit).second /= norm;//sqrt(norm);
            ++hit;
          }
      }
  }

  /*- hashing. -*/
  /* uint32_t mrf::mrf_hash(const str_chain &chain)
  {
    // rank chains which do not contain any skipped token (i.e. no
    // order preservation).
    str_chain cchain(chain);
    if (!chain.has_skip())
      cchain = chain.rank_alpha();

    uint32_t h = 0;
    size_t csize = std::min(10,(int)cchain.size());  // beware: hctable may be too small...
    for (size_t i=0;i<csize;i++)
      {
  std::string token = cchain.at(i);
  uint32_t hashed_token = mrf::_skip_token;
  if (token != "<skip>")
    hashed_token= mrf::SuperFastHash(token.c_str(),token.size());

  #ifdef DEBUG
  //debug
   //std::cout << "hashed token: " << hashed_token << std::endl;
  //debug
  #endif

  h += hashed_token * mrf::_hctable[i]; // beware...
      }
    return h;
  } */

  uint32_t mrf::mrf_hash(const std::vector<std::string> &tokens)
  {
    uint32_t h = 0;
    size_t csize = std::min(10,(int)tokens.size());
    for (size_t i=0; i<csize; i++)
      {
        std::string token = tokens.at(i);
        uint32_t hashed_token= SuperFastHash(token.c_str(),token.size());

#ifdef DEBUG
        //debug
        //std::cout << "hashed token: " << hashed_token << std::endl;
        //debug
#endif

        h += hashed_token * mrf::_hctable[i]; // beware...
      }
    return h;
  }

  // Paul Hsieh's super fast hash function.

#include "stdint.h"
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
		      +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif


  uint32_t SuperFastHash (const char * data, uint32_t len)
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

  int mrf::hash_compare(const uint32_t &a, const uint32_t &b)
  {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
  }

  double mrf::radiance(const std::string &query1,
                       const std::string &query2,
                       const uint32_t &window_length_default)
  {
    return mrf::radiance(query1,query2,0,window_length_default);
  }

  double mrf::radiance(const std::string &query1,
                       const std::string &query2,
                       const int &min_radius,
                       const int &max_radius)
  {
    // mrf call.
    std::vector<uint32_t> features1;
    mrf::mrf_features_query(query1,features1,min_radius,max_radius);

    std::vector<uint32_t> features2;
    mrf::mrf_features_query(query2,features2,min_radius,max_radius);

    uint32_t common_features = 0;
    return mrf::radiance(features1,features2,common_features);
  }

  double mrf::distance(const std::vector<uint32_t> &sorted_features1,
                       const std::vector<uint32_t> &sorted_features2,
                       uint32_t &common_features)
  {
    // distance computation.
    common_features = 0;
    int nsf1 = sorted_features1.size();
    int nsf2 = sorted_features2.size();
    int cfeat1 = 0;  // feature counter.
    int cfeat2 = 0;
    while (cfeat1<nsf1)
      {
        int cmp = mrf::hash_compare(sorted_features1.at(cfeat1),
                                    sorted_features2.at(cfeat2));

        if (cmp > 0)
          {
            ++cfeat2; // keep on moving in set 2.
            if (cfeat2 >= nsf2)
              break;
          }
        else if (cmp < 0)
          {
            ++cfeat1;
          }
        else
          {
            common_features++;

            ++cfeat1;
            ++cfeat2;
            if (cfeat2 >= nsf2)
              break;
          }
      }

    double found_only_in_set1 = nsf1 - common_features;
    double found_only_in_set2 = nsf2 - common_features;

    double distance = found_only_in_set1 + found_only_in_set2;

#ifdef DEBUG
    //debug
    std::cout << "nsf1: " << nsf1 << " -- nsf2: " << nsf2 << std::endl;
    std::cout << "common features: " << common_features << std::endl;
    std::cout << "found only in set1: " << found_only_in_set1 << std::endl;
    std::cout << "found only in set2: " << found_only_in_set2 << std::endl;
    //debug
#endif

    return distance;
  }

  double mrf::radiance(const std::vector<uint32_t> &sorted_features1,
                       const std::vector<uint32_t> &sorted_features2,
                       uint32_t &common_features)
  {
    // distance.
    double dist = mrf::distance(sorted_features1,sorted_features2,common_features);

    // radiance.
    double radiance = (common_features * common_features) / (dist + mrf::_epsilon);

    return radiance;
  }

} /* end of namespace. */
