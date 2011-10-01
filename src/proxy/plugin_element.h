/**
 * The Seeks proxy and plugin framework are part of the SEEKS project.
 * Copyright (C) 2009, 2010 Emmanuel Benazera, ebenazer@seeks-project.info
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#ifndef PLUGIN_ELEMENT_H
#define PLUGIN_ELEMENT_H

#include "plugin.h"
#include "proxy_dts.h"
#include "mutexes.h"
#include "pcrs.h"

#if defined(HAVE_PERL) && !defined(NO_PERL)
#include <EXTERN.h>
#include <perl.h>
#endif

#include <string>
#include <vector>

namespace sp
{
  /**
   * \brief modular element for building a Seeks plugin. Each plugin element
   *        can be activated and respond to a set of regexp patterns. Positive
   *        patterns define the activation rules per se. Negative pattters are
   *        convenient when positive patterns are not bullet-proof, or to negate
   *        certain rules easily.
   *
   * Pattern file format:
   *   * '+'\n followed by lines of regexp patterns indicates positive patterns.
   *   * '-'\n followed by lines of regexp patterns indicates negative patterns.
   */
  class plugin_element
  {
    public:
      /**
       * \brief constructor for plugin element.
       *        Compiles patterns from strings.
       * @param pos_patterns are patterns (e.g. URI patterns) that activate the plugin.
       * @param neg_patterns are patterns the plugin should not be activated by
       *        (default is everything but what is in the positive patterns set, but
       *        negative patterns are sometimes useful).
       * @param parent the plugin object this element belongs to.
       */
      plugin_element(const std::vector<std::string> &pos_patterns,
                     const std::vector<std::string> &neg_patterns,
                     plugin *parent);

      /**
       * \brief constructor for plugin element.
       *        Activation patterns come in a file.
       * @param pattern_filename the the pattern file, see plugin_element class description
       *        for pattern file format.
       * @param parent the plugin object this element belongs to.
       */
      plugin_element(const char *pattern_filename,
                     plugin *parent); // read and compile patterns from file.

      /**
       * \brief constructor for plugin element.
       * @param pos_patterns are compiled patterns (e.g. URI patterns) that activate the plugin.
       * @param neg_patterns are compiled patterns the plugin should not be activated by
       *        (default is everything but what is in the positive patterns set, but
       *        negative patterns are sometimes useful).
       * @param parent the plugin object this element belongs to.
       */
      plugin_element(const std::vector<url_spec*> &pos_patterns,
                     const std::vector<url_spec*> &neg_patterns,
                     plugin *parent);

      /**
       * \brief constructor with code (e.g. Perl) in an external file.
       * @param  pos_patterns are patterns (e.g. URI patterns) that activate the plugin.
       * @param neg_patterns are patterns the plugin should not be activated by
       *        (default is everything but what is in the positive patterns set, but
       *        negative patterns are sometimes useful).
       * @param code_filename filename with plugin code.
       * @param pcrs true if submitted code can be compiled with pcrs.
       * @param perl true if submitted code is written in Perl.
       * @param parent the plugin object this element belongs to.
       */
      plugin_element(const std::vector<std::string> &pos_patterns,
                     const std::vector<std::string> &neg_patterns,
                     const char *code_filename,
                     const bool &pcrs, const bool &perl,
                     plugin *parent);


      /**
       * \brief constructor with code (e.g. Perl) in an external file.
       * @param  pos_patterns are patterns (e.g. URI patterns) that activate the plugin.
       * @param pattern_filename the the pattern file, see plugin_element class description
       *        for pattern file format.
       * @param code_filename filename with plugin code.
       * @param pcrs true if submitted code can be compiled with pcrs.
       * @param perl true if submitted code is written in Perl.
       * @param parent the plugin object this element belongs to.
       */
      plugin_element(const char *pattern_filename,
                     const char *code_filename,
                     const bool &pcrs, const bool &perl,
                     plugin *parent);
      /**
       * \brief destructor.
       */
      virtual ~plugin_element();

    protected:
      /**
       * \brief destroys compiled pattern objects.
       */
      void clear_patterns();

    public:
      /**
       * \brief compiles patterns from regexp strings.
       * @param patterns pattern strings.
       * @param c_patterns compiled patterns in memory.
       * @see url_spec.
       */
      void compile_patterns(const std::vector<std::string> &patterns,
                            std::vector<url_spec*> &c_patterns);

      /**
       * \brief loads pattern file content.
       * @return SP_ERR_OK if no error.
       */
      sp_err load_pattern_file();

      /**
       * \brief loads code file content.
       * @return SP_ERR_OK if no error.
       */
      sp_err load_code_file();

      /**
       * \brief reloads both pattern and code files (if any).
       * @return true if everything went right, false otherwise.
       */
      bool reload();

      /**
       * \brief prints plugin element's information.
       * @return string.
       */
      virtual std::string print()
      {
        return "";
      };

      /**
       * \brief matches a HTTP header with this plugin element's compiled patterns.
       * @return true if it matches, false otherwise.
       */
      bool match_url(const http_request *http);

      std::vector<url_spec*> _pos_patterns; /**< (positive) patterns that activate the plugin. */
      std::vector<url_spec*> _neg_patterns; /**< (negative) patterns that deactivate the plugin. */

      plugin *_parent; /**< parent plugin. */

      /* two paths: pcrs or full Perl interpreter. */ // TODO: actually 3, with C/C++ path.
    protected:
      bool _pcrs;  /**< whether this plugin element uses compiled pcrs patterns. */
      bool _perl;  /**< whether this plugin element embeds Perl code. */

      //1- pcrs.
    protected:
      /**
       * \brief loads and tries to compile pcrs code.
       * @param buf buffer containing a regexp pattern.
       * @param lastjob a pointer to the last compiled job, building up a linked list of jobs.
       */
      void pcrs_load_code(char *buf, pcrs_job *lastjob);

      /**
       * \brief load a pcrs regexp pattern file.
       * @return 0 if no error, -1 otherwise.
       */
      int pcrs_load_code_file();

      /**
       * \brief compiles regexp patterns as jobs dynamically when needed given a HTTP request.
       * @param csp the state object containing the HTTP request.
       * @return a list of compiled jobs.
       */
      pcrs_job* compile_dynamic_pcrs_job_list(const client_state *csp); // compilation of dynamic jobs.
    public:
      char* pcrs_plugin_response(client_state *csp, char *old);

    protected:
      std::list<const char*> _job_patterns; /**< lines of regexp code. */
      pcrs_job *_joblist; /**< The resulting compiled pcrs_jobs. */
      bool _is_dynamic; /**< whether at least one of the pcrs jobs is dynamic. */

#if defined(HAVE_PERL) && !defined(NO_PERL)
    public:
      //2- Perl.
      PerlInterpreter *_perl_interpreter;  // single interpreter.
      // XXX: multiple jobs can be handled through the call of Perl subroutines.

      /**
       * \brief loads a Perl code file.
       */
      int perl_load_code_file();

      /**
       * \brief execute a Perl routine with an embedded Perl interpreter.
       * @param subr_name routine name.
       * @param str result string to put the result in.
       * @returns str as containing the result.
       */
      char** perl_execute_subroutine(const std::string &subr_name, char **str);
#endif

      // XXX: not sure we need these variables here, but in the children classes instead.
      char* _pattern_filename; /**< plugin patterns file. */
      char* _code_filename; /**< plugin code in perl or perl regex syntax. */

#if defined(FEATURE_PTHREAD) || defined(_WIN32)
      /* thread safe. */
      sp_mutex_t _ple_mutex;
#endif
  };

} /* end of namespace. */

#endif
