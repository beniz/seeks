/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2010 Emmanuel Benazera, juban@free.fr
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

#include "ocvsurf.h"
#include "highgui.h" // from OpenCV.

#include <iostream>
#include <stdlib.h>

using namespace sp;
using namespace seeks_plugins;

int main(int argc, char **argv)
{
   if (argc < 3)
     {
	std::cout << "Usage: <img1> <img2>\n";
	exit(0);
     }
   
   std::string img1 = std::string(argv[1]);
   std::string img2 = std::string(argv[2]);
   
   IplImage *img1_obj = cvLoadImage(img1.c_str(),CV_LOAD_IMAGE_GRAYSCALE);
   IplImage *img2_obj = cvLoadImage(img2.c_str(),CV_LOAD_IMAGE_GRAYSCALE);
   
   CvMemStorage* storage = cvCreateMemStorage(0);
   
   CvSeq *o1points = 0, *o1desc = 0;
   CvSeq *o2points = 0, *o2desc = 0;
   cvExtractSURF(img1_obj, 0, &o1points, &o1desc,
		 storage, ocvsurf::_surf_params);
   cvExtractSURF(img2_obj, 0, &o2points, &o2desc,
		 storage, ocvsurf::_surf_params);
   
   std::cout << "keypoints1 size: " << o1points->total << std::endl;
   std::cout << "descriptors1 size: " << o1desc->total << std::endl;
   std::cout << "keypoints2 size: " << o2points->total << std::endl;
   std::cout << "descriptors2 size: " << o2desc->total << std::endl;
   
   std::vector<surf_pair> ptpairs;
   ocvsurf::flannFindPairs(o1desc,o2desc,
			   ptpairs);
}
