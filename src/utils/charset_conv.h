/**
 * This file is part of the SEEKS project.
 * Copyright (C) 2011 Emmanuel Benazera, ebenazer@seeks-project.info
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

#include "config.h"
#include <iconv.h>
#include <errno.h>

#ifdef FEATURE_ICU
#include "unicode/utypes.h"
#include "unicode/uenum.h"
#include "unicode/ucsdet.h"
#include "unicode/ucnv.h"
#endif

#include <iostream>

char* iconv_convert (const char *from_charset, const char *to_charset, const char *input)
{
  size_t inleft, outleft, converted = 0;
  char *output, *outbuf;
  const char *inbuf;
  size_t outlen;
  iconv_t cd;

  if ((cd = iconv_open (to_charset, from_charset)) == (iconv_t) -1)
    return NULL;

  inleft = strlen (input);
  inbuf = input;

  /* we'll start off allocating an output buffer which is the same size
   * as our input buffer. */
  outlen = 2*inleft+1;

  /* we allocate 4 bytes more than what we need for nul-termination... */
  if (!(output = (char*)malloc (outlen + 4))) {
    iconv_close (cd);
    return NULL;
  }

  errno = 0;
  outbuf = output + converted;
  outleft = outlen - converted;
  
  converted = iconv (cd, (char **) &inbuf, &inleft, &outbuf, &outleft);
  if (converted != (size_t) -1 || errno == EINVAL) 
    {
      /*
       * EINVAL  An  incomplete  multibyte sequence has been encoun­-
       *         tered in the input.
       *
       * We'll just truncate it and ignore it.
       */
    }
  else if (errno != E2BIG) 
    {
      /*
       * EILSEQ An invalid multibyte sequence has been  encountered
       *        in the input.
       *
       * Bad input, we can't really recover from this. 
       */
      iconv_close (cd);
      free (output);
      return NULL;
    }
  else
    {
      /*
       * E2BIG   There is not sufficient room at *outbuf.
       */
      iconv_close (cd);
      free (output);
      return NULL;
      
    }
  
  /* flush the iconv conversion */
  iconv(cd, NULL, NULL, &outbuf, &outleft);
  iconv_close(cd);

  /* Note: not all charsets can be nul-terminated with a single
   * nul byte. UCS2, for example, needs 2 nul bytes and UCS4
   * needs 4. I hope that 4 nul bytes is enough to terminate all
   * multibyte charsets? */

  /* nul-terminate the string */
  memset(outbuf, 0, 4);

  return output;
}

#ifdef FEATURE_ICU

// detection.
const char* icu_detection_best_match(const char *input, const size_t &input_size,
				     int32_t *confidence)
{
  UErrorCode uec;
  UCharsetDetector *ucsd = ucsdet_open(&uec);
  
  ucsdet_setText(ucsd,input,input_size, &uec);
  const UCharsetMatch *ucsm = ucsdet_detect(ucsd,&uec);
  if (!ucsm)
    {
      ucsdet_close(ucsd);
      *confidence = 0;
      return NULL;
    }
  const char *cs = ucsdet_getName(ucsm,&uec);
  *confidence = ucsdet_getConfidence(ucsm,&uec);
  
  ucsdet_close(ucsd);
  return cs;
}

// conversion.
char* icu_conversion(const char *from_cset, const char *to_cset,
		     const char *input, int32_t *clen)
{
  int32_t targetSize = 2048;
  UErrorCode status = U_ZERO_ERROR;
  char *target = (char*)malloc(sizeof(char) * targetSize);
  *clen = ucnv_convert(from_cset,to_cset,target,targetSize,input,-1,&status);
  if (!U_SUCCESS(status))
    {
      free(target);   
      return NULL;
    }
  return target;
}

#endif
