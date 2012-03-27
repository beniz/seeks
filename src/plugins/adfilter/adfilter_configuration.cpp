
#include "configuration_spec.h"
#include "adfilter_configuration.h"

#include <list>

#define hash_adblock_list     3579134231ul /* "adblock-list" */
#define hash_update_frequency 3579134231ul /* "update-frequency" */
#define hash_use_filter       3579134231ul /* "use-filter" */
#define hash_use_blocker      3579134231ul /* "use-blocker" */

using namespace sp;
using sp::configuration_spec;

adfilter_configuration::adfilter_configuration(const std::string &filename) 
 : configuration_spec(filename)
{
  //load_config();
}
/*
void adfilter_configuration::set_default_config()
{
  //this->_adblock_lists = new std::list<std::string>(); // List of adblock rules URL
  this->_update_frequency = 86400 * 7;                 // List update frequecy in seconds
    //   features activation
  this->_use_filter = true;                            // If true, use page filtering
  this->_use_blocker = false;                          // If true, use URL blocking
}

void adfilter_configuration::handle_config_cmd(char *cmd, const uint32_t &cmd_hash, char *arg, char *buf, const unsigned long &linenum)
{
 /* //std::vector<std::string> bpvec;
  //char tmp[BUFFER_SIZE];
  //int vec_count;
  //char *vec[20]; // max 10 urls per feed parser.
  //int i;
  //feed_parser fed;
  //feed_parser def_fed;
  //bool def = false;
  switch (cmd_hash)
  {
    case hash_lang :
      _lang = std::string(arg);
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Websearch language");
      break;

    case hash_n :
      _Nr = atoi(arg);
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Number of websearch results per page");
      break;

    case hash_se :
      strlcpy(tmp,arg,sizeof(tmp));
      vec_count = miscutil::ssplit(tmp," \t",vec,SZ(vec),1,1);
      div_t divresult;
      divresult = div(vec_count-1,3);
      if (divresult.rem > 0)
        {
          errlog::log_error(LOG_LEVEL_ERROR, "Wrong number of parameters for search-engine "
                            "directive in websearch plugin configuration file");
          break;
        }
      if (_default_engines)
        {
          // reset engines.
          _se_enabled = feeds();
          _se_options.clear();
          _default_engines = false;
          _se_default = feeds();
        }

      fed = feed_parser(vec[0]);
      def_fed = feed_parser(vec[0]);
      for (i=1; i<vec_count; i+=3)
        {
          fed.add_url(vec[i]);
          std::string fu_name = vec[i+1];
          def = false;
          if (strcmp(vec[i+2],"default")==0)
            def = true;
          feed_url_options fuo(vec[i],fu_name,def);
          _se_options.insert(std::pair<const char*,feed_url_options>(fuo._url.c_str(),fuo));
          if (def)
            def_fed.add_url(vec[i]);
        }
      _se_enabled.add_feed(fed);
      if (!def_fed.empty())
        _se_default.add_feed(def_fed);

      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Enabled search engine");
      break;

    case hash_thumbs:
      _thumbs = static_cast<bool>(atoi(arg));
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Enable search results webpage thumbnails");
      break;

    case hash_qcd :
      _query_context_delay = strtod(arg,NULL);
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Delay in seconds before deletion of cached websearches and results");
      break;

    case hash_js:
      _js = static_cast<bool>(atoi(arg));
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Enable javascript use on the websearch result page");
      break;

    case hash_content_analysis:
      _content_analysis = static_cast<bool>(atoi(arg));
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Enable the background download of webpages pointed to by websearch results and content analysis");
      break;

    case hash_se_transfer_timeout:
      _se_transfer_timeout = atol(arg);
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Sets the transfer timeout in seconds for connections to a search engine");
      break;

    case hash_se_connect_timeout:
      _se_connect_timeout = atol(arg);
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Sets the connection timeout in seconds for connections to a search engine");
      break;

    case hash_ct_transfer_timeout:
      _ct_transfer_timeout = atol(arg);
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Sets the transfer timeout in seconds when fetching content for analysis and caching");
      break;

    case hash_ct_connect_timeout:
      _ct_connect_timeout = atol(arg);
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Sets the connection timeout in seconds when fetching content for analysis and caching");
      break;

    case hash_clustering:
      _clustering = static_cast<bool>(atoi(arg));
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Enables the clustering from the UI");
      break;

    case hash_max_expansions:
      _max_expansions = atoi(arg);
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Sets the maximum number of query expansions");
      break;

    case hash_extended_highlight:
      _extended_highlight = static_cast<bool>(atoi(arg));
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Enables a more discriminative word highlight scheme");
      break;

    case hash_background_proxy:
      _background_proxy_addr = std::string(arg);
      miscutil::tokenize(_background_proxy_addr,bpvec,":");
      if (bpvec.size()!=2)
        {
          errlog::log_error(LOG_LEVEL_ERROR, "wrong address:port for background proxy: %s",_background_proxy_addr.c_str());
          _background_proxy_addr = "";
        }
      else
        {
          _background_proxy_addr = bpvec.at(0);
          _background_proxy_port = atoi(bpvec.at(1).c_str());
        }
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Background proxy for fetching URLs");
      break;

    case hash_show_node_ip:
      _show_node_ip = static_cast<bool>(atoi(arg));
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Enable rendering of the node IP address in the info bar");
      break;

    case hash_personalization:
      _personalization = static_cast<bool>(atoi(arg));
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Enable personalized result ranking");
      break;

    case hash_result_message:
      if (!arg)
        break;
      _result_message = std::string(arg);
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Message to appear in a panel next to the search results");
      break;

    case hash_dyn_ui:
      _dyn_ui = static_cast<bool>(atoi(arg));
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Enabled the dynamic UI");
      break;

    case hash_ui_theme:
      _ui_theme = std::string(arg);
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "User Interface selected theme");
      break;

    case hash_num_reco_queries:
      _num_reco_queries = atoi(arg);
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Max number of recommended queries");
      break;

    case hash_num_recent_queries:
      _num_recent_queries = atoi(arg);
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Max number of recent queries");
      break;

    case hash_cross_query_ri:
      _cross_query_ri = static_cast<bool>(atoi(arg));
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Whether to inject results from one query into results of another");
      break;

    case hash_max_summary_length:
      _max_summary_length = atoi(arg);
      configuration_spec::html_table_row(_config_args,cmd,arg,
                                         "Maximum result snippet summary length");
      break;

    default:
      break;

  } // end of switch.
}
*/
