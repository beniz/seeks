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
 * 
 * Portions of the code below appear in the sample code from OpenCV with
 * the following comment:
 * A Demo to OpenCV Implementation of SURF
 * Further Information Refer to "SURF: Speed-Up Robust Feature"
 * Author: Liu Liu
 * liuliu.1987+opencv@gmail.com
 */

#include "ocvsurf.h"
#include "miscutil.h"
#include "errlog.h"

#include <cxflann.h>

#include <iostream>
#include <fstream>

#include <unistd.h>

using sp::miscutil;
using sp::errlog;

namespace seeks_plugins
{
   CvSURFParams ocvsurf::_surf_params = cvSURFParams(500,1);
         
   void ocvsurf::generate_surf_features(const std::string *img_content,
					CvSeq *&objectKeypoints,
					CvSeq *&objectDescriptors,
					const img_search_snippet *sg)
     {
	// XXX: Ridiculous! OpenCV doesn't support loading an image from memory,
	// all hacks tried where too complicated, and deep into OpenCV. Sad.
	// Now we need to write every image on disk and reload it, which
	// is totally stupid.
	static std::string tmp_rep = "/tmp/"; //TODO: in configuration.
	
	// write to file.
	std::string tfname = tmp_rep + miscutil::to_string(sg->_id);
	std::ofstream ofile(tfname.c_str(),std::ofstream::binary);
	ofile.write(img_content->c_str(),img_content->length());
	
	IplImage *img = cvLoadImage(tfname.c_str(),CV_LOAD_IMAGE_GRAYSCALE);
	if (!img) // failed loading image, usually from a broken transfer earlier...
	  {
	     unlink(tfname.c_str());
	     return;
	  }
	
	// remove file.
	unlink(tfname.c_str());
	
	// extract SURF features.
	try
	  {
	     cvExtractSURF(img, 0, &objectKeypoints, &objectDescriptors, 
			   sg->_surf_storage, ocvsurf::_surf_params);
	  }
	catch (cv::Exception e)
	  {
	     errlog::log_error(LOG_LEVEL_ERROR,"Error extracting features from image loaded from %s: %s", tfname.c_str(),
			       e.err.c_str());
	     return; // failure.
	  }
	
	//debug
	/* std::cerr << "extracted features for img " << sg->_cached << std::endl;
	 std::cerr << "keypoints: " << objectKeypoints->total << " -- descriptors: " << objectDescriptors->total << std::endl; */
	//debug
     }

   void ocvsurf::flannFindPairs(CvSeq *o1desc,
				CvSeq *o2desc,
				std::vector<surf_pair> &ptpairs)
     {
	int length = (int)(o1desc->elem_size/sizeof(float));
	
	cv::Mat m_object(o1desc->total, length, CV_32F);
	cv::Mat m_image(o2desc->total, length, CV_32F);
	
	// copy descriptors
	CvSeqReader obj_reader;
	float* obj_ptr = m_object.ptr<float>(0);
	cvStartReadSeq( o1desc, &obj_reader );
	for(int i = 0; i < o1desc->total; i++ )
	  {
	     const float* descriptor = (const float*)obj_reader.ptr;
	     CV_NEXT_SEQ_ELEM( obj_reader.seq->elem_size, obj_reader );
	     memcpy(obj_ptr, descriptor, length*sizeof(float));
	     obj_ptr += length;
	  }
	
	CvSeqReader img_reader;
	float* img_ptr = m_image.ptr<float>(0);
	cvStartReadSeq( o2desc, &img_reader );
	for(int i = 0; i < o2desc->total; i++ )
	  {
	     const float* descriptor = (const float*)img_reader.ptr;
	     CV_NEXT_SEQ_ELEM( img_reader.seq->elem_size, img_reader );
	     memcpy(img_ptr, descriptor, length*sizeof(float));
	     img_ptr += length;
	  }
	
	// find nearest neighbors using FLANN
	cv::Mat m_indices(o1desc->total, 2, CV_32S);
	cv::Mat m_dists(o1desc->total, 2, CV_32F);
	cv::flann::Index flann_index(m_image, cv::flann::KDTreeIndexParams(4));  // using 4 randomized kdtrees
	flann_index.knnSearch(m_object, m_indices, m_dists, 2, cv::flann::SearchParams(64) ); // 64 is maximum number of leaves being checked
	
	int* indices_ptr = m_indices.ptr<int>(0);
	float* dists_ptr = m_dists.ptr<float>(0);
	for (int i=0;i<m_indices.rows;++i) 
	  {
	     /* if (dists_ptr[2*i]<0.6*dists_ptr[2*i+1]) 
	       {
		  ptpairs.push_back(i);
		  ptpairs.push_back(indices_ptr[2*i]);
	       } */
	     ptpairs.push_back(surf_pair(i,indices_ptr[2*i],dists_ptr[2*i]/static_cast<double>(dists_ptr[2*i+1])));
	  }
	
	//debug
	//std::cout << "ptpairs size: " << ptpairs.size() << std::endl;
	//debug
     }

   
   
} /* end of namespace. */
  
