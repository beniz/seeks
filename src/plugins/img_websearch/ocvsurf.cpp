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
#define EXTENDED_DESCRIPTOR 1
   
   CvSURFParams ocvsurf::_surf_params = cvSURFParams(600,EXTENDED_DESCRIPTOR);
         
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
	catch (cv::Exception &e)
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

   /* void ocvsurf::flannFindPairs(CvSeq *o1desc,
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
	cv::flann::Index flann_index(m_image, cv::flann::KDTreeIndexParams(8));  // using 8 randomized kdtrees
	flann_index.knnSearch(m_object, m_indices, m_dists, 2, cv::flann::SearchParams(64) ); // 64 is maximum number of leaves being checked
	
	int* indices_ptr = m_indices.ptr<int>(0);
	float* dists_ptr = m_dists.ptr<float>(0);
	for (int i=0;i<m_indices.rows;++i) 
	  { */
	     /* if (dists_ptr[2*i]<0.6*dists_ptr[2*i+1]) 
	       {
		  ptpairs.push_back(i);
		  ptpairs.push_back(indices_ptr[2*i]);
	       } */
	    /* ptpairs.push_back(surf_pair(i,indices_ptr[2*i],dists_ptr[2*i]/static_cast<double>(dists_ptr[2*i+1])));
	  }
	
	//debug
	//std::cout << "ptpairs size: " << ptpairs.size() << std::endl;
	//debug
     } */

#define CORRELATION_THRESHOLD 0.9
   // brute-force attempt at correlating the two sets of features
   int ocvsurf::bruteMatch(CvMat *&points1, CvMat *&points2,
			   CvSeq *kp1, CvSeq *desc1, CvSeq *kp2, CvSeq *desc2,
			   const bool &filter) 
     {
      int i,j,k;
	double* avg1 = (double*)malloc(sizeof(double)*kp1->total);
        double* avg2 = (double*)malloc(sizeof(double)*kp2->total);
        double* dev1 = (double*)malloc(sizeof(double)*kp1->total);
        double* dev2 = (double*)malloc(sizeof(double)*kp2->total);
        int* best1 = (int*)malloc(sizeof(int)*kp1->total);
	int* best2 = (int*)malloc(sizeof(int)*kp2->total);
        double* best1corr = (double*)malloc(sizeof(double)*kp1->total);
        double* best2corr = (double*)malloc(sizeof(double)*kp2->total);
        float *seq1, *seq2;
        int descriptor_size = EXTENDED_DESCRIPTOR ? 128 : 64;
        for (i=0; i<kp1->total; i++) 
	  {
	   // find average and standard deviation of each descriptor
	   avg1[i] = 0;
	   dev1[i] = 0;
	   seq1 = (float*)cvGetSeqElem(desc1, i);
	   for (k=0; k<descriptor_size; k++) avg1[i] += seq1[k];
	   avg1[i] /= descriptor_size;
	   for (k=0; k<descriptor_size; k++) dev1[i] +=
	     (seq1[k]-avg1[i])*(seq1[k]-avg1[i]);
	   dev1[i] = sqrt(dev1[i]/descriptor_size);
	   
	   // initialize best1 and best1corr
	   best1[i] = -1;
	   best1corr[i] = -1.;
        }
        for (j=0; j<kp2->total; j++) 
	  {
	   // find average and standard deviation of each descriptor
	     avg2[j] = 0;
	     dev2[j] = 0;
	     seq2 = (float*)cvGetSeqElem(desc2, j);
	     for (k=0; k<descriptor_size; k++) avg2[j] += seq2[k];
	     avg2[j] /= descriptor_size;
	     for (k=0; k<descriptor_size; k++) dev2[j] +=
	       (seq2[k]-avg2[j])*(seq2[k]-avg2[j]);
	     dev2[j] = sqrt(dev2[j]/descriptor_size);
	     
	     // initialize best2 and best2corr
	     best2[j] = -1;
	     best2corr[j] = -1.;
	  }
        double corr;
        for (i = 0; i < kp1->total; ++i) 
	  {
	     seq1 = (float*)cvGetSeqElem(desc1, i);
	     for (j = 0; j < kp2->total; ++j) {
                corr = 0;
                seq2 = (float*)cvGetSeqElem(desc2, j);
                for (k = 0; k < descriptor_size; ++k)
		  corr += (seq1[k]-avg1[i])*(seq2[k]-avg2[j]);
                corr /= (descriptor_size-1)*dev1[i]*dev2[j];
                if (corr > best1corr[i]) {
		   best1corr[i] = corr;
		   best1[i] = j;
                }
                if (corr > best2corr[j]) 
		  {
		     best2corr[j] = corr;
		     best2[j] = i;
		  }
	     }
	  }
        j = 0;
        for (i = 0; i < kp1->total; i++)
	  if (best2[best1[i]] == i && best1corr[i] > CORRELATION_THRESHOLD)
	    j++;
	
	if (j>0 && filter)
	  {
	     points1 = cvCreateMat(1,j,CV_32FC2);
	     points2 = cvCreateMat(1,j,CV_32FC2);
	     CvPoint2D32f *p1, *p2;
	     j = 0;
	     for (i = 0; i < kp1->total; i++) 
	       {
		  if (best2[best1[i]] == i && best1corr[i] > CORRELATION_THRESHOLD) 
		    {
		       p1 = &((CvSURFPoint*)cvGetSeqElem(kp1,i))->pt;
		       p2 = &((CvSURFPoint*)cvGetSeqElem(kp2,best1[i]))->pt;
		       points1->data.fl[j*2] = p1->x;
		       points1->data.fl[j*2+1] = p1->y;
		       points2->data.fl[j*2] = p2->x;
		       points2->data.fl[j*2+1] = p2->y;
		       j++;
		    }
	       }
	  }
		
        free(best2corr);
        free(best1corr);
        free(best2);
        free(best1);
        free(avg1);
        free(avg2);
        free(dev1);
        free(dev2);
	return j;
     } 
   
   int ocvsurf::removeOutliers(CvMat *&points1, CvMat *&points2) 
     {
	CvMat *F = cvCreateMat(3,3,CV_32FC1); //TODO: free F ?
	CvMat *status = cvCreateMat(1,points1->cols,CV_8UC1); //TODO: free status.
	int fm_count = cvFindFundamentalMat(points1,points2,F,
					    CV_FM_RANSAC,1.,0.99,status );
	
	// iterates the set of putative correspondences and removes correspondences
	// marked as outliers by cvFindFundamentalMat()
	int count = 0;
	int j = 0;
	for (int i = 0; i < status->cols; i++) if (CV_MAT_ELEM(*status,unsigned
							       char,0,i)) count++;
	CvMat *points1_ = points1;
	CvMat *points2_ = points2;
	if (!count) 
	  { // no inliers
	     points1 = NULL;
	     points2 = NULL; // beware: leak ?
	     return j;
	  }
	else 
	  {
	     points1 = cvCreateMat(1,count,CV_32FC2);
	     points2 = cvCreateMat(1,count,CV_32FC2);
	     int j = 0;
	     for (int i = 0; i < status->cols; i++) {
		if (CV_MAT_ELEM(*status,unsigned char,0,i)) {
		   points1->data.fl[j*2] = points1_->data.fl[i*2];
		   // //p1->x
		   points1->data.fl[j*2+1] = points1_->data.fl[i*2+1];
		   //p1->y
		   points2->data.fl[j*2] = points2_->data.fl[i*2];
		   // //p2->x
		   points2->data.fl[j*2+1] = points2_->data.fl[i*2+1];
		   // //p2->y
		   j++;
		}
	     }
          }
	cvReleaseMat(&points1_);
	cvReleaseMat(&points2_);
	return j;
     }
   
} /* end of namespace. */
  
