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

#include "curl_mget.h"
#include "ocvsurf.h"

#include <iostream>
#include <stdlib.h>

using namespace sp;
using namespace seeks_plugins;

int main(int argc, char **argv)
{
  if (argc < 2)
    {
      std::cout << "Usage: <img url>\n";
      exit(0);
    }

  std::string addr = std::string(argv[1]);
  std::vector<std::string> addrs;
  addrs.push_back(addr);

  curl_mget cmg(1,60,0,60,0); // 60 seconds connection & transfer timeout.
  std::string **outputs = cmg.www_mget(addrs,1,NULL,false); // don't use a proxy.

  //std::cout << *outputs[0] << std::endl;

  ocvsurf::init();

  //cv::Mat imgmat = ocvsurf::imload(*outputs[0],-1); // -1 for untouched img.
  cv::Mat imgmat = cv::imread("tb2.jpg",-1);

  std::cout << "loaded\n";

  cv::namedWindow("Image", 1);
  cv::imshow("Image",imgmat);
  cvWaitKey(0);
}
