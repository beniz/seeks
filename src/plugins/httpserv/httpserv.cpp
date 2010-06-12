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

#include "httpserv.h"
#include "httpserv_configuration.h"
#include "seeks_proxy.h"
#include "plugin_manager.h"
#include "websearch.h"
#include "cgisimple.h"
#include "sweeper.h"
#include "miscutil.h"
#include "errlog.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <iostream>

using namespace sp;

namespace seeks_plugins
{
   
   httpserv::httpserv(const std::string &address,
		      const u_short &port)
     : plugin(),_address(address),_port(port)
       {
	  /* initializing and starting server. */
	  _evbase = event_base_new();
          _srv = evhttp_new(_evbase);
          evhttp_bind_socket(_srv, _address.c_str(), _port); // server binding to the address & port.
	  
	  errlog::log_error(LOG_LEVEL_INFO,"Seeks HTTP server plugin listening on %s:%u",
			    _address.c_str(),_port);
	  
          init_callbacks();
          event_base_dispatch(_evbase); // XXX: for debug.
       }
   
   httpserv::httpserv()
     : plugin()
       {
	  _name = "httpserv";
	  _version_major = "0";
	  _version_minor = "1";
	
	  if (seeks_proxy::_datadir.empty())
	    _config_filename = plugin_manager::_plugin_repository + "httpserv/httpserv-config";
	  else
	    _config_filename = seeks_proxy::_datadir + "/plugins/httpserv/httpserv-config";
	  
#ifdef SEEKS_CONFIGDIR
	  struct stat stFileInfo;
	  if (!stat(_config_filename.c_str(), &stFileInfo)  == 0)
	    {
	       _config_filename = SEEKS_CONFIGDIR "/httpserv-config";
	    }
#endif
	  
	  if (httpserv_configuration::_hconfig == NULL)
	    httpserv_configuration::_hconfig = new httpserv_configuration(_config_filename);
	  _configuration = httpserv_configuration::_hconfig;
	  
	  /* sets address & port. */
	  _address = httpserv_configuration::_hconfig->_host;
	  _port = httpserv_configuration::_hconfig->_port;
	  
	  /* initializing and starting server. */
	  _evbase = event_base_new();
	  _srv = evhttp_new(_evbase);
	  evhttp_bind_socket(_srv, _address.c_str(), _port); // server binding to the address & port.
	  	  
	  /* errlog::log_error(LOG_LEVEL_INFO,"Seeks HTTP server plugin listening on %s:%u",
			    _address.c_str(),_port); */
	  
	  init_callbacks();
	  
	  /* pthread_t server_thread;
	  int err = pthread_create(&server_thread,NULL,
				   (void*(*)(void*))&event_base_dispatch,_evbase);
	  pthread_detach(server_thread); */
       }
   
   httpserv::~httpserv()
     {
	evhttp_free(_srv);
	event_base_free(_evbase);
     }

   void httpserv::start()
     {
	errlog::log_error(LOG_LEVEL_INFO,"Seeks HTTP server plugin listening on %s:%u",
			  _address.c_str(),_port);
	
	pthread_t server_thread;
	int err = pthread_create(&server_thread,NULL,
				 (void*(*)(void*))&event_base_dispatch,_evbase);
	pthread_detach(server_thread);
     }
   
   void httpserv::stop()
     {
	event_base_loopbreak(_evbase);
     }
      
   void httpserv::init_callbacks()
     {
	evhttp_set_cb(_srv,"/search",&httpserv::websearch,NULL);
	evhttp_set_cb(_srv,"/",&httpserv::websearch_hp,NULL);
	evhttp_set_cb(_srv,"/websearch-hp",&httpserv::websearch_hp,NULL);
	evhttp_set_cb(_srv,"/seeks_hp_search.css",&httpserv::seeks_hp_css,NULL);
	evhttp_set_cb(_srv,"/seeks_search.css",&httpserv::seeks_search_css,NULL);
	evhttp_set_cb(_srv,"/opensearch.xml",&httpserv::opensearch_xml,NULL);
	//evhttp_set_gencb(_srv,&httpserv::unknown_path,NULL); /* 404: catches all other resources. */
	evhttp_set_gencb(_srv,&httpserv::file_service,NULL);
     }
   
   void httpserv::reply_with_error_400(struct evhttp_request *r)
     {
	evhttp_send_reply(r, HTTP_BADREQUEST, "BAD REQUEST", NULL);
     }
   
   void httpserv::reply_with_error(struct evhttp_request *r,
				   const int &http_code,
				   const char *message,
				   const std::string &error_message)
     {
	/* error output. */
	errlog::log_error(LOG_LEVEL_ERROR,"httpserv error: %s",error_message.c_str());
	
	// body.
	struct evbuffer *buffer;
	buffer = evbuffer_new();
	
	char msg[error_message.length()];
	for (size_t i=0;i<error_message.length();i++)
	  msg[i] = error_message[i];
	evbuffer_add(buffer,msg,sizeof(msg));
	
	evhttp_send_reply(r, http_code, message, buffer);
	evbuffer_free(buffer);
     }
      
   void httpserv::reply_with_empty_body(struct evhttp_request *r,
					const int &http_code,
					const char *message)
     {
	evhttp_send_reply(r, http_code, message, NULL);
     }
      
   void httpserv::reply_with_body(struct evhttp_request *r,
				  const int &http_code,
				  const char *message,
				  const std::string &content,
				  const std::string &content_type)
     {
	/* headers. */
	evhttp_add_header(r->output_headers,"Content-Type",content_type.c_str());
		
	/* body. */
	struct evbuffer *buffer;
	buffer = evbuffer_new();
	
	char msg[content.length()];
	for (size_t i=0;i<content.length();i++)
	  msg[i] = content[i];
	evbuffer_add(buffer,msg,sizeof(msg));
	
	evhttp_send_reply(r, http_code, message, buffer);
	evbuffer_free(buffer);
     }

   void httpserv::websearch_hp(struct evhttp_request *r, void *arg)
     {
	client_state csp;
	csp._config = seeks_proxy::_config;
	http_response rsp;
	hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;
	
	/* return requested file. */
	sp_err serr = websearch::cgi_websearch_hp(&csp,&rsp,&parameters);
	if (serr != SP_ERR_OK)
	  {
	     httpserv::reply_with_empty_body(r,404,"ERROR");
	     return;
	  }
	
	/* fill up response. */
	std::string content = std::string(rsp._body);
	httpserv::reply_with_body(r,200,"OK",content);
     
	/* run the sweeper, for timed out query contexts. */
	sweeper::sweep();
     }
      
   void httpserv::seeks_hp_css(struct evhttp_request *r, void *arg)
     {
	client_state csp;
	csp._config = seeks_proxy::_config;
	http_response rsp;
	hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;
	
	/* return requested file. */
	sp_err serr = websearch::cgi_websearch_search_hp_css(&csp,&rsp,&parameters);
	if (serr != SP_ERR_OK)
	  {
	     httpserv::reply_with_empty_body(r,404,"ERROR");
	     return;
	  }
	
	/* fill up response. */
	std::string content = std::string(rsp._body);
	httpserv::reply_with_body(r,200,"OK",content,"text/css");
     }
      
   void httpserv::seeks_search_css(struct evhttp_request *r, void *arg)
     {
	client_state csp;
	csp._config = seeks_proxy::_config;
	http_response rsp;
	hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;
		
	/* return requested file. */
	sp_err serr = websearch::cgi_websearch_search_css(&csp,&rsp,&parameters);
	if (serr != SP_ERR_OK)
	  {
	     httpserv::reply_with_empty_body(r,404,"ERROR");
	     return;
	  }

	/* fill up response. */
	std::string content = std::string(rsp._body);
	httpserv::reply_with_body(r,200,"OK",content,"text/css");
     }

   void httpserv::opensearch_xml(struct evhttp_request *r, void *arg)
     {
	client_state csp;
	csp._config = seeks_proxy::_config;
	http_response rsp;
	hash_map<const char*,const char*,hash<const char*>,eqstr> parameters;
	
	/* return requested file. */
	sp_err serr = websearch::cgi_websearch_opensearch_xml(&csp,&rsp,&parameters);
	if (serr != SP_ERR_OK)
	  {
	     httpserv::reply_with_empty_body(r,404,"ERROR");
	     return;
	  }
	
	/* fill up response. */
	std::string content = std::string(rsp._body);
	httpserv::reply_with_body(r,200,"OK",content,"application/opensearchdescription+xml");
     }
      
   void httpserv::file_service(struct evhttp_request *r, void *arg)
     {
	client_state csp;
	csp._config = seeks_proxy::_config;
	http_response rsp;
	hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
	  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
	
	/* return requested file. */
	std::string uri_str = std::string(r->uri);
	
	/* XXX: truely, this is a hack, we're routing websearch file service. */
	std::string ct;  // content-type.
	sp_err serr = SP_ERR_OK;
	if (miscutil::strncmpic(uri_str.c_str(),"/plugins",8)==0)
	  {
	     uri_str = uri_str.substr(9);
	     miscutil::add_map_entry(parameters,"file",1,uri_str.c_str(),1);
	     serr = cgisimple::cgi_plugin_file_server(&csp,&rsp,parameters);
	  }
	else if (miscutil::strncmpic(uri_str.c_str(),"/public",7)==0)
	  {
	     uri_str = uri_str.substr(8);
	     miscutil::add_map_entry(parameters,"file",1,uri_str.c_str(),1);
	     serr = cgisimple::cgi_file_server(&csp,&rsp,parameters);
	  }
	else if (miscutil::strncmpic(uri_str.c_str(),"/websearch-hp",13)==0) // to catch some errors.
	  {
	     miscutil::free_map(parameters);
	     httpserv::websearch_hp(r,arg);
	     return;
	  }
	else if (miscutil::strncmpic(uri_str.c_str(),"/robots.txt",11)==0)
	  {
	     miscutil::add_map_entry(parameters,"file",1,uri_str.c_str(),1); // returns public/robots.txt
	     serr = cgisimple::cgi_file_server(&csp,&rsp,parameters);
	     ct = "text/plain";
	  }
	
	// XXX: other services can be routed here.
	miscutil::free_map(parameters);
	if (serr != SP_ERR_OK)
	  {
	     httpserv::reply_with_empty_body(r,404,"ERROR");
	     return;
	  }
	
	/* fill up response. */
	if (ct.empty())
	  {
	     std::list<const char*>::const_iterator lit = rsp._headers.begin();
	     while(lit!=rsp._headers.end())
	       {
		  if (miscutil::strncmpic((*lit),"content-type:",13) == 0)
		    {
		       ct = std::string((*lit));
		       ct = ct.substr(14);
		       break;
		    }
		  ++lit;
	       }
	  }
	std::string content = std::string(rsp._body,rsp._content_length);
	httpserv::reply_with_body(r,200,"OK",content,ct);
     }
   
   void httpserv::websearch(struct evhttp_request *r, void *arg)
     {
	client_state csp;
	csp._config = seeks_proxy::_config;
	http_response rsp;
	hash_map<const char*,const char*,hash<const char*>,eqstr> *parameters
	  = new hash_map<const char*,const char*,hash<const char*>,eqstr>();
	
	/* parse query. */
	const char *uri_str = r->uri;
	int err = 0;
	if (uri_str)
	  {
	     std::string uri = std::string(r->uri);
	     err = httpserv::parse_query(uri,*parameters);
	  }
	if (err != 0 || !uri_str)
	  {
	     // send 400 error response.
	     httpserv::reply_with_error_400(r);
	     return;
	  }
	
	/* fill up csp headers. */
	const char *rheader = evhttp_find_header(r->input_headers, "accept-language");
	if (rheader)
	  miscutil::enlist_unique_header(&csp._headers,"accept-language",strdup(rheader));
	/* rheader = evhttp_find_header(r->input_headers, "host");
	if (rheader)
	  miscutil::enlist_unique_header(&csp._headers,"Seeks-Remote-Location",strdup(rheader)); */
	
	/* perform websearch. */
	sp_err serr = websearch::cgi_websearch_search(&csp,&rsp,parameters);
	miscutil::free_map(parameters);
	miscutil::list_remove_all(&csp._headers);
	
	if (serr != SP_ERR_OK)
	  {
	     std::string error_message = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\"><html><head><title>500 - Seeks websearch error </title></head><body></body></html>";
	     httpserv::reply_with_error(r,500,"ERROR",error_message);
	     return;
	  }
	
	/* fill up response. */
	std::string ct; // content-type.
	std::list<const char*>::const_iterator lit = rsp._headers.begin();
	while(lit!=rsp._headers.end())
	  {
	     if (miscutil::strncmpic((*lit),"content-type:",13) == 0)
	       {
		  ct = std::string((*lit));
		  ct = ct.substr(14);
		  break;
	       }
	     ++lit;
	  }
	std::string content;
	if (rsp._body)
	  content = std::string(rsp._body); // XXX: beware of length.
	httpserv::reply_with_body(r,200,"OK",content,ct);
     }
   
   void httpserv::unknown_path(struct evhttp_request *r, void *arg)
     {
	httpserv::reply_with_empty_body(r,404,"ERROR");
     }

   int httpserv::parse_query(const std::string &uri,
			     hash_map<const char*,const char*,hash<const char*>,eqstr> &parameters)
     {
	/* decode uri. */
	char *dec_uri_str = evhttp_decode_uri(uri.c_str());
	std::string dec_uri = std::string(dec_uri_str);
	free(dec_uri_str);
	
	/* parse query. */
	std::vector<std::string> host_and_params;
	miscutil::tokenize(dec_uri,host_and_params,"?&");
	if (host_and_params.size()<2)
	  return 1; // error parsing parameters.
	size_t nparams = host_and_params.size();
	for (size_t i=1;i<nparams;i++)
	  {
	     std::vector<std::string> tokens;
	     miscutil::tokenize(host_and_params.at(i),tokens,"=");
	     if (tokens.size()!=2)
	       {
		  // ignore parameter.
	       }
	     else miscutil::add_map_entry(&parameters,tokens.at(0).c_str(),1,tokens.at(1).c_str(),1);
	  }
	return 0;
     }
         
   /* auto-registration */
   extern "C"
     {
	plugin* makerh()
	  {
	     return new httpserv;
	  }
     }
   
   class proxy_autorh
     {
      public:
	proxy_autorh()
	  {
	     plugin_manager::_factory["httpserv"] = makerh; // beware: default plugin shell with no name.
	  }
     };
   
   proxy_autorh _p; // one instance, instanciated when dl-opening.
   
} /* end of namespace. */
