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

#undef HAVE_CONFIG_H
#include "cv.h" // OpenCV.
#include "highgui.h"
#define HAVE_CONFIG_H
#include "img_search_snippet.h"

namespace seeks_plugins
{
   
   class surf_pair
     {
      public:
	surf_pair(const int &i, const int &j,
		  const double &dist)
	  :_i(i),_j(j),_dist(dist)
	    {};
	~surf_pair() {};

	int _i;
	int _j;
	double _dist;
     };
      
   class ocvsurf
     {
      public:
	static void generate_surf_features(const std::string *img_content,
					   CvSeq *&objectKeyPoints,
					   CvSeq *&objectDescriptors,
					   const img_search_snippet *sp);
	
	/* static void flannFindPairs(CvSeq *o1desc,
				   CvSeq *o2desc,
				   std::vector<surf_pair> &ptpairs); */
	
	static int bruteMatch(CvMat *&points1, CvMat *&points2,
			      CvSeq *kp1, CvSeq *desc1, CvSeq *kp2, CvSeq *desc2,
			      const bool &filter);
	
	static int removeOutliers(CvMat *&points1, CvMat *&points2);
	
	static CvSURFParams _surf_params;
     };
   
} /* end of namespace. */

#endif
  
