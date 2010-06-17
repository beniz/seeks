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
  
#include "websearch_sg.h"
#include "qc_sg.h"
#include "miscutil.h"

using sp::miscutil;

namespace seeks_plugins
{
   
   websearch_sg::websearch_sg()
     :websearch()
       {
       }
   
   websearch_sg::~websearch_sg()
     {
     }

   //TODO: create query_context_sg when falling back on the solitary websearch mode...
   sp_err websearch_sg::cgi_websearch_search_sg(client_state *csp,
						http_response *rsp,
						const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters)
     {
	const char *social = miscutil::lookup(parameters,"social");
	if (!social || miscutil::strcmpic(social,"no")==0)
	  return websearch_sg::perform_websearch(csp,rsp,parameters); // falling back on solitary websearch.
	if (!parameters->empty())
	  {
	     const char *query = miscutil::lookup(parameters,"q"); // grab the query.
	     if (!query || strlen(query) == 0)
	       {
		  // return websearch homepage instead.
		  return websearch::cgi_websearch_hp(csp,rsp,parameters);
	       }
	     else se_handler::preprocess_parameters(parameters); // preprocess the query...
	  
	     // perform websearch or other requested action.
	     const char *action = miscutil::lookup(parameters,"action");
	     if (!action)
	       return websearch::cgi_websearch_hp(csp,rsp,parameters);
	     
	     /**
	      * Action can be of type:
	      * - "expand": requests an expansion of the search results, expansion horizon is
	      *           specified by parameter "expansion".
	      * - "page": requests a result page, already in cache.
	      * - "similarity": requests a reordering of results, in decreasing order from the
	      *                 specified search result, identified by parameter "id".
	      * - "clusterize": requests the forming of a cluster of results, the number of specified
	      *                 clusters is given by parameter "clusters".
	      * - "urls": requests a grouping by url.
	      * - "titles": requests a grouping by title.
	      */
	     sp_err err = SP_ERR_OK;
	     if (miscutil::strcmpic(action,"expand") == 0)
	       err = websearch_sg::perform_websearch_sg(csp,rsp,parameters); // social websearch.
	     else if (miscutil::strcmpic(action,"page") == 0)
	       err = websearch::perform_websearch(csp,rsp,parameters);
	     else if (miscutil::strcmpic(action,"similarity") == 0)
	       err = websearch::cgi_websearch_similarity(csp,rsp,parameters);
	     else if (miscutil::strcmpic(action,"clusterize") == 0)
	       err = websearch::cgi_websearch_clusterize(csp,rsp,parameters);
	     else if (miscutil::strcmpic(action,"urls") == 0)
	       err = websearch::cgi_websearch_neighbors_url(csp,rsp,parameters);
	     else if (miscutil::strcmpic(action,"titles") == 0)
	       err = websearch::cgi_websearch_neighbors_title(csp,rsp,parameters);
	     else if (miscutil::strcmpic(action,"types") == 0)
	       err = websearch::cgi_websearch_clustered_types(csp,rsp,parameters);
	     else return websearch::cgi_websearch_hp(csp,rsp,parameters);
	     
	     return err;
	  }
	else
	  {
	     return SP_ERR_OK;
	  }
     }
      
   sp_err websearch_sg::perform_websearch_sg(client_state *csp,
					     http_response *rsp,
					     const hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters,
					     bool render)
     {
	sp_err err = websearch_sg::perform_websearch(csp,rsp,parameters);
	
	if (err != SP_ERR_OK)
	  return err;
	
	// event-based update.
	qc_sg *qcs = static_cast<qc_sg*>(websearch::lookup_qc(parameters,csp));
	qcs->subscribe();
	
	return err;
     }
      
} /* end of namespace. */
