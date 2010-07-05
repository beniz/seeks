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

#ifndef OCVSURF_H
#define OCVSURF_H

#include "cv.h" // OpenCV.
#include "highgui.h"
#include "img_search_snippet.h"

namespace seeks_plugins
{
   
   class ocvsurf
     {
      public:
	static void generate_surf_features(const std::string *img_content,
					   CvSeq *&objectKeyPoints,
					   CvSeq *&objectDescriptors,
					   const img_search_snippet *sp);
	
	static void flannFindPairs(CvSeq *o1desc,
				   CvSeq *o2desc,
				   std::vector<int> &ptpairs);
	
	static CvSURFParams _surf_params;
     };
   
} /* end of namespace. */

#endif
  
