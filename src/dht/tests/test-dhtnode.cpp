/**
 * This is the p2p messaging component of the Seeks project,
 * a collaborative websearch overlay network.
 *
 * Copyright (C) 2006, 2010  Emmanuel Benazera, juban@free.fr
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

#include "dht_api.h"
#include "miscutil.h"
#include "errlog.h"
#include "Transport.h"
#include "l1_protob_rpc_server.h"

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

using namespace dht;
using sp::miscutil;
using sp::errlog;

const char *usage = "Usage: test_dhtnode <ip:port> (--join ip:port) or (--self-bootstrap) (--persist) (--config config_file) (--nvnodes number_of_virtual_nodes)\n";

bool persistence = false;
DHTNode *dnode = NULL;

void sig_handler(int the_signal)
{
  switch(the_signal)
    {
    case SIGTERM:
    case SIGINT:
    case SIGHUP:
      dnode->leave();
      delete dnode; // hibernates, stop threads and destroys internal structures.
      exit(the_signal);
      break;
    default:
      std::cerr << "sig_handler: exiting on unexpected signal " << the_signal << std::endl;
    }
  return;
}

int main(int argc, char **argv)
{
  if (argc < 3)
    {
      std::cout << usage;
      exit(0);
    }

  char *net_addr = argv[1];
  char *vec1[2];
  miscutil::ssplit(net_addr,":",vec1,SZ(vec1),0,0);
  net_addr = vec1[0];
  int net_port = atoi(vec1[1]);

  // init logging module.
  errlog::init_log_module();
  errlog::set_debug_level(LOG_LEVEL_ERROR | LOG_LEVEL_DHT);

  int nvnodes = -1;
  bool joinb = false;
  bool sbootb = false;
  if (argc > 2)
    {
      std::vector<NetAddress> bootstrap_nodelist;
      int i=2;
      while(i<argc)
        {
          const char *arg = argv[i];
          if (strcmp(arg,"--join")==0)
            {
              joinb = true;
              char *bootnode = argv[i+1];
              char* vec[2];
              miscutil::ssplit(bootnode,":",vec,SZ(vec),0,0);
              int bootstrap_port = atoi(vec[1]);
              std::cout << "bootstrap node at ip: " << vec[0] << " -- port: " << bootstrap_port << std::endl;
              NetAddress bootstrap_na(vec[0],bootstrap_port);
              bootstrap_nodelist.push_back(bootstrap_na);
              i+=2;
            }
          else if (strcmp(arg,"--self-bootstrap")==0)
            {
              sbootb = true;
              i++;
            }
          else if (strcmp(arg,"--persist") == 0)
            {
              persistence = true;
              i++;
            }
          else if (strcmp(arg,"--config") == 0)
            {
              DHTNode::_dht_config_filename = argv[i+1];
              std::cout << "dht config: " << DHTNode::_dht_config_filename << std::endl;
              i+=2;
            }
          else if (strcmp(arg,"--nvnodes") == 0)
            {
              nvnodes = atoi(argv[i+1]);
              i+=2;
            }
          else
            {
              std::cout << usage << std::endl;
              exit(0);
            }
        }

      dht_configuration::_dht_config = new dht_configuration(DHTNode::_dht_config_filename);
      if (nvnodes > 0)
        dht_configuration::_dht_config->_nvnodes = nvnodes;
      dnode = new DHTNode(net_addr,net_port,true);

      /**
       * unix signal handling for graceful termination.
       */
      const int catched_signals[] = { SIGTERM, SIGINT, SIGHUP, 0 };
      for (int idx=0; catched_signals[idx] != 0; idx++)
        {
          if (signal(catched_signals[idx],&sig_handler) == SIG_ERR)
            {
              std::cerr << "Can't set signal-handler value for signal "
                        << catched_signals[idx] << std::endl;
            }
        }

      /**
       * Join an existing ring or perform self-bootstrap.
       */
      if (joinb)
        {
          bool reset = true;
          dht_api::join_start(*dnode,bootstrap_nodelist,reset);
        }
      else if (sbootb)
        dht_api::self_bootstrap(*dnode);
    }

  pthread_join(dnode->_transport->_l1_server->_rpc_server_thread,NULL);
}
