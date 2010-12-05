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
 *                Written by and Copyright (C) 2001 the SourceForge
 *                Privoxy team. http://www.privoxy.org/
 *
 *                Based on the Internet Junkbuster originally written
 *                by and Copyright (C) 1997 Anonymous Coders and
 *                Junkbusters Corporation.  http://www.junkbusters.com
 *
 *                This program is free software; you can redistribute it
 *                and/or modify it under the terms of the GNU General
 *                Public License as published by the Free Software
 *                Foundation; either version 2 of the License, or (at
 *                your option) any later version.
 *
 *                This program is distributed in the hope that it will
 *                be useful, but WITHOUT ANY WARRANTY; without even the
 *                implied warranty of MERCHANTABILITY or FITNESS FOR A
 *                PARTICULAR PURPOSE.  See the GNU General Public
 *                License for more details.
 *
 *                The GNU General Public License should be included with
 *                this file.  If not, you can view it at
 *                http://www.gnu.org/copyleft/gpl.html
 *                or write to the Free Software Foundation, Inc., 59
 *                Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *********************************************************************/

#ifndef PCRS_H
#define PCRS_H

#include <pcre.h>

namespace sp
{

  /*
   * Constants:
   */

#define FALSE 0
#define TRUE 1

  /* Capacity */
#define PCRS_MAX_SUBMATCHES  33     /* Maximum number of capturing subpatterns allowed. MUST be <= 99! FIXME: Should be dynamic */
#define PCRS_MAX_MATCH_INIT  40     /* Initial amount of matches that can be stored in global searches */
#define PCRS_MAX_MATCH_GROW  1.6    /* Factor by which storage for matches is extended if exhausted */

  /*
   * PCRS error codes
   *
   * They are supposed to be handled together with PCRE error
   * codes and have to start with an offset to prevent overlaps.
   *
   * PCRE 6.7 uses error codes from -1 to -21, PCRS error codes
   * below -100 should be safe for a while.
   */
#define PCRS_ERR_NOMEM           -100      /* Failed to acquire memory. */
#define PCRS_ERR_CMDSYNTAX       -101      /* Syntax of s///-command */
#define PCRS_ERR_STUDY           -102      /* pcre error while studying the pattern */
#define PCRS_ERR_BADJOB          -103      /* NULL job pointer, pattern or substitute */
#define PCRS_WARN_BADREF         -104      /* Backreference out of range */
#define PCRS_WARN_TRUNCATION     -105      /* At least one pcrs variable was too big,
  * only the first part was used. */
  /* Flags */
#define PCRS_GLOBAL          1      /* Job should be applied globally, as with perl's g option */
#define PCRS_TRIVIAL         2      /* Backreferences in the substitute are ignored */
#define PCRS_SUCCESS         4      /* Job did previously match */

  /*
   * Data types:
   */

  /* A compiled substitute */
  class pcrs_substitute
  {
    public:
      pcrs_substitute()
          :_text(NULL),_length(0),_backrefs(0),_block_offset(),
          _block_length(),_backref(),_backref_count()
      {};

      ~pcrs_substitute() {}; //TODO.

      char  *_text;                                   /* The plaintext part of the substitute, with all backreferences stripped */
      size_t _length;                                 /* The substitute may not be a valid C string so we can't rely on strlen(). */
      int    _backrefs;                               /* The number of backreferences */
      int    _block_offset[PCRS_MAX_SUBMATCHES];      /* Array with the offsets of all plaintext blocks in text */
      size_t _block_length[PCRS_MAX_SUBMATCHES];      /* Array with the lengths of all plaintext blocks in text */
      int    _backref[PCRS_MAX_SUBMATCHES];           /* Array with the backref number for all plaintext block borders */
      int    _backref_count[PCRS_MAX_SUBMATCHES + 2]; /* Array with the number of references to each backref index */
  };

  /*
   * A match, including all captured subpatterns (submatches)
   * Note: The zeroth is the whole match, the PCRS_MAX_SUBMATCHES + 0th
   * is the range before the match, the PCRS_MAX_SUBMATCHES + 1th is the
   * range after the match.
   */
  class pcrs_match
  {
    public:
      pcrs_match()
          :_submatches(0),_submatch_offset(),
          _submatch_length()
      {
        for (int i=0; i<PCRS_MAX_SUBMATCHES+2; i++)
          {
            _submatch_offset[i] = 0;
            _submatch_length[i] = 0;
          }
      };

      ~pcrs_match() {};

      int    _submatches;                               /* Number of captured subpatterns */
      int    _submatch_offset[PCRS_MAX_SUBMATCHES + 2]; /* Offset for each submatch in the subject */
      size_t _submatch_length[PCRS_MAX_SUBMATCHES + 2]; /* Length of each submatch in the subject */
  };


  /* A PCRS job */
  class pcrs_job
  {
    public:
      pcrs_job()
          :_pattern(NULL),_hints(NULL),_options(0),
          _flags(0),_substitute(NULL),_next(NULL)
      {};

      ~pcrs_job();

      static pcrs_job* pcrs_free_job(pcrs_job *job);
      static void pcrs_free_joblist(pcrs_job *job);

      pcre *_pattern;                            /* The compiled pcre pattern */
      pcre_extra *_hints;                        /* The pcre hints for the pattern */
      int _options;                              /* The pcre options (numeric) */
      int _flags;                                /* The pcrs and user flags (see "Flags" above) */
      pcrs_substitute *_substitute;              /* The compiled pcrs substitute */
      pcrs_job *_next;                    /* Pointer for chaining jobs to joblists */
  };


  /*
   * Prototypes:
   */
  class pcrs
  {
    public:
      /* Main usage */
      static pcrs_job* pcrs_compile_command(const char *command, int *errptr);
      static pcrs_job* pcrs_compile(const char *pattern, const char *substitute,
                                    const char *options, int *errptr);
      static int pcrs_execute(pcrs_job *job, const char *subject, size_t subject_length,
                              char **result, size_t *result_length);
      static int pcrs_execute_list(pcrs_job *joblist, char *subject, size_t subject_length,
                                   char **result, size_t *result_length);

      static int pcrs_job_is_dynamic(char *job);
      static char pcrs_get_delimiter(const char *string);
      static char *pcrs_execute_single_command(const char *subject, const char *pcrs_command, int *hits);

      static pcrs_job *pcrs_compile_dynamic_command(char *pcrs_command, const struct pcrs_variable v[], int *error);

      /* Info on errors: */
      static const char *pcrs_strerror(const int error);

    private:
      static int pcrs_parse_perl_options(const char *optstring, int *flags);
      static pcrs_substitute* pcrs_compile_replacement(const char *replacement, int trivialflag,
          int capturecount, int *errptr);
      static int is_hex_sequence(const char *sequence);

  };

  /*
   * Variable/value pair for dynamic pcrs commands.
   */
  class pcrs_variable
  {
    public:
      pcrs_variable()
          :_name(NULL),_value(NULL),_static_value(0)
      {};

      pcrs_variable(const char *name, char *value,
                    const int &static_value)
          :_name(name),_value(value),_static_value(static_value)
      {};

      ~pcrs_variable() {}; // TODO.

      const char *_name;
      char *_value;
      int _static_value;
  };

  /* Only relevant for maximum pcrs variable size */
#ifndef PCRS_BUFFER_SIZE
#define PCRS_BUFFER_SIZE 4000
#endif /* ndef PCRS_BUFFER_SIZE */

} /* end of namespace. */

#endif /* ndef PCRS_H */

