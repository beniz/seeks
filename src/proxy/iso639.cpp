/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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

#include "iso639.h"
#include "mem_utils.h"

#include <string.h>
#include <string>
#include <iostream>

namespace sp
{
  hash_map<const char*,bool,hash<const char*>,eqstr> iso639::_codes = hash_map<const char*,bool,hash<const char*>,eqstr>();

  void iso639::initialize()
  {
    std::string str_codes[191] =
    {
      "aa","ab","ae","af","ak","am","an","ar","as","av","ay","az","ba","be","bg","bh","bi","bm","bn","bo","br","bs",
      "ca","ce","ch","co","cr","cs","cu","cv","cy","da","de","dv","dz","ee","el","en","eo","es","et","eu","fa","ff",
      "fi","fj","fo","fr","fy","ga","gd","gl","gn","gu","gv","ha","he","hi","ho","hr","ht","hu","hy","hz","ia","id",
      "ie","ig","ii","ik","in","io","is","it","iu","iw","ja","ji","jv","jw","ka","kg","ki","kj","kk","kl","km","kn",
      "ko","kr","ks","ku","kv","kw","ky","la","lb","lg","li","ln","lo","lt","lu","lv","mg","mh","mi","mk","ml","mn",
      "mo","mr","ms","mt","my","na","nb","nd","ne","ng","nl","nn","no","nr","nv","ny","oc","oj","om","or","os","pa",
      "pi","pl","ps","pt","qu","rm","rn","ro","ru","rw","ry","sa","sc","sd","se","sg","sh","si","sk","sl","sm","sn",
      "so","sq","sr","ss","st","su","sv","sw","ta","te","tg","th","ti","tk","tl","tn","to","tr","ts","tt","tw","ty",
      "ug","uk","ur","uz","ve","vi","vo","wa","wo","xh","yi","yo","za","zh","zu"
    };

    for (int i=0; i<191; i++)
      iso639::_codes.insert(std::pair<const char*,bool>(strdup(str_codes[i].c_str()),true)); // not freed...
  }

  void iso639::cleanup()
  {
    hash_map<const char*,bool,hash<const char*>,eqstr>::iterator hit,hit2;
    hit = _codes.begin();
    while(hit!=_codes.end())
      {
        hit2 = hit;
        const char *key = (*hit2).first;
        ++hit;
        _codes.erase(hit2);
        free_const(key);
      }
  }

  bool iso639::has_code(const char *c)
  {
    hash_map<const char*,bool,hash<const char*>,eqstr>::const_iterator hit;
    if ((hit=iso639::_codes.find(c))!=iso639::_codes.end())
      {
        return (*hit).second;
      }
    else return false;
  }


} /* end of namespace. */
