/*********************************************************************
 * Purpose     :  pcrs is a supplement to the pcre library by Philip Hazel
 *                <ph10@cam.ac.uk> and adds Perl-style substitution. That
 *                is, it mimics Perl's 's' operator. See pcrs(3) for details.
 *
 *                WARNING: This file contains additional functions and bug
 *                fixes that aren't part of the latest official pcrs package
 *                (which apparently is no longer maintained).
 *
 * Copyright   :  Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 *
 *                Written and Copyright (C) 2000, 2001 by Andreas S. Oesterhelt
 *                <andreas@oesterhelt.org>
 *
 *                Copyright (C) 2006, 2007 Fabian Keil <fk@fabiankeil.de>
 *
 *                This program is free software; you can redistribute it
 *                and/or modify it under the terms of the GNU Lesser
 *                General Public License (LGPL), version 2.1, which  should
 *                be included in this distribution (see LICENSE.txt), with
 *                the exception that the permission to replace that license
 *                with the GNU General Public License (GPL) given in section
 *                3 is restricted to version 2 of the GPL.
 *
 *                This program is distributed in the hope that it will
 *                be useful, but WITHOUT ANY WARRANTY; without even the
 *                implied warranty of MERCHANTABILITY or FITNESS FOR A
 *                PARTICULAR PURPOSE.  See the license for more details.
 *
 *                The GNU Lesser General Public License should be included
 *                with this file.  If not, you can view it at
 *                http://www.gnu.org/licenses/lgpl.html
 *                or write to the Free Software Foundation, Inc., 59
 *                Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *********************************************************************/

#include "pcrs.h"

#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>

#include <pcre.h>

#include "mem_utils.h"
/* For xtoi */
#include "encode.h"

#include <algorithm> // std::copy.

namespace sp
{

  /*********************************************************************
   *
   * Function    :  pcrs_strerror
   *
   * Description :  Return a string describing a given error code.
   *
   * Parameters  :
   *          1  :  error = the error code
   *
   * Returns     :  char * to the descriptive string
   *
   *********************************************************************/
  const char* pcrs::pcrs_strerror(const int error)
  {
    if (error < 0)
      {
        switch (error)
          {
            /* Passed-through PCRE error: */
          case PCRE_ERROR_NOMEMORY:
            return "(pcre:) No memory";

            /* Shouldn't happen unless PCRE or PCRS bug, or user messed with compiled job: */
          case PCRE_ERROR_NULL:
            return "(pcre:) NULL code or subject or ovector";
          case PCRE_ERROR_BADOPTION:
            return "(pcre:) Unrecognized option bit";
          case PCRE_ERROR_BADMAGIC:
            return "(pcre:) Bad magic number in code";
          case PCRE_ERROR_UNKNOWN_NODE:
            return "(pcre:) Bad node in pattern";

            /* Can't happen / not passed: */
          case PCRE_ERROR_NOSUBSTRING:
            return "(pcre:) Fire in power supply";
          case PCRE_ERROR_NOMATCH:
            return "(pcre:) Water in power supply";

#ifdef PCRE_ERROR_MATCHLIMIT
            /*
             * Only reported by PCRE versions newer than our own.
             */
          case PCRE_ERROR_MATCHLIMIT:
            return "(pcre:) Match limit reached";
#endif /* def PCRE_ERROR_MATCHLIMIT */

            /* PCRS errors: */
          case PCRS_ERR_NOMEM:
            return "(pcrs:) No memory";
          case PCRS_ERR_CMDSYNTAX:
            return "(pcrs:) Syntax error while parsing command";
          case PCRS_ERR_STUDY:
            return "(pcrs:) PCRE error while studying the pattern";
          case PCRS_ERR_BADJOB:
            return "(pcrs:) Bad job - NULL job, pattern or substitute";
          case PCRS_WARN_BADREF:
            return "(pcrs:) Backreference out of range";
          case PCRS_WARN_TRUNCATION:
            return "(pcrs:) At least one variable was too big and has been truncated before compilation";

            /*
             * XXX: With the exception of PCRE_ERROR_MATCHLIMIT we
             * only catch PCRE errors that can happen with our internal
             * version. If Privoxy is linked against a newer
             * PCRE version all bets are off ...
             */
          default:
            return "Unknown error. Privoxy out of sync with PCRE?";
          }
      }
    /* error >= 0: No error */
    return "(pcrs:) Everything's just fine. Thanks for asking.";
  }


  /*********************************************************************
   *
   * Function    :  pcrs_parse_perl_options
   *
   * Description :  This function parses a string containing the options to
   *                Perl's s/// operator. It returns an integer that is the
   *                pcre equivalent of the symbolic optstring.
   *                Since pcre doesn't know about Perl's 'g' (global) or pcrs',
   *                'T' (trivial) options but pcrs needs them, the corresponding
   *                flags are set if 'g'or 'T' is encountered.
   *                Note: The 'T' and 'U' options do not conform to Perl.
   *
   * Parameters  :
   *          1  :  optstring = string with options in perl syntax
   *          2  :  flags = see description
   *
   * Returns     :  option integer suitable for pcre
   *
   *********************************************************************/
  int pcrs::pcrs_parse_perl_options(const char *optstring, int *flags)
  {
    size_t i;
    int rc = 0;
    *flags = 0;

    if (NULL == optstring) return 0;

    for (i = 0; i < strlen(optstring); i++)
      {
        switch (optstring[i])
          {
          case 'e':
            break; /* ToDo ;-) */
          case 'g':
            *flags |= PCRS_GLOBAL;
            break;
          case 'i':
            rc |= PCRE_CASELESS;
            break;
          case 'm':
            rc |= PCRE_MULTILINE;
            break;
          case 'o':
            break;
          case 's':
            rc |= PCRE_DOTALL;
            break;
          case 'x':
            rc |= PCRE_EXTENDED;
            break;
          case 'U':
            rc |= PCRE_UNGREEDY;
            break;
          case 'T':
            *flags |= PCRS_TRIVIAL;
            break;
          default:
            break;
          }
      }
    return rc;
  }


  /*********************************************************************
   *
   * Function    :  pcrs_compile_replacement
   *
   * Description :  This function takes a Perl-style replacement (2nd argument
   *                to the s/// operator and returns a compiled pcrs_substitute,
   *                or NULL if memory allocation for the substitute structure
   *                fails.
   *
   * Parameters  :
   *          1  :  replacement = replacement part of s/// operator
   *                              in perl syntax
   *          2  :  trivialflag = Flag that causes backreferences to be
   *                              ignored.
   *          3  :  capturecount = Number of capturing subpatterns in
   *                               the pattern. Needed for $+ handling.
   *          4  :  errptr = pointer to an integer in which error
   *                         conditions can be returned.
   *
   * Returns     :  pcrs_substitute data structure, or NULL if an
   *                error is encountered. In that case, *errptr has
   *                the reason.
   *
   *********************************************************************/
  pcrs_substitute* pcrs::pcrs_compile_replacement(const char *replacement, int trivialflag, int capturecount, int *errptr)
  {
    int i, k, l, quoted;
    size_t length;
    char *text;
    pcrs_substitute *r;

    i = k = l = quoted = 0;

    /*
     * Sanity check
     */
    if (NULL == replacement)
      {
        replacement = "";
      }

    /*
     * Get memory or fail
     */
    if (NULL == (r = new pcrs_substitute()))
      {
        *errptr = PCRS_ERR_NOMEM;
        return NULL;
      }

    length = strlen(replacement);

    if (NULL == (text = (char*) zalloc(length+1)))
      {
        delete r;
        *errptr = PCRS_ERR_NOMEM;
        return NULL;
      }
    memset(text, '\0', length + 1);

    /*
     * In trivial mode, just copy the substitute text
     */
    if (trivialflag)
      {
        text = strncpy(text, replacement, length + 1);
        k = (int)length;
      }
    /*
     * Else, parse, cut out and record all backreferences
     */
    else
      {
        while (i < (int)length)
          {
            /* Quoting */
            if (replacement[i] == '\\')
              {
                if (quoted)
                  {
                    text[k++] = replacement[i++];
                    quoted = 0;
                  }
                else
                  {
                    if (replacement[i+1] && strchr("tnrfae0", replacement[i+1]))
                      {
                        switch (replacement[++i])
                          {
                          case 't':
                            text[k++] = '\t';
                            break;
                          case 'n':
                            text[k++] = '\n';
                            break;
                          case 'r':
                            text[k++] = '\r';
                            break;
                          case 'f':
                            text[k++] = '\f';
                            break;
                          case 'a':
                            text[k++] = 7;
                            break;
                          case 'e':
                            text[k++] = 27;
                            break;
                          case '0':
                            text[k++] = '\0';
                            break;
                          }
                        i++;
                      }
                    else if (pcrs::is_hex_sequence(&replacement[i]))
                      {
                        /*
                         * Replace a hex sequence with a single
                         * character with the sequence's ascii value.
                         * e.g.: '\x7e' => '~'
                         */
                        //const
                        int ascii_value = encode::xtoi(&replacement[i+2]);

                        /* assert(ascii_value > 0);
                        assert(ascii_value < 256); */

                        //hack: we don't want the proxy to exit on a substitution error,
                        //      I prefer to return garbage (and file a bug).
                        if (ascii_value < 0 || ascii_value > 256)
                          ascii_value = 1;
                        //hack

                        text[k++] = (char)ascii_value;
                        i += 4;
                      }
                    else
                      {
                        quoted = 1;
                        i++;
                      }
                  }
                continue;
              }

            /* Backreferences */
            if (replacement[i] == '$' && !quoted && i < (int)(length - 1))
              {
                char *symbol, symbols[] = "'`+&";
                r->_block_length[l] = (size_t)(k - r->_block_offset[l]);

                /* Numerical backreferences */
                if (isdigit((int)replacement[i + 1]))
                  {
                    while (i < (int)length && isdigit((int)replacement[++i]))
                      {
                        r->_backref[l] = r->_backref[l] * 10 + replacement[i] - 48;
                      }
                    if (r->_backref[l] > capturecount)
                      {
                        *errptr = PCRS_WARN_BADREF;
                      }
                  }

                /* Symbolic backreferences: */
                else if (NULL != (symbol = strchr(symbols, replacement[i + 1])))
                  {
                    if (symbol - symbols == 2) /* $+ */
                      {
                        r->_backref[l] = capturecount;
                      }
                    else if (symbol - symbols == 3) /* $& */
                      {
                        r->_backref[l] = 0;
                      }
                    else /* $' or $` */
                      {
                        r->_backref[l] = PCRS_MAX_SUBMATCHES + 1 - (symbol - symbols);
                      }
                    i += 2;
                  }
                /* Invalid backref -> plain '$' */
                else
                  {
                    goto plainchar;
                  }

                /* Valid and in range? -> record */
                if (r->_backref[l] < PCRS_MAX_SUBMATCHES + 2)
                  {
                    r->_backref_count[r->_backref[l]] += 1;
                    r->_block_offset[++l] = k;
                  }
                else
                  {
                    *errptr = PCRS_WARN_BADREF;
                  }
                continue;
              }

plainchar:
            /* Plain chars are copied */
            text[k++] = replacement[i++];
            quoted = 0;
          }
      } /* -END- if (!trivialflag) */

    /*
     * Finish & return
     */
    r->_text = text;
    r->_backrefs = l;
    r->_length = (size_t)k;
    r->_block_length[l] = (size_t)(k - r->_block_offset[l]);

    return r;
  }

  /*-- pcrs_job --*/
  pcrs_job::~pcrs_job()
  {
    if (_pattern) freez(_pattern);
    if (_hints) freez(_hints);
    if (_substitute)
      {
        if (_substitute->_text) freez(_substitute->_text);
        delete _substitute;
      }
  }

  /*********************************************************************
   *
   * Function    :  pcrs_free_job
   *
   * Description :  Frees the memory used by a pcrs_job struct and its
   *                dependant structures.
   *
   * Parameters  :
   *          1  :  job = pointer to the pcrs_job structure to be freed
   *
   * Returns     :  a pointer to the next job, if there was any, or
   *                NULL otherwise.
   *
   *********************************************************************/
  pcrs_job* pcrs_job::pcrs_free_job(pcrs_job *job)
  {
    pcrs_job *next = NULL;

    if (job == NULL)
      {
        return NULL;
      }
    else
      {
        next = job->_next;
        delete job;
      }
    return next;
  }


  /*********************************************************************
   *
   * Function    :  pcrs_free_joblist
   *
   * Description :  Iterates through a chained list of pcrs_job's and
   *                frees them using pcrs_free_job.
   *
   * Parameters  :
   *          1  :  joblist = pointer to the first pcrs_job structure to
   *                be freed
   *
   * Returns     :  N/A
   *
   *********************************************************************/
  void pcrs_job::pcrs_free_joblist(pcrs_job *joblist)
  {
    while ( NULL != (joblist = pcrs_job::pcrs_free_job(joblist)) ) {};

    return;
  }

  /*********************************************************************
   *
   * Function    :  pcrs_compile_command
   *
   * Description :  Parses a string with a Perl-style s/// command,
   *                calls pcrs_compile, and returns a corresponding
   *                pcrs_job, or NULL if parsing or compiling the job
   *                fails.
   *
   * Parameters  :
   *          1  :  command = string with perl-style s/// command
   *          2  :  errptr = pointer to an integer in which error
   *                         conditions can be returned.
   *
   * Returns     :  a corresponding pcrs_job data structure, or NULL
   *                if an error was encountered. In that case, *errptr
   *                has the reason.
   *
   *********************************************************************/
  pcrs_job* pcrs::pcrs_compile_command(const char *command, int *errptr)
  {
    int i, k, l, quoted = FALSE;
    size_t limit;
    char delimiter;
    char *tokens[4];
    pcrs_job *newjob;

    k = l = 0;

    /*
     * Tokenize the perl command
     */
    limit = strlen(command);
    if (limit < 4)
      {
        *errptr = PCRS_ERR_CMDSYNTAX;
        return NULL;
      }
    else
      {
        delimiter = command[1];
      }

    tokens[l] = (char *) malloc(limit + 1);

    for (i = 0; i <= (int)limit; i++)
      {

        if (command[i] == delimiter && !quoted)
          {
            if (l == 3)
              {
                l = -1;
                break;
              }
            tokens[0][k++] = '\0';
            tokens[++l] = tokens[0] + k;
            continue;
          }

        else if (command[i] == '\\' && !quoted)
          {
            quoted = TRUE;
            if (command[i+1] == delimiter) continue;
          }
        else
          {
            quoted = FALSE;
          }
        tokens[0][k++] = command[i];
      }

    /*
     * Syntax error ?
     */
    if (l != 3)
      {
        *errptr = PCRS_ERR_CMDSYNTAX;
        freez(tokens[0]);
        return NULL;
      }

    newjob = pcrs::pcrs_compile(tokens[1], tokens[2], tokens[3], errptr);
    freez(tokens[0]);
    return newjob;
  }


  /*********************************************************************
   *
   * Function    :  pcrs_compile
   *
   * Description :  Takes the three arguments to a perl s/// command
   *                and compiles a pcrs_job structure from them.
   *
   * Parameters  :
   *          1  :  pattern = string with perl-style pattern
   *          2  :  substitute = string with perl-style substitute
   *          3  :  options = string with perl-style options
   *          4  :  errptr = pointer to an integer in which error
   *                         conditions can be returned.
   *
   * Returns     :  a corresponding pcrs_job data structure, or NULL
   *                if an error was encountered. In that case, *errptr
   *                has the reason.
   *
   *********************************************************************/
  pcrs_job* pcrs::pcrs_compile(const char *pattern, const char *substitute,
                               const char *options, int *errptr)
  {
    pcrs_job *newjob;
    int flags;
    int capturecount;
    const char *error;

    *errptr = 0;

    /*
     * Handle NULL arguments
     */
    if (pattern == NULL) pattern = "";
    if (substitute == NULL) substitute = "";

    /*
     * Get and init memory
     */
    if (NULL == (newjob = new pcrs_job()))
      {
        *errptr = PCRS_ERR_NOMEM;
        return NULL;
      }

    /*
     * Evaluate the options
     */
    newjob->_options = pcrs::pcrs_parse_perl_options(options, &flags);
    newjob->_flags = flags;

    /*
     * Compile the pattern
     */
    newjob->_pattern = pcre_compile(pattern, newjob->_options, &error, errptr, NULL);
    if (newjob->_pattern == NULL)
      {
        pcrs_job::pcrs_free_job(newjob);
        return NULL;
      }

    /*
     * Generate hints. This has little overhead, since the
     * hints will be NULL for a boring pattern anyway.
     */
    newjob->_hints = pcre_study(newjob->_pattern, 0, &error);
    if (error != NULL)
      {
        *errptr = PCRS_ERR_STUDY;
        pcrs_job::pcrs_free_job(newjob);
        return NULL;
      }

    /*
     * Determine the number of capturing subpatterns.
     * This is needed for handling $+ in the substitute.
     */
    if (0 > (*errptr = pcre_fullinfo(newjob->_pattern, newjob->_hints,
                                     PCRE_INFO_CAPTURECOUNT, &capturecount)))
      {
        pcrs_job::pcrs_free_job(newjob);
        return NULL;
      }

    /*
     * Compile the substitute
     */
    if (NULL == (newjob->_substitute = pcrs::pcrs_compile_replacement(substitute,
                                       newjob->_flags & PCRS_TRIVIAL, capturecount, errptr)))
      {
        pcrs_job::pcrs_free_job(newjob);
        return NULL;
      }

    return newjob;
  }


  /*********************************************************************
   *
   * Function    :  pcrs_execute_list
   *
   * Description :  This is a multiple job wrapper for pcrs_execute().
   *                Apply the regular substitutions defined by the jobs in
   *                the joblist to the subject.
   *                The subject itself is left untouched, memory for the result
   *                is malloc()ed and it is the caller's responsibility to free
   *                the result when it's no longer needed.
   *
   *                Note: For convenient string handling, a null byte is
   *                      appended to the result. It does not count towards the
   *                      result_length, though.
   *
   *
   * Parameters  :
   *          1  :  joblist = the chained list of pcrs_jobs to be executed
   *          2  :  subject = the subject string
   *          3  :  subject_length = the subject's length
   *          4  :  result = char** for returning  the result
   *          5  :  result_length = size_t* for returning the result's length
   *
   * Returns     :  On success, the number of substitutions that were made.
   *                 May be > 1 if job->flags contained PCRS_GLOBAL
   *                On failure, the (negative) pcre error code describing the
   *                 failure, which may be translated to text using pcrs_strerror().
   *
   *********************************************************************/
  int pcrs::pcrs_execute_list(pcrs_job *joblist, char *subject, size_t subject_length,
                              char **result, size_t *result_length)
  {
    pcrs_job *job;
    char *old, *newstr = NULL;
    int hits, total_hits;

    old = subject;
    *result_length = subject_length;
    total_hits = 0;

    for (job = joblist; job != NULL; job = job->_next)
      {
        hits = pcrs::pcrs_execute(job, old, *result_length, &newstr, result_length);

        if (old != subject) freez(old);

        if (hits < 0)
          {
            return(hits);
          }
        else
          {
            total_hits += hits;
            old = newstr;
          }
      }

    *result = newstr;
    return(total_hits);
  }


  /*********************************************************************
   *
   * Function    :  pcrs_execute
   *
   * Description :  Apply the regular substitution defined by the job to the
   *                subject.
   *                The subject itself is left untouched, memory for the result
   *                is malloc()ed and it is the caller's responsibility to free
   *                the result when it's no longer needed.
   *
   *                Note: For convenient string handling, a null byte is
   *                      appended to the result. It does not count towards the
   *                      result_length, though.
   *
   * Parameters  :
   *          1  :  job = the pcrs_job to be executed
   *          2  :  subject = the subject (== original) string
   *          3  :  subject_length = the subject's length
   *          4  :  result = char** for returning  the result
   *          5  :  result_length = size_t* for returning the result's length
   *
   * Returns     :  On success, the number of substitutions that were made.
   *                 May be > 1 if job->flags contained PCRS_GLOBAL
   *                On failure, the (negative) pcre error code describing the
   *                 failure, which may be translated to text using pcrs_strerror().
   *
   *********************************************************************/
  int pcrs::pcrs_execute(pcrs_job *job, const char *subject, size_t subject_length,
                         char **result, size_t *result_length)
  {
    int offsets[3 * PCRS_MAX_SUBMATCHES],
    offset,
    i, k,
    matches_found,
    submatches,
    max_matches = PCRS_MAX_MATCH_INIT;
    size_t newsize;
    pcrs_match *matches, *dummy;
    char *result_offset;

    offset = i = 0;

    /*
     * Sanity check & memory allocation
     */
    if (job == NULL || job->_pattern == NULL || job->_substitute == NULL || NULL == subject)
      {
        *result = NULL;
        return(PCRS_ERR_BADJOB);
      }

    if (NULL == (matches = new pcrs_match[max_matches]))
      {
        *result = NULL;
        return(PCRS_ERR_NOMEM);
      }

    /*
     * Find the pattern and calculate the space
     * requirements for the result
     */
    newsize = subject_length;

    while ((submatches = pcre_exec(job->_pattern, job->_hints,
                                   subject, (int)subject_length, offset, 0, offsets, 3 * PCRS_MAX_SUBMATCHES)) > 0)
      {
        job->_flags |= PCRS_SUCCESS;
        matches[i]._submatches = submatches;

        for (k = 0; k < submatches; k++)
          {
            matches[i]._submatch_offset[k] = offsets[2 * k];

            /* Note: Non-found optional submatches have length -1-(-1)==0 */
            matches[i]._submatch_length[k] = (size_t)(offsets[2 * k + 1] - offsets[2 * k]);

            /* reserve mem for each submatch as often as it is ref'd */
            newsize += matches[i]._submatch_length[k] * (size_t)job->_substitute->_backref_count[k];
          }
        /* plus replacement text size minus match text size */
        newsize += job->_substitute->_length - matches[i]._submatch_length[0];

        /* chunk before match */
        matches[i]._submatch_offset[PCRS_MAX_SUBMATCHES] = 0;
        matches[i]._submatch_length[PCRS_MAX_SUBMATCHES] = (size_t)offsets[0];
        newsize += (size_t)offsets[0] * (size_t)job->_substitute->_backref_count[PCRS_MAX_SUBMATCHES];

        /* chunk after match */
        matches[i]._submatch_offset[PCRS_MAX_SUBMATCHES + 1] = offsets[1];
        matches[i]._submatch_length[PCRS_MAX_SUBMATCHES + 1] = subject_length - (size_t)offsets[1] - 1;
        newsize += (subject_length - (size_t)offsets[1]) * (size_t)job->_substitute->_backref_count[PCRS_MAX_SUBMATCHES + 1];

        /* Storage for matches exhausted? -> Extend! */
        if (++i >= max_matches)
          {
            int old_max_matches = max_matches;
            max_matches = (int)(max_matches * PCRS_MAX_MATCH_GROW);
            if (NULL == (dummy = new pcrs_match[max_matches]))
              {
                delete[] matches;
                *result = NULL;
                return(PCRS_ERR_NOMEM);
              }
            std::copy(matches,matches+old_max_matches,dummy);
            matches = dummy;
          }

        /* Non-global search or limit reached? */
        if (!(job->_flags & PCRS_GLOBAL)) break;

        /* Don't loop on empty matches */
        if (offsets[1] == offset)
          if ((size_t)offset < subject_length)
            offset++;
          else
            break;
        /* Go find the next one */
        else
          offset = offsets[1];
      }
    /* Pass pcre error through if (bad) failiure */
    if (submatches < PCRE_ERROR_NOMATCH)
      {
        delete[] matches;
        return submatches;
      }
    matches_found = i;

    /*
     * Get memory for the result (must be freed by caller!)
     * and append terminating null byte.
     */
    if ((*result = (char*) zalloc(newsize+1)) == NULL)
      {
        delete[] matches;
        return PCRS_ERR_NOMEM;
      }
    else
      {
        (*result)[newsize] = '\0';
      }

    /*
     * Replace
     */
    offset = 0;
    result_offset = *result;

    for (i = 0; i < matches_found; i++)
      {
        /* copy the chunk preceding the match */
        memcpy(result_offset, subject + offset, (size_t)(matches[i]._submatch_offset[0] - offset));
        result_offset += matches[i]._submatch_offset[0] - offset;

        /* For every segment of the substitute.. */
        for (k = 0; k <= job->_substitute->_backrefs; k++)
          {
            /* ...copy its text.. */
            memcpy(result_offset, job->_substitute->_text + job->_substitute->_block_offset[k],
                   job->_substitute->_block_length[k]);
            result_offset += job->_substitute->_block_length[k];

            /* ..plus, if it's not the last chunk, i.e.: There *is* a backref.. */
            if (k != job->_substitute->_backrefs
                /* ..in legal range.. */
                && job->_substitute->_backref[k] < PCRS_MAX_SUBMATCHES + 2
                /* ..and referencing a real submatch.. */
                && job->_substitute->_backref[k] < matches[i]._submatches
                /* ..that is nonempty.. */
                && matches[i]._submatch_length[job->_substitute->_backref[k]] > 0)
              {
                /* ..copy the submatch that is ref'd. */
                memcpy(
                  result_offset,
                  subject + matches[i]._submatch_offset[job->_substitute->_backref[k]],
                  matches[i]._submatch_length[job->_substitute->_backref[k]]
                );
                result_offset += matches[i]._submatch_length[job->_substitute->_backref[k]];
              }
          }
        offset =  matches[i]._submatch_offset[0] + (int)matches[i]._submatch_length[0];
      }

    /* Copy the rest. */
    memcpy(result_offset, subject + offset, subject_length - (size_t)offset);

    *result_length = newsize;
    delete[] matches;
    return matches_found;
  }

#define is_hex_digit(x) ((x) && strchr("0123456789ABCDEF", toupper(x)))

  /*********************************************************************
   *
   * Function    :  is_hex_sequence
   *
   * Description :  Checks the first four characters of a string
   *                and decides if they are a valid hex sequence
   *                (like '\x40').
   *
   * Parameters  :
   *          1  :  sequence = The string to check
   *
   * Returns     :  Non-zero if it's valid sequence, or
   *                Zero if it isn't.
   *
   *********************************************************************/
  int pcrs::is_hex_sequence(const char *sequence)
  {
    return (sequence[0] == '\\' &&
            sequence[1] == 'x'  &&
            is_hex_digit(sequence[2]) &&
            is_hex_digit(sequence[3]));
  }


  /*
   * Functions below this line are only part of the pcrs version
   * included in Privoxy (and Seeks proxy). If you use any of them you
   * should not try to dynamically link against external pcrs versions.
   */

  /*********************************************************************
   *
   * Function    :  pcrs_job_is_dynamic
   *
   * Description :  Checks if a job has the "D" (dynamic) option set.
   *
   * Parameters  :
   *          1  :  job = The job to check
   *
   * Returns     :  TRUE if the job is indeed dynamic, otherwise
   *                FALSE
   *
   *********************************************************************/
  int pcrs::pcrs_job_is_dynamic (char *job)
  {
    const char delimiter = job[1];
    const size_t length = strlen(job);
    char *option;

    if (length < 5)
      {
        /*
         * The shortest valid (but useless)
         * dynamic pattern is "s@@@D"
         */
        return FALSE;
      }

    /*
     * Everything between the last character
     * and the last delimiter is an option ...
     */
    for (option = job + length; *option != delimiter; option--)
      {
        if (*option == 'D')
          {
            /*
             * ... and if said option is 'D' the job is dynamic.
             */
            return TRUE;
          }
      }
    return FALSE;
  }


  /*********************************************************************
   *
   * Function    :  pcrs_get_delimiter
   *
   * Description :  Tries to find a character that is safe to
   *                be used as a pcrs delimiter for a certain string.
   *
   * Parameters  :
   *          1  :  string = The string to search in
   *
   * Returns     :  A safe delimiter if one was found, otherwise '\0'.
   *
   *********************************************************************/
  char pcrs::pcrs_get_delimiter(const char *string)
  {
    /*
     * Some characters that are unlikely to
     * be part of pcrs replacement strings.
     */
    char delimiters[] = "><§#+*~%^°-:;µ!@";
    char *d = delimiters;

    /* Take the first delimiter that isn't part of the string */
    while (*d && NULL != strchr(string, *d))
      {
        d++;
      }
    return *d;
  }

  /*********************************************************************
   *
   * Function    :  pcrs_execute_single_command
   *
   * Description :  Apply single pcrs command to the subject.
   *                The subject itself is left untouched, memory for the result
   *                is malloc()ed and it is the caller's responsibility to free
   *                the result when it's no longer needed.
   *
   * Parameters  :
   *          1  :  subject = the subject (== original) string
   *          2  :  pcrs_command = the pcrs command as string (s@foo@bar@)
   *          3  :  hits = int* for returning  the number of modifications
   *
   * Returns     :  NULL in case of errors, otherwise the
   *                result of the pcrs command.
   *
   *********************************************************************/
  char* pcrs::pcrs_execute_single_command(const char *subject,
                                          const char *pcrs_command, int *hits)
  {
    size_t size;
    char *result = NULL;
    pcrs_job *job;

    assert(subject);
    assert(pcrs_command);

    *hits = 0;
    size = strlen(subject);

    job = pcrs::pcrs_compile_command(pcrs_command, hits);
    if (NULL != job)
      {
        *hits = pcrs::pcrs_execute(job, subject, size, &result, &size);
        if (*hits < 0)
          {
            freez(result);
          }
        pcrs_job::pcrs_free_job(job);
      }
    return result;
  }

  static const char warning[] = "... [too long, truncated]";
  /*********************************************************************
   *
   * Function    :  pcrs_compile_dynamic_command
   *
   * Description :  Takes a dynamic pcrs command, fills in the
   *                values of the variables and compiles it.
   *
   * Parameters  :
   *          1  :  pcrs_command = The dynamic pcrs command to compile
   *          2  :  v = NULL terminated array of variables and their values.
   *          3  :  error = pcrs error code
   *
   * Returns     :  NULL in case of hard errors, otherwise the
   *                compiled pcrs job.
   *
   *********************************************************************/
  pcrs_job* pcrs::pcrs_compile_dynamic_command(char *pcrs_command,
      const pcrs_variable v[], int *error)
  {
    char buf[PCRS_BUFFER_SIZE];
    const char *original_pcrs_command = pcrs_command;
    char *pcrs_command_tmp = NULL;
    pcrs_job *job = NULL;
    int truncation = 0;
    char d;
    int ret;

    while ((NULL != v->_name) && (NULL != pcrs_command))
      {
        assert(NULL != v->_value);

        if (NULL == strstr(pcrs_command, v->_name))
          {
            /*
             * Skip the substitution if the variable
             * name isn't part of the pattern.
             */
            v++;
            continue;
          }

        /* Use pcrs to replace the variable with its value. */
        d = pcrs::pcrs_get_delimiter(v->_value);
        if ('\0' == d)
          {
            /* No proper delimiter found */
            *error = PCRS_ERR_CMDSYNTAX;
            return NULL;
          }

        /*
         * Variable names are supposed to contain alpha
         * numerical characters plus '_' only.
         */
        assert(NULL == strchr(v->_name, d));

        ret = snprintf(buf, sizeof(buf), "s%c\\$%s%c%s%cgT", d, v->_name, d, v->_value, d);
        assert(ret >= 0);
        if ((size_t)ret >= sizeof(buf))
          {
            /*
             * Value didn't completely fit into buffer,
             * overwrite the end of the substitution text
             * with a truncation message and close the pattern
             * properly.
             */
            const size_t trailer_size = sizeof(warning) + 3; /* 3 for d + "gT" */
            char *trailer_start = buf + sizeof(buf) - trailer_size;

            ret = snprintf(trailer_start, trailer_size, "%s%cgT", warning, d);
            assert(ret == trailer_size - 1);
            assert(sizeof(buf) == strlen(buf) + 1);
            truncation = 1;
          }

        pcrs_command_tmp = pcrs::pcrs_execute_single_command(pcrs_command, buf, error);
        if (NULL == pcrs_command_tmp)
          {
            return NULL;
          }

        if (pcrs_command != original_pcrs_command)
          {
            freez(pcrs_command);
          }
        pcrs_command = pcrs_command_tmp;

        v++;
      }

    job = pcrs::pcrs_compile_command(pcrs_command, error);
    if (pcrs_command != original_pcrs_command)
      {
        freez(pcrs_command);
      }

    if (truncation)
      {
        *error = PCRS_WARN_TRUNCATION;
      }

    return job;
  }

} /* end of namespace. */
