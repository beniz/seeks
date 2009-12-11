/*********************************************************************
 * Purpose     :  Declares functions to parse/crunch headers and pages.
 *                Functions declared include:
 *                   `add_to_iob', `client_cookie_adder', `client_from',
 *                   `client_referrer', `client_send_cookie', `client_ua',
 *                   `client_uagent', `client_x_forwarded',
 *                   `client_x_forwarded_adder', `client_xtra_adder',
 *                   `content_type', `crumble', `destroy_list', `enlist',
 *                   `flush_socket', ``get_header', `sed', `filter_header'
 *                   `server_content_encoding', `server_content_disposition',
 *                   `server_last_modified', `client_accept_language',
 *                   `crunch_client_header', `client_if_modified_since',
 *                   `client_if_none_match', `get_destination_from_headers',
 *                   `parse_header_time', `decompress_iob' and `server_set_cookie'.
 *
 * Copyright   :  Modified by Emmanuel Benazera for the Seeks Project,
 *                2009.
 *              
 *                Written by and Copyright (C) 2001-2009 the
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
 *
 *********************************************************************/

#include "config.h"
#include "parsers.h"
#include "mem_utils.h"
#include "proxy_configuration.h"

#ifndef _WIN32
#include <stdio.h>
#include <sys/types.h>
#endif

#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

//#ifdef __GLIBC__
/*
 * Convince GNU's libc to provide a strptime prototype.
 */
//#define __USE_XOPEN
//#endif /*__GLIBC__ */
#include <time.h>

#include <iostream>

#ifdef FEATURE_ZLIB
#include <zlib.h>

#define GZIP_IDENTIFIER_1       0x1f
#define GZIP_IDENTIFIER_2       0x8b

#define GZIP_FLAG_CHECKSUM      0x02
#define GZIP_FLAG_EXTRA_FIELDS  0x04
#define GZIP_FLAG_FILE_NAME     0x08
#define GZIP_FLAG_COMMENT       0x10
#define GZIP_FLAG_RESERVED_BITS 0xe0
#endif

#if !defined(_WIN32) && !defined(__OS2__)
#include <unistd.h>
#endif

#include "proxy_dts.h"

#ifdef FEATURE_PTHREAD
#include "seeks_proxy.h" /* for mutex semapores only */
#endif /* def FEATURE_PTHREAD */

#include "errlog.h"
#include "spsockets.h"
#include "miscutil.h"
#include "filters.h"
#include "cgi.h"

#ifndef HAVE_STRPTIME
#include "strptime.h"
#endif

namespace sp
{

parsers_list parsers::_client_patterns[] = {
   parsers_list("referer:",8,&parsers::client_referrer),
   parsers_list("user-agent:",11,&parsers::client_uagent),
   parsers_list("ua-",3,&parsers::client_ua),
   parsers_list("from:",5,&parsers::client_from),
   parsers_list("cookie:",7,&parsers::client_send_cookie),
   parsers_list("x-forwarded-for:",16,&parsers::client_x_forwarded),
   parsers_list("Accept-Encoding:",16,&parsers::client_accept_encoding),
   parsers_list("TE:",3,&parsers::client_te),
   parsers_list("Host:",5,&parsers::client_host),
   parsers_list("if-modified-since:",18,&parsers::client_if_modified_since),
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   parsers_list("Keep-Alive:",11,&parsers::client_keep_alive),
#else
   parsers_list("Keep-Alive:",11,&parsers::crumble),
#endif
   parsers_list("connection:",11,&parsers::client_connection),
   parsers_list("proxy-connection:",17,&parsers::crumble),
   parsers_list("max-forwards:",13,&parsers::client_max_forwards),
   parsers_list("Accept-Language:",16,&parsers::client_accept_language),
   parsers_list("if-none-match:",14,&parsers::client_if_none_match),
   parsers_list("Range:",6,&parsers::client_range),
   parsers_list("Request-Range:",14,&parsers::client_range),
   parsers_list("If-Range:",9,&parsers::client_range),
   parsers_list("X-Filter:",9,&parsers::client_x_filter),
   parsers_list("*",0,&parsers::crunch_client_header),
//   parsers_list("*",0,&parsers::filter_header),
   parsers_list(NULL,0,NULL)
};

parsers_list parsers::_server_patterns[] = {
   parsers_list("HTTP/",5,&parsers::server_http),
   parsers_list("set-cookie:",11,&parsers::server_set_cookie),
   parsers_list("connection:",11,&parsers::server_connection),
   parsers_list("Content-Type:",13,&parsers::server_content_type),
   parsers_list("Content-MD5:",12,&parsers::server_content_md5),
   parsers_list("Content-Encoding:",17,&parsers::server_content_encoding),
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   parsers_list("Content-Length:",15,&parsers::server_save_content_length),
   parsers_list("Keep-Alive:",11,&parsers::server_keep_alive),
   parsers_list("Proxy-Connection:",17,&parsers::server_proxy_connection),
#else
   parsers_list("Keep-Alive:",11,&parsers::crumble),
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */
   parsers_list("Transfer-Encoding:",18,&parsers::server_transfer_coding),
   parsers_list("content-disposition:",20,&parsers::server_content_disposition),
   parsers_list("Last-Modified:",14,&parsers::server_last_modified),
   parsers_list("*",0,&parsers::crunch_server_header),
//   parsers_list("*",0,&parsers::filter_header),
   parsers_list(NULL,0, NULL)
};

const add_header_func_ptr parsers::_add_client_headers[] = {
     parsers::client_host_adder,
     parsers::client_x_forwarded_for_adder,
     parsers::client_xtra_adder,
     /* Temporarily disabled:    client_accept_encoding_adder, */
     parsers::client_connection_header_adder,
     NULL
};

const add_header_func_ptr parsers::_add_server_headers[] = {
     parsers::server_connection_adder,
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
     parsers::server_proxy_connection_adder,
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */
   NULL
};

/*********************************************************************
 *
 * Function    :  flush_socket
 *
 * Description :  Write any pending "buffered" content.
 *
 * Parameters  :
 *          1  :  fd = file descriptor of the socket to read
 *          2  :  iob = The I/O buffer to flush, usually csp->iob.
 *
 * Returns     :  On success, the number of bytes written are returned (zero
 *                indicates nothing was written).  On error, -1 is returned,
 *                and errno is set appropriately.  If count is zero and the
 *                file descriptor refers to a regular file, 0 will be
 *                returned without causing any other effect.  For a special
 *                file, the results are not portable.
 *
 *********************************************************************/
long parsers::flush_socket(sp_socket fd, iob *iob)
{
   long len = iob->_eod - iob->_cur;

   if (len <= 0)
   {
      return(0);
   }

   if (spsockets::write_socket(fd, iob->_cur, (size_t)len))
   {
      return(-1);
   }
   iob->_eod = iob->_cur = iob->_buf;
   return(len);
}


/*********************************************************************
 *
 * Function    :  add_to_iob
 *
 * Description :  Add content to the buffered page, expanding the
 *                buffer if necessary.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  buf = holds the content to be added to the page
 *          3  :  n = number of bytes to be added
 *
 * Returns     :  SP_ERR_OK on success, SP_ERR_MEMORY if out-of-memory
 *                or buffer limit reached.
 *
 *********************************************************************/
sp_err parsers::add_to_iob(client_state *csp, char *buf, long n)
{
   iob *iob = &csp->_iob;
   size_t used, offset, need, want;
   char *p;

   if (n <= 0) return SP_ERR_OK;

   used   = (size_t)(iob->_eod - iob->_buf);
   offset = (size_t)(iob->_cur - iob->_buf);
   need   = used + (size_t)n + 1;

   /*
    * If the buffer can't hold the new data, extend it first.
    * Use the next power of two if possible, else use the actual need.
    */
   if (need > csp->_config->_buffer_limit)
   {
      errlog::log_error(LOG_LEVEL_INFO,
			"Buffer limit reached while extending the buffer (iob). Needed: %d. Limit: %d",
			need, csp->_config->_buffer_limit);
      return SP_ERR_MEMORY;
   }

   if (need > iob->_size)
   {
      for (want = csp->_iob._size ? csp->_iob._size : 512; want <= need;) want *= 2;
      
      if (want <= csp->_config->_buffer_limit 
	  && NULL != (p = (char *)realloc(iob->_buf, want)))  // beware.
      {
         iob->_size = want;
      }
      else if (NULL != (p = (char *)realloc(iob->_buf, need))) // beware.
      {
         iob->_size = need;
      }
      else
      {
         errlog::log_error(LOG_LEVEL_ERROR, "Extending the buffer (iob) failed: %E");
         return SP_ERR_MEMORY;
      }

      /* Update the iob pointers */
      iob->_cur = p + offset;
      iob->_eod = p + used;
      iob->_buf = p;
   }

   /* copy the new data into the iob buffer */
   memcpy(iob->_eod, buf, (size_t)n);

   /* point to the end of the data */
   iob->_eod += n;

   /* null terminate == cheap insurance */
   *iob->_eod = '\0';

   return SP_ERR_OK;
}


#ifdef FEATURE_ZLIB
/*********************************************************************
 *
 * Function    :  decompress_iob
 *
 * Description :  Decompress buffered page, expanding the
 *                buffer as necessary.  csp->_iob->_cur
 *                should point to the the beginning of the
 *                compressed data block.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  SP_ERR_OK on success,
 *                SP_ERR_MEMORY if out-of-memory limit reached, and
 *                SP_ERR_COMPRESS if error decompressing buffer.
 *
 *********************************************************************/
sp_err parsers::decompress_iob(client_state *csp)
{
   char  *buf;       /* new, uncompressed buffer */
   char  *cur;       /* Current iob position (to keep the original 
                      * iob->_cur unmodified if we return early) */
   size_t bufsize;   /* allocated size of the new buffer */
   size_t old_size;  /* Content size before decompression */
   size_t skip_size; /* Number of bytes at the beginning of the iob
                        that we should NOT decompress. */
   int status;       /* return status of the inflate() call */
   z_stream zstr;    /* used by calls to zlib */

   assert(csp->_iob._cur - csp->_iob._buf > 0);
   assert(csp->_iob._eod - csp->_iob._cur > 0);

   bufsize = csp->_iob._size;
   skip_size = (size_t)(csp->_iob._cur - csp->_iob._buf);
   old_size = (size_t)(csp->_iob._eod - csp->_iob._cur);

   cur = csp->_iob._cur;

   if (bufsize < (size_t)10)
   {
      /*
       * This is to protect the parsing of gzipped data,
       * but it should(?) be valid for deflated data also.
       */
      errlog::log_error(LOG_LEVEL_ERROR, "Buffer too small decompressing iob");
      return SP_ERR_COMPRESS;
   }

   if (csp->_content_type & CT_GZIP)
   {
      /*
       * Our task is slightly complicated by the facts that data
       * compressed by gzip does not include a zlib header, and
       * that there is no easily accessible interface in zlib to
       * handle a gzip header. We strip off the gzip header by
       * hand, and later inform zlib not to expect a header.
       */

      /*
       * Strip off the gzip header. Please see RFC 1952 for more
       * explanation of the appropriate fields.
       */
      if (((*cur++ & 0xff) != GZIP_IDENTIFIER_1)
       || ((*cur++ & 0xff) != GZIP_IDENTIFIER_2)
       || (*cur++ != Z_DEFLATED))
      {
         errlog::log_error(LOG_LEVEL_ERROR, "Invalid gzip header when decompressing");
         return SP_ERR_COMPRESS;
      }
      else
      {
         int flags = *cur++;
         if (flags & GZIP_FLAG_RESERVED_BITS)
         {
            /* The gzip header has reserved bits set; bail out. */
            errlog::log_error(LOG_LEVEL_ERROR, "Invalid gzip header flags when decompressing");
            return SP_ERR_COMPRESS;
         }

         /*
          * Skip mtime (4 bytes), extra flags (1 byte)
          * and OS type (1 byte).
          */
         cur += 6;

         /* Skip extra fields if necessary. */
         if (flags & GZIP_FLAG_EXTRA_FIELDS)
         {
            /*
             * Skip a given number of bytes, specified
             * as a 16-bit little-endian value.
             *
             * XXX: this code is untested and should probably be removed.
             */
            int skip_bytes;
            skip_bytes = *cur++;
            skip_bytes += *cur++ << 8;

            /*
             * The number of bytes to skip should be positive
             * and we'd like to stay in the buffer.
             */
            if ((skip_bytes < 0) || (skip_bytes >= (csp->_iob._eod - cur)))
            {
               errlog::log_error(LOG_LEVEL_ERROR,
				 "Unreasonable amount of bytes to skip (%d). Stopping decompression",
				 skip_bytes);
               return SP_ERR_COMPRESS;
            }
            errlog::log_error(LOG_LEVEL_INFO,
			      "Skipping %d bytes for gzip compression. Does this sound right?",
			      skip_bytes);
            cur += skip_bytes;
         }

         /* Skip the filename if necessary. */
         if (flags & GZIP_FLAG_FILE_NAME)
         {
            /* A null-terminated string is supposed to follow. */
            while (*cur++ && (cur < csp->_iob._eod));
         }

         /* Skip the comment if necessary. */
         if (flags & GZIP_FLAG_COMMENT)
         {
            /* A null-terminated string is supposed to follow. */
            while (*cur++ && (cur < csp->_iob._eod));
         }

         /* Skip the CRC if necessary. */
         if (flags & GZIP_FLAG_CHECKSUM)
         {
            cur += 2;
         }

         if (cur >= csp->_iob._eod)
         {
            /*
             * If the current position pointer reached or passed
             * the buffer end, we were obviously tricked to skip
             * too much.
             */
            errlog::log_error(LOG_LEVEL_ERROR,
               "Malformed gzip header detected. Aborting decompression.");
            return SP_ERR_COMPRESS;
         }
      }
   }
   else if (csp->_content_type & CT_DEFLATE)
   {
      /*
       * XXX: The debug level should be lowered
       * before the next stable release.
       */
      errlog::log_error(LOG_LEVEL_INFO, "Decompressing deflated iob: %d", *cur);
      /*
       * In theory (that is, according to RFC 1950), deflate-compressed
       * data should begin with a two-byte zlib header and have an
       * adler32 checksum at the end. It seems that in practice only
       * the raw compressed data is sent. Note that this means that
       * we are not RFC 1950-compliant here, but the advantage is that
       * this actually works. :)
       *
       * We add a dummy null byte to tell zlib where the data ends,
       * and later inform it not to expect a header.
       *
       * Fortunately, add_to_iob() has thoughtfully null-terminated
       * the buffer; we can just increment the end pointer to include
       * the dummy byte.  
       */
      csp->_iob._eod++;
   }
   else
   {
      errlog::log_error(LOG_LEVEL_ERROR,
         "Unable to determine compression format for decompression");
      return SP_ERR_COMPRESS;
   }

   /* Set up the fields required by zlib. */
   zstr.next_in  = (Bytef *)cur;
   zstr.avail_in = (unsigned int)(csp->_iob._eod - cur);
   zstr.zalloc   = Z_NULL;
   zstr.zfree    = Z_NULL;
   zstr.opaque   = Z_NULL;

   /*
    * Passing -MAX_WBITS to inflateInit2 tells the library
    * that there is no zlib header.
    */
   if (inflateInit2(&zstr, -MAX_WBITS) != Z_OK)
   {
      errlog::log_error(LOG_LEVEL_ERROR, "Error initializing decompression");
      return SP_ERR_COMPRESS;
   }

   /*
    * Next, we allocate new storage for the inflated data.
    * We don't modify the existing iob yet, so in case there
    * is error in decompression we can recover gracefully.
    */
   buf = (char*) zalloc(bufsize);
   if (NULL == buf)
   {
      errlog::log_error(LOG_LEVEL_ERROR, "Out of memory decompressing iob");
      return SP_ERR_MEMORY;
   }

   assert(bufsize >= skip_size);
   memcpy(buf, csp->_iob._buf, skip_size);
   zstr.avail_out = bufsize - skip_size;
   zstr.next_out  = (Bytef *)buf + skip_size;

   /* Try to decompress the whole stream in one shot. */
   while (Z_BUF_ERROR == (status = inflate(&zstr, Z_FINISH)))
   {
      /* We need to allocate more memory for the output buffer. */

      char *tmpbuf;                /* used for realloc'ing the buffer */
      size_t oldbufsize = bufsize; /* keep track of the old bufsize */

      if (0 == zstr.avail_in)
      {
         /*
          * If zlib wants more data then there's a problem, because
          * the complete compressed file should have been buffered.
          */
         errlog::log_error(LOG_LEVEL_ERROR,
			   "Unexpected end of compressed iob. Using what we got so far.");
         break;
      }

      /*
       * If we tried the limit and still didn't have enough
       * memory, just give up.
       */
      if (bufsize == csp->_config->_buffer_limit)
      {
         errlog::log_error(LOG_LEVEL_ERROR, "Buffer limit reached while decompressing iob");
         return SP_ERR_MEMORY;
      }

      /* Try doubling the buffer size each time. */
      bufsize *= 2;

      /* Don't exceed the buffer limit. */
      if (bufsize > csp->_config->_buffer_limit)
      {
         bufsize = csp->_config->_buffer_limit;
      }
    
      /* Try to allocate the new buffer. */
      tmpbuf = (char*)realloc(buf, bufsize);
      if (NULL == tmpbuf)
      {
         errlog::log_error(LOG_LEVEL_ERROR, "Out of memory decompressing iob");
	 freez(buf);
	 buf = NULL;
         return SP_ERR_MEMORY;
      }
      else
      {
         char *oldnext_out = (char *)zstr.next_out;

         /*
          * Update the fields for inflate() to use the new
          * buffer, which may be in a location different from
          * the old one.
          */
         zstr.avail_out += bufsize - oldbufsize;
         zstr.next_out   = (Bytef *)tmpbuf + bufsize - zstr.avail_out;

         /*
          * Compare with an uglier method of calculating these values
          * that doesn't require the extra oldbufsize variable.
          */
         assert(zstr.avail_out == tmpbuf + bufsize - (char *)zstr.next_out);
         assert((char *)zstr.next_out == tmpbuf + ((char *)oldnext_out - buf));
         assert(zstr.avail_out > 0U);

         buf = tmpbuf;
      }
   }

   if (Z_STREAM_ERROR == inflateEnd(&zstr))
   {
      errlog::log_error(LOG_LEVEL_ERROR,
			"Inconsistent stream state after decompression: %s", zstr.msg);
      /*
       * XXX: Intentionally no return.
       *
       * According to zlib.h, Z_STREAM_ERROR is returned
       * "if the stream state was inconsistent".
       *
       * I assume in this case inflate()'s status
       * would also be something different than Z_STREAM_END
       * so this check should be redundant, but lets see.
       */
   }

   if ((status != Z_STREAM_END) && (0 != zstr.avail_in))
   {
      /*
       * We failed to decompress the stream and it's
       * not simply because of missing data.
       */
      errlog::log_error(LOG_LEVEL_ERROR,
         "Unexpected error while decompressing to the buffer (iob): %s",
         zstr.msg);
      return SP_ERR_COMPRESS;
   }

   /*
    * Finally, we can actually update the iob, since the
    * decompression was successful. First, free the old
    * buffer.
    */
   freez(csp->_iob._buf);
   csp->_iob._buf = NULL;

   /* Now, update the iob to use the new buffer. */
   csp->_iob._buf  = buf;
   csp->_iob._cur  = csp->_iob._buf + skip_size;
   csp->_iob._eod  = (char *)zstr.next_out;
   csp->_iob._size = bufsize;
  
   /*
    * Make sure the new uncompressed iob obeys some minimal
    * consistency conditions.
    */
   if ((csp->_iob._buf <  csp->_iob._cur)
    && (csp->_iob._cur <= csp->_iob._eod)
    && (csp->_iob._eod <= csp->_iob._buf + csp->_iob._size))
   {
      const size_t new_size = (size_t)(csp->_iob._eod - csp->_iob._cur);
      if (new_size > (size_t)0)
      {
         errlog::log_error(LOG_LEVEL_RE_FILTER,
            "Decompression successful. Old size: %d, new size: %d.",
            old_size, new_size);
      }
      else
      {
         /* zlib thinks this is OK, so lets do the same. */
         errlog::log_error(LOG_LEVEL_INFO, "Decompression didn't result in any content.");
      }
   }
   else
   {
      /* It seems that zlib did something weird. */
      errlog::log_error(LOG_LEVEL_ERROR,
         "Unexpected error decompressing the buffer (iob): %d==%d, %d>%d, %d<%d",
         csp->_iob._cur, csp->_iob._buf + skip_size, csp->_iob._eod, csp->_iob._buf,
         csp->_iob._eod, csp->_iob._buf + csp->_iob._size);
      return SP_ERR_COMPRESS;
   }

   return SP_ERR_OK;

}
#endif /* defined(FEATURE_ZLIB) */


/*********************************************************************
 *
 * Function    :  string_move
 *
 * Description :  memmove wrapper to move the last part of a string
 *                towards the beginning, overwriting the part in
 *                the middle. strlcpy() can't be used here as the
 *                strings overlap.
 *
 * Parameters  :
 *          1  :  dst = Destination to overwrite
 *          2  :  src = Source to move.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void parsers::string_move(char *dst, char *src)
{
   assert(dst < src);

   /* +1 to copy the terminating nul as well. */
   memmove(dst, src, strlen(src)+1);
}


/*********************************************************************
 *
 * Function    :  normalize_lws
 *
 * Description :  Reduces unquoted linear white space in headers
 *                to a single space in accordance with RFC 2616 2.2.
 *                This simplifies parsing and filtering later on.
 *
 *                XXX: Remove log messages before
 *                     the next stable release?
 *
 * Parameters  :
 *          1  :  header = A header with linear white space to reduce.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void parsers::normalize_lws(char *header)
{
   char *p = header;

   while (*p != '\0')
   {
      if (isspace(*p) && isspace(*(p+1)))
      {
         char *q = p+1;

         while (isspace(*q))
         {
            q++;
         }
         errlog::log_error(LOG_LEVEL_HEADER, "Reducing white space in '%s'", header);
         parsers::string_move(p+1, q);
      }

      if (*p == '\t')
      {
         errlog::log_error(LOG_LEVEL_HEADER,
			   "Converting tab to space in '%s'", header);
         *p = ' ';
      }
      else if (*p == '"')
      {
         char *end_of_token = strstr(p+1, "\"");

         if (NULL != end_of_token)
         {
            /* Don't mess with quoted text. */
            p = end_of_token;
         }
         else
         {
            errlog::log_error(LOG_LEVEL_HEADER,
               "Ignoring single quote in '%s'", header);
         }
      }
      p++;
   }

   p = strchr(header, ':');
   if ((p != NULL) && (p != header) && isspace(*(p-1)))
   {
      /*
       * There's still space before the colon.
       * We don't want it.
       */
      parsers::string_move(p-1, p);
   }
}


/*********************************************************************
 *
 * Function    :  get_header
 *
 * Description :  This (odd) routine will parse the csp->_iob
 *                to get the next complete header.
 *
 * Parameters  :
 *          1  :  iob = The I/O buffer to parse, usually csp->_iob.
 *
 * Returns     :  Any one of the following:
 *
 * 1) a pointer to a dynamically allocated string that contains a header line
 * 2) NULL  indicating that the end of the header was reached
 * 3) ""    indicating that the end of the iob was reached before finding
 *          a complete header line.
 *
 *********************************************************************/
char* parsers::get_header(iob *iob)
{
   char *header;

   header = parsers::get_header_line(iob);

   if ((header == NULL) || (*header == '\0'))
   {
      /*
       * No complete header read yet, tell the client.
       */
      return header;
   }

   while ((iob->_cur[0] == ' ') || (iob->_cur[0] == '\t'))
   {
      /*
       * Header spans multiple lines, append the next one.
       */
      char *continued_header;
      
      continued_header = parsers::get_header_line(iob);
      if ((continued_header == NULL) || (*continued_header == '\0'))
       {
         /*
          * No complete header read yet, return what we got.
          * XXX: Should "unread" header instead.
          */
         errlog::log_error(LOG_LEVEL_INFO,
            "Failed to read a multi-line header properly: '%s'",
            header);
         break;
      }

      if (SP_ERR_OK != miscutil::string_join(&header, continued_header))
      {
         errlog::log_error(LOG_LEVEL_FATAL,
            "Out of memory while appending multiple headers.");
      }
      else
      {
         /* XXX: remove before next stable release. */
         errlog::log_error(LOG_LEVEL_HEADER,
            "Merged multiple header lines to: '%s'",
            header);
      }
   }

   parsers::normalize_lws(header);

   return header;
}


/*********************************************************************
 *
 * Function    :  get_header_line
 *
 * Description :  This (odd) routine will parse the csp->_iob
 *                to get the next header line.
 *
 * Parameters  :
 *          1  :  iob = The I/O buffer to parse, usually csp->_iob.
 *
 * Returns     :  Any one of the following:
 *
 * 1) a pointer to a dynamically allocated string that contains a header line
 * 2) NULL  indicating that the end of the header was reached
 * 3) ""    indicating that the end of the iob was reached before finding
 *          a complete header line.
 *
 *********************************************************************/
char* parsers::get_header_line(iob *iob)
{
   char *p, *q, *ret;

   if ((iob->_cur == NULL)
      || ((p = strchr(iob->_cur, '\n')) == NULL))
   {
      return (char*)""; /* couldn't find a complete header */
   }

   *p = '\0';

   ret = strdup(iob->_cur);
   if (ret == NULL)
   {
      /* FIXME No way to handle error properly */
      errlog::log_error(LOG_LEVEL_FATAL, "Out of memory in get_header_line()");
   }
   assert(ret != NULL);

   iob->_cur = p+1;

   if ((q = strchr(ret, '\r')) != NULL) *q = '\0';

   /* is this a blank line (i.e. the end of the header) ? */
   if (*ret == '\0')
   {
      freez(ret);
      ret = NULL;
      return NULL;
   }

   return ret;
}


/*********************************************************************
 *
 * Function    :  get_header_value
 *
 * Description :  Get the value of a given header from a chained list
 *                of header lines or return NULL if no such header is
 *                present in the list.
 *
 * Parameters  :
 *          1  :  header_list = pointer to list
 *          2  :  header_name = string with name of header to look for.
 *                              Trailing colon required, capitalization
 *                              doesn't matter.
 *
 * Returns     :  NULL if not found, else value of header
 *
 *********************************************************************/
char* parsers::get_header_value(const std::list<const char*> *header_list, const char *header_name)
{
   char *ret = NULL;
   size_t length = 0;

   assert(header_list);
   assert(header_name);
   length = strlen(header_name);

   std::list<const char*>::const_iterator lit = header_list->begin();
   while(lit!=header_list->end())
     {
	const char *str = (*lit);
	++lit;
	
	if (str)
	  {
	     if (!miscutil::strncmpic(str, header_name, length))
	       {
		  /*
		   * Found: return pointer to start of value
		   */
		  ret = (char*)str + length;
		  while (*ret && isspace(*ret)) ret++;
		  return ret;
	       }
	  }
     }
   
   /* 
    * Not found
    */
   return NULL;
}

//seeks: deprecated, should be turned into a plugin.
/*********************************************************************
 *
 * Function    :  scan_headers
 *
 * Description :  Scans headers, applies tags and updates action bits. 
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  SP_ERR_OK
 *
 *********************************************************************/
/* sp_err parsers::scan_headers(client_state *csp)
{
   sp_err err = SP_ERR_OK;

   std::list<const char*>::const_iterator lit = csp->_headers.begin();
   while(lit!=csp->_headers.end())
     {
	const char *str = (*lit); */
	
	/* Header crunch()ed in previous run? -> ignore */
	/* if (str == NULL) 
	  {
	     ++lit;
	     continue;
	  }
	
	errlog::log_error(LOG_LEVEL_HEADER, "scan: %s", str);
	err = parsers::header_tagger(csp, (char*)str); // beware.
	++lit;
     }
   
   return err;
} */

//seeks: deprecated, should be turned into a plugin.
/*********************************************************************
 *
 * Function    :  sed
 *
 * Description :  add, delete or modify lines in the HTTP header streams.
 *                On entry, it receives a linked list of headers space
 *                that was allocated dynamically (both the list nodes
 *                and the header contents).
 *
 *                As a side effect it frees the space used by the original
 *                header lines.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  filter_server_headers = Boolean to switch between
 *                                        server and header filtering.
 *
 * Returns     :  SP_ERR_OK in case off success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::sed(client_state *csp, int filter_server_headers)
{
   /* XXX: use more descriptive names. */
   const parsers_list *v;
   const add_header_func_ptr *f;
   sp_err err = SP_ERR_OK;

   if (filter_server_headers)
   {
      v = parsers::_server_patterns;
      f = parsers::_add_server_headers;
   }
   else
   {
      v = parsers::_client_patterns;
      f = parsers::_add_client_headers;
   }

   //parsers::scan_headers(csp);

   while ((err == SP_ERR_OK) && (v->_str != NULL))
   {
      std::list<const char*>::iterator lit = csp->_headers.begin();
      while(lit!=csp->_headers.end() && err == SP_ERR_OK)
	{
	   const char *str = (*lit);
	   
	   /* Header crunch()ed in previous run? -> ignore */
            if (str == NULL) 
	     {
		++lit;
		continue;
	     }
	   
	   /* Does the current parser handle this header? */
	   if ((miscutil::strncmpic(str, v->_str, v->_len) == 0) ||
	       (v->_len == CHECK_EVERY_HEADER_REMAINING))
	     {
		char *cstr = strdup(str);
		err = v->_parser(csp, &cstr);
		free_const((*lit));
		
		if (cstr)
		  {
		     (*lit) = cstr;
		  }
		else 
		  {
		     std::list<const char*>::iterator clit = lit;
		     ++lit;
		     csp->_headers.erase(clit);
		     continue;
		  }
	     }
	   ++lit;
      }
      v++;
   }

   /* place additional headers on the csp->_headers list */
  while ((err == SP_ERR_OK) && (*f))
   { 
      err = (*f)(csp);
      f++;
   }

   return err;
}


/*********************************************************************
 *
 * Function    :  update_server_headers
 *
 * Description :  Updates server headers after the body has been modified.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  SP_ERR_OK in case off success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::update_server_headers(client_state *csp)
{
   sp_err err = SP_ERR_OK;

   static const parsers_list server_patterns_light[] = {
      parsers_list("Content-Length:",15,&parsers::server_adjust_content_length),
      parsers_list("Transfer-Encoding:",18,&parsers::server_transfer_coding),
#ifdef FEATURE_ZLIB
      parsers_list("Content-Encoding:",17,&parsers::server_content_encoding),
#endif /* def FEATURE_ZLIB */
      parsers_list(NULL,0, NULL)
   };

   if (miscutil::strncmpic(csp->_http._cmd, "HEAD", 4))
   {
      const parsers_list *v;
      
      for (v = server_patterns_light; (err == SP_ERR_OK) && (v->_str != NULL); v++)
      {
	 std::list<const char*>::iterator lit = csp->_headers.begin();
	 while(lit!=csp->_headers.end())
	   {
	      const char *str = (*lit);
	      
	      /* Header crunch()ed in previous run? -> ignore */
	      if (str == NULL) 
		{
		   ++lit;
		   continue;
		}
	      
	      /* Does the current parser handle this header? */
	      if (miscutil::strncmpic(str, v->_str, v->_len) == 0)
		{
		   char *cstr = strdup(str);
		   err = v->_parser(csp,(char**)&cstr);
		   free_const(str);
		   
		   if (cstr && (strcmp(cstr,"")!=0))
		     {
			(*lit) = cstr;
		     }
		   else
		     {
			if (strcmp(cstr,"")==0)
			  freez(cstr);
			std::list<const char*>::iterator clit = lit;
			++lit;
			csp->_headers.erase(clit);
			continue;
		     }
		}
	      ++lit;
	   }
      }
   }
   
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   if ((SP_ERR_OK == err)
       && (csp->_flags & CSP_FLAG_MODIFIED)
       && (csp->_flags & CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE)
       && !(csp->_flags & CSP_FLAG_SERVER_CONTENT_LENGTH_SET))
     {
	char header[50];
	
	parsers::create_content_length_header(csp->_content_length, header, sizeof(header));
	err = miscutil::enlist(&csp->_headers, header);
	if (SP_ERR_OK == err)
	  {
	     errlog::log_error(LOG_LEVEL_HEADER,
			       "Content modified with no Content-Length header set. "
			       "Created: %s.", header);
	  }
     }
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */
   
   return err;
}
   
//seeks: deprecated, should be turned into a plugin.
/*********************************************************************
 *
 * Function    :  header_tagger
 *
 * Description :  Executes all text substitutions from applying
 *                tag actions and saves the result as tag.
 *
 *                XXX: Shares enough code with filter_header() and
 *                pcrs_filter_response() to warrant some helper functions.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = Header that is used as tagger input
 *
 * Returns     :  SP_ERR_OK on success and always succeeds
 *
 *********************************************************************/
/* sp_err parsers::header_tagger(client_state *csp, char *header)
{
   int wanted_filter_type;
   int multi_action_index;
   int i;
   pcrs_job *job;

   file_list *fl;
   re_filterfile_spec *b;
   
   int found_filters = 0;
   const size_t header_length = strlen(header);
   
   if (csp->_flags & CSP_FLAG_CLIENT_HEADER_PARSING_DONE)
   {
      wanted_filter_type = FT_SERVER_HEADER_TAGGER;
      multi_action_index = ACTION_MULTI_SERVER_HEADER_TAGGER;
   }
   else
   {
      wanted_filter_type = FT_CLIENT_HEADER_TAGGER;
      multi_action_index = ACTION_MULTI_CLIENT_HEADER_TAGGER;
   } */

   /* Check if there are any filters */
   /* for (i = 0; i < MAX_AF_FILES; i++)
   {
      fl = csp->_rlist[i];
      if (NULL != fl)
      {
         if (NULL != fl->_f)
         {
           found_filters = 1;
           break;
         }
      }
   }

   if (0 == found_filters)
   {
      errlog::log_error(LOG_LEVEL_ERROR, "Inconsistent configuration: "
         "tagging enabled, but no taggers available.");
      return SP_ERR_OK;
   }

   for (i = 0; i < MAX_AF_FILES; i++)
   {
      fl = csp->_rlist[i];
      if ((NULL == fl) || (NULL == fl->_f))
      { */
         /*
          * Either there are no filter files
          * left, or this filter file just
          * contains no valid filters.
          *
          * Continue to be sure we don't miss
          * valid filter files that are chained
          * after empty or invalid ones.
          */
         /* continue;
      } */

      /* For all filters, */
      /* for (b = (re_filterfile_spec*)fl->_f; b; b = b->_next)
      {
         if (b->_type != wanted_filter_type)
         { */
            /* skip the ones we don't care about, */
       /*     continue;
         } */
         /* leaving only taggers that could apply, of which we use the ones, */
         /* std::list<const char*>::const_iterator lit = csp->_action._multi[multi_action_index].begin();
	 while(lit!=csp->_action._multi[multi_action_index].end())
	   {
	      const char *tag_name = (*lit);
	      ++lit; */

	      /* that do apply, and */
	      /* if (strcmp(b->_name, tag_name) == 0)
		{
		   char *modified_tag = NULL;
		   char *tag = header;
		   size_t size = header_length;
		   pcrs_job *joblist = b->_joblist;
		   
		   if (b->_dynamic) joblist = filters::compile_dynamic_pcrs_job_list(csp, b);
		   
		   if (NULL == joblist)
		     {
			errlog::log_error(LOG_LEVEL_RE_FILTER,
					  "Tagger %s has empty joblist. Nothing to do.", b->_name);
			continue;
		     } */
		   
		   /* execute their pcrs_joblist on the header. */
		   /* for (job = joblist; NULL != job; job = job->_next)
		     {
			const int hits = pcrs::pcrs_execute(job, tag, size, &modified_tag, &size);

			if (0 < hits)
			  { */
			     /* Success, continue with the modified version. */
			     /* if (tag != header)
			       {
				  freez(tag);
				  tag = NULL;
			       }
			     tag = modified_tag;
			  }
			else
			  { */
			     /* Tagger doesn't match */
			     /* if (0 > hits)
			       { */
				  /* Regex failure, log it but continue anyway. */
				  /* assert(NULL != header);
				  errlog::log_error(LOG_LEVEL_ERROR,
						    "Problems with tagger \'%s\' and header \'%s\': %s",
						    b->_name, *header, pcrs::pcrs_strerror(hits));
			       }
			     freez(modified_tag);
			     modified_tag = NULL;
			  }
		     }
		   
		   if (b->_dynamic) pcrs_job::pcrs_free_joblist(joblist); */

		   /* If this tagger matched */
/*		   if (tag != header)
		     {
			if (0 == size)
			  { */
			     /*
			      * There is to technical limitation which makes
			      * it impossible to use empty tags, but I assume
			      * no one would do it intentionally.
			      */
			    /* freez(tag); 
			     tag = NULL;
			     errlog::log_error(LOG_LEVEL_INFO,
					       "Tagger \'%s\' created an empty tag. Ignored.",
					       b->_name);
			     continue;
			  }
 
			if (!miscutil::list_contains_item(&csp->_tags, tag))
			  {
			     if (SP_ERR_OK != miscutil::enlist(&csp->_tags, tag))
			       {
				  errlog::log_error(LOG_LEVEL_ERROR,
						    "Insufficient memory to add tag \'%s\', "
						    "based on tagger \'%s\' and header \'%s\'",
						    tag, b->_name, *header);
			       }
			     else
			       {
				  const char *action_message; */
				  /*
				   * update the action bits right away, to make
				   * tagging based on tags set by earlier taggers
				   * of the same kind possible.
				   */
/*				  if (actions::update_action_bits_for_tag(csp, tag))
				    {
				       action_message = "Action bits updated accordingly.";
				    }
				  else
				    {
				       action_message = "No action bits update necessary.";
				    }
				  
				  errlog::log_error(LOG_LEVEL_HEADER,
						    "Tagger \'%s\' added tag \'%s\'. %s",
						    b->_name, tag, action_message);
			       }
			  }
			else
			  { */
			     /* XXX: Is this log-worthy? */
			    /* errlog::log_error(LOG_LEVEL_HEADER,
					       "Tagger \'%s\' didn't add tag \'%s\'. "
					       "Tag already present", b->_name, tag);
			  }
			freez(tag);
			tag = NULL;
		     } */ /* if the tagger matched */
		//} /* if the tagger applies */
         //} /* for every tagger that could apply */
      //} /* for all filters */
   //} /* for all filter files */

/*   return SP_ERR_OK;
} */

/* here begins the family of parser functions that reformat header lines */

//seeks: deprecated, should go into a dedicated plugin.
/*********************************************************************
 *
 * Function    :  filter_header
 *
 * Description :  Executes all text substitutions from all applying
 *                +(server|client)-header-filter actions on the header.
 *                Most of the code was copied from pcrs_filter_response,
 *                including the rather short variable names
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success and always succeeds
 *
 *********************************************************************/
/* sp_err parsers::filter_header(client_state *csp, char **header)
{
   int hits=0;
   int matches;
   size_t size = strlen(*header);

   char *newheader = NULL;
   pcrs_job *job;

   file_list *fl;
   re_filterfile_spec *b;
   
   int i, found_filters = 0;
   int wanted_filter_type;
   int multi_action_index;

   if (csp->_flags & CSP_FLAG_NO_FILTERING)
   {
      return SP_ERR_OK;
   }

   if (csp->_flags & CSP_FLAG_CLIENT_HEADER_PARSING_DONE)
   {
      wanted_filter_type = FT_SERVER_HEADER_FILTER;
      multi_action_index = ACTION_MULTI_SERVER_HEADER_FILTER;
   }
   else
   {
      wanted_filter_type = FT_CLIENT_HEADER_FILTER;
      multi_action_index = ACTION_MULTI_CLIENT_HEADER_FILTER;
   } */

   /*
    * Need to check the set of re_filterfiles...
    */
/*   for (i = 0; i < MAX_AF_FILES; i++)
   {
      fl = csp->_rlist[i];
      if (NULL != fl)
      {
         if (NULL != fl->_f)
         {
           found_filters = 1;
           break;
         }
      }
   }

   if (0 == found_filters)
   {
      errlog::log_error(LOG_LEVEL_ERROR, "Inconsistent configuration: "
         "header filtering enabled, but no matching filters available.");
      return SP_ERR_OK;
   }

   for (i = 0; i < MAX_AF_FILES; i++)
   {
      fl = csp->_rlist[i];
      if ((NULL == fl) || (NULL == fl->_f))
      { */
         /*
          * Either there are no filter files
          * left, or this filter file just
          * contains no valid filters.
          *
          * Continue to be sure we don't miss
          * valid filter files that are chained
          * after empty or invalid ones.
          */
/*         continue;
      } */
      /*
       * For all applying +filter actions, look if a filter by that
       * name exists and if yes, execute its pcrs_joblist on the
       * buffer.
       */
/*      for (b = (re_filterfile_spec*)fl->_f; b; b = b->_next)
      {
         if (b->_type != wanted_filter_type)
         { */
            /* Skip other filter types */
   /*         continue;
         }

	 std::list<const char*>::const_iterator lit = csp->_action._multi[multi_action_index].begin();
	 while(lit!=csp->_action._multi[multi_action_index].end())
	   {
	      const char *str = (*lit);
	      ++lit;

	      if (strcmp(b->_name, str) == 0)
		{
		   int current_hits = 0;
		   pcrs_job *joblist = b->_joblist;
		   
		   if (b->_dynamic) joblist = filters::compile_dynamic_pcrs_job_list(csp, b);
		   
		   if (NULL == joblist)
		     {
			errlog::log_error(LOG_LEVEL_RE_FILTER, "Filter %s has empty joblist. Nothing to do.", b->_name);
			continue;
		     }
		   
		   errlog::log_error(LOG_LEVEL_RE_FILTER, "filtering \'%s\' (size %d) with \'%s\' ...",
				     *header, size, b->_name); */
		   
		   /* Apply all jobs from the joblist */
/*		   for (job = joblist; NULL != job; job = job->_next)
		     {
			matches = pcrs::pcrs_execute(job, *header, size, &newheader, &size);
			if ( 0 < matches )
			  {
			     current_hits += matches; 
			     errlog::log_error(LOG_LEVEL_HEADER, "Transforming \"%s\" to \"%s\"", *header, newheader);
			     freez(*header);
			     *header = newheader;
			  }
			else if ( 0 == matches )
			  { */
			     /* Filter doesn't change header */
			/*     freez(newheader);
			     newheader = NULL;
			  }
			else
			  { */
			     /* RegEx failure */
			/*     errlog::log_error(LOG_LEVEL_ERROR, "Filtering \'%s\' with \'%s\' didn't work out: %s",
					       *header, b->_name, pcrs::pcrs_strerror(matches));
			     if (newheader != NULL)
			       {
				  errlog::log_error(LOG_LEVEL_ERROR, "Freeing what's left: %s", newheader);
				  freez(newheader);
				  newheader = NULL;
			       }
			  }
		     }
		   
		   if (b->_dynamic) pcrs_job::pcrs_free_joblist(joblist);
		   
		   errlog::log_error(LOG_LEVEL_RE_FILTER, "... produced %d hits (new size %d).", current_hits, size);
		   hits += current_hits;
		}
	   }
      }
   } */
   
   /*
    * Additionally checking for hits is important because if
    * the continue hack is triggered, server headers can
    * arrive empty to separate multiple heads from each other.
    */
/*   if ((0 == size) && hits)
   {
      errlog::log_error(LOG_LEVEL_HEADER, "Removing empty header %s", *header);
      freez(*header);
      *header = NULL;
   }

   return SP_ERR_OK;
} */

/*********************************************************************
 *
 * Function    :  server_connection
 *
 * Description :  Makes sure a proper "Connection:" header is
 *                set and signals connection_header_adder to
 *                do nothing.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::server_connection(client_state *csp, char **header)
{
   if (!miscutil::strcmpic(*header, "Connection: keep-alive")
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
    && !(csp->_flags & CSP_FLAG_SERVER_SOCKET_TAINTED)
#endif
      )
   {
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
      if ((csp->_config->_feature_flags & RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE))
      {
         csp->_flags |= CSP_FLAG_SERVER_CONNECTION_KEEP_ALIVE;
      }

      if ((csp->_flags & CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE))
      {
         errlog::log_error(LOG_LEVEL_HEADER,
            "Keeping the server header '%s' around.", *header);
      }
      else
#endif /* FEATURE_CONNECTION_KEEP_ALIVE */
      {
         char *old_header = *header;

         *header = strdup("Connection: close");
         if (header == NULL)
         {
            return SP_ERR_MEMORY;
         }
         errlog::log_error(LOG_LEVEL_HEADER, "Replaced: \'%s\' with \'%s\'", old_header, *header);
         freez(old_header);
	 old_header = NULL;
      }
   }

   /* Signal server_connection_adder() to return early. */
   csp->_flags |= CSP_FLAG_SERVER_CONNECTION_HEADER_SET;

   return SP_ERR_OK;
}


#ifdef FEATURE_CONNECTION_KEEP_ALIVE
/*********************************************************************
 *
 * Function    :  server_keep_alive
 *
 * Description :  Stores the server's keep alive timeout.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK.
 *
 *********************************************************************/
sp_err parsers::server_keep_alive(client_state *csp, char **header)
{
   unsigned int keep_alive_timeout;
   const char *timeout_position = strstr(*header, "timeout=");

   if ((NULL == timeout_position)
    || (1 != sscanf(timeout_position, "timeout=%u", &keep_alive_timeout)))
   {
      errlog::log_error(LOG_LEVEL_ERROR, "Couldn't parse: %s", *header);
   }
   else
   {
      if (keep_alive_timeout < csp->_server_connection._keep_alive_timeout)
      {
         errlog::log_error(LOG_LEVEL_HEADER,
            "Reducing keep-alive timeout from %u to %u.",
            csp->_server_connection._keep_alive_timeout, keep_alive_timeout);
         csp->_server_connection._keep_alive_timeout = keep_alive_timeout;
      }
      else
      {
         /* XXX: Is this log worthy? */
         errlog::log_error(LOG_LEVEL_HEADER,
            "Server keep-alive timeout is %u. Sticking with %u.",
			   keep_alive_timeout, csp->_server_connection._keep_alive_timeout);
      }
      csp->_flags |= CSP_FLAG_SERVER_KEEP_ALIVE_TIMEOUT_SET;
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  server_proxy_connection
 *
 * Description :  Figures out whether or not we should add a
 *                Proxy-Connection header.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK.
 *
 *********************************************************************/
sp_err parsers::server_proxy_connection(client_state *csp, char **header)
{
   csp->_flags |= CSP_FLAG_SERVER_PROXY_CONNECTION_HEADER_SET;
   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  client_keep_alive
 *
 * Description :  Stores the client's keep alive timeout.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK.
 *
 *********************************************************************/
sp_err parsers::client_keep_alive(client_state *csp, char **header)
{
   unsigned int keep_alive_timeout;
   const char *timeout_position = strstr(*header, ": ");

   if (!(csp->_config->_feature_flags & RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE))
   {
      errlog::log_error(LOG_LEVEL_HEADER,
         "keep-alive support is disabled. Crunching: %s.", *header);
      freez(*header);
      *header = NULL;
      return SP_ERR_OK;
   }

   if ((NULL == timeout_position)
    || (1 != sscanf(timeout_position, ": %u", &keep_alive_timeout)))
   {
      errlog::log_error(LOG_LEVEL_ERROR, "Couldn't parse: %s", *header);
   }
   else
   {
      if (keep_alive_timeout < csp->_config->_keep_alive_timeout)
      {
         errlog::log_error(LOG_LEVEL_HEADER,
            "Reducing keep-alive timeout from %u to %u.",
			   csp->_config->_keep_alive_timeout, keep_alive_timeout);
         csp->_server_connection._keep_alive_timeout = keep_alive_timeout;
      }
      else
      {
         /* XXX: Is this log worthy? */
         errlog::log_error(LOG_LEVEL_HEADER,
            "Client keep-alive timeout is %u. Sticking with %u.",
            keep_alive_timeout, csp->_config->_keep_alive_timeout);
      }
   }

   return SP_ERR_OK;
}
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */



/*********************************************************************
 *
 * Function    :  client_connection
 *
 * Description :  Makes sure a proper "Connection:" header is
 *                set and signals connection_header_adder 
 *                to do nothing.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_connection(client_state *csp, char **header)
{
   static const char connection_close[] = "Connection: close";

   if (!miscutil::strcmpic(*header, connection_close))
   {
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
      if ((csp->_config->_feature_flags & RUNTIME_FEATURE_CONNECTION_SHARING))
      {
	 if (!miscutil::strcmpic(csp->_http._ver, "HTTP/1.1"))
          {
             errlog::log_error(LOG_LEVEL_HEADER,
                "Removing \'%s\' to imply keep-alive.", *header);
             freez(*header);
	     *header = NULL;
          }
          else
          {
             char *old_header = *header;

             *header = strdup("Connection: keep-alive");
             if (header == NULL)
             {
                return SP_ERR_MEMORY;
             }
             errlog::log_error(LOG_LEVEL_HEADER,
                "Replaced: \'%s\' with \'%s\'", old_header, *header);
             freez(old_header);
	     old_header = NULL;
	  }
      }
      else
      {
         errlog::log_error(LOG_LEVEL_HEADER,
            "Keeping the client header '%s' around. "
            "The connection will not be kept alive.",
            *header);
         csp->_flags &= ~CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE;
      }
   }
   else if ((csp->_config->_feature_flags & RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE))
   {
      errlog::log_error(LOG_LEVEL_HEADER,
         "Keeping the client header '%s' around. "
         "The server connection will be kept alive if possible.",
         *header);
      csp->_flags |= CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE;
#endif  /* def FEATURE_CONNECTION_KEEP_ALIVE */
   }
   else
   {
      char *old_header = *header;

      *header = strdup(connection_close);
      if (header == NULL)
      {
         return SP_ERR_MEMORY;
      }
      errlog::log_error(LOG_LEVEL_HEADER,
         "Replaced: \'%s\' with \'%s\'", old_header, *header);
      freez(old_header);
      old_header = NULL;
   }

   /* Signal client_connection_adder() to return early. */
   csp->_flags |= CSP_FLAG_CLIENT_CONNECTION_HEADER_SET;

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  crumble
 *
 * Description :  This is called if a header matches a pattern to "crunch"
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::crumble(client_state *csp, char **header)
{
   (void)csp;
   errlog::log_error(LOG_LEVEL_HEADER, "crumble crunched: %s!", *header);
   freez(*header);
   *header = NULL;
   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  crunch_server_header
 *
 * Description :  Crunch server header if it matches a string supplied by the
 *                user. Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success and always succeeds
 *
 *********************************************************************/
sp_err parsers::crunch_server_header(client_state *csp, char **header)
{
   const char *crunch_pattern;

   /* Do we feel like crunching? */
   if ((csp->_action._flags & ACTION_CRUNCH_SERVER_HEADER))
   {
      crunch_pattern = csp->_action._string[ACTION_STRING_SERVER_HEADER];

      /* Is the current header the lucky one? */
      if (strstr(*header, crunch_pattern))
      {
         errlog::log_error(LOG_LEVEL_HEADER, "Crunching server header: %s (contains: %s)", *header, crunch_pattern);  
         freez(*header);
	 *header = NULL;
      }
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  server_content_type
 *
 * Description :  Set the content-type for filterable types (text/.*,
 *                .*xml.*, javascript and image/gif) unless filtering has been
 *                forbidden (CT_TABOO) while parsing earlier headers.
 *                NOTE: Since text/plain is commonly used by web servers
 *                      for files whose correct type is unknown, we don't
 *                      set CT_TEXT for it.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::server_content_type(client_state *csp, char **header)
{
   /* Remove header if it isn't the first Content-Type header */
   if ((csp->_content_type & CT_DECLARED))
   {
     /*
      * Another, slightly slower, way to see if
      * we already parsed another Content-Type header.
      */
      assert(NULL != parsers::get_header_value(&csp->_headers, "Content-Type:"));

      errlog::log_error(LOG_LEVEL_ERROR,
         "Multiple Content-Type headers. Removing and ignoring: \'%s\'",
         *header);
      freez(*header);
      *header = NULL;

      return SP_ERR_OK;
   }

   /*
    * Signal that the Content-Type has been set.
    */
   csp->_content_type |= CT_DECLARED;

   if (!(csp->_content_type & CT_TABOO))
   {
      /*
       * XXX: The assumption that text/plain is a sign of
       * binary data seems to be somewhat unreasonable nowadays
       * and should be dropped after 3.0.8 is out.
       */
      if ((strstr(*header, "text/") && !strstr(*header, "plain"))
        || strstr(*header, "xml")
        || strstr(*header, "application/x-javascript"))
      {
         csp->_content_type |= CT_TEXT;
      }
      else if (strstr(*header, "image/gif"))
      {
         csp->_content_type |= CT_GIF;
      }
   }

   /*
    * Are we messing with the content type?
    */
   if (csp->_action._flags & ACTION_CONTENT_TYPE_OVERWRITE)
   {
      /*
       * Make sure the user doesn't accidently
       * change the content type of binary documents. 
       */
      if ((csp->_content_type & CT_TEXT) || (csp->_action._flags & ACTION_FORCE_TEXT_MODE))
      {
         freez(*header);
         *header = strdup("Content-Type: ");
         miscutil::string_append(header, csp->_action._string[ACTION_STRING_CONTENT_TYPE]);

         if (header == NULL)
         {
            errlog::log_error(LOG_LEVEL_HEADER, "Insufficient memory to replace Content-Type!");
            return SP_ERR_MEMORY;
         }
         errlog::log_error(LOG_LEVEL_HEADER, "Modified: %s!", *header);
      }
      else
      {
         errlog::log_error(LOG_LEVEL_HEADER, "%s not replaced. "
            "It doesn't look like a content type that should be filtered. "
            "Enable force-text-mode if you know what you're doing.", *header);
      }
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  server_transfer_coding
 *
 * Description :  - Prohibit filtering (CT_TABOO) if transfer coding compresses
 *                - Raise the CSP_FLAG_CHUNKED flag if coding is "chunked"
 *                - Remove header if body was chunked but has been
 *                  de-chunked for filtering.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::server_transfer_coding(client_state *csp, char **header)
{
   /*
    * Turn off pcrs and gif filtering if body compressed
    */
   if (strstr(*header, "gzip") || strstr(*header, "compress") || strstr(*header, "deflate"))
   {
#ifdef FEATURE_ZLIB
      /*
       * XXX: Added to test if we could use CT_GZIP and CT_DEFLATE here.
       */
      errlog::log_error(LOG_LEVEL_INFO, "Marking content type for %s as CT_TABOO because of %s.",
         csp->_http._cmd, *header);
#endif /* def FEATURE_ZLIB */
      csp->_content_type = CT_TABOO;
   }

   /*
    * Raise flag if body chunked
    */
   if (strstr(*header, "chunked"))
     {
      csp->_flags |= CSP_FLAG_CHUNKED;

      /*
       * If the body was modified, it has been de-chunked first
       * and the header must be removed.
       *
       * FIXME: If there is more than one transfer encoding,
       * only the "chunked" part should be removed here.
       */
      if (csp->_flags & CSP_FLAG_MODIFIED)
      {
         errlog::log_error(LOG_LEVEL_HEADER, "Removing: %s", *header);
	 *header = strdup("");
      }
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  server_content_encoding
 *
 * Description :  This function is run twice for each request,
 *                unless FEATURE_ZLIB and filtering are disabled.
 *
 *                The first run is used to check if the content
 *                is compressed, if FEATURE_ZLIB is disabled
 *                filtering is then disabled as well, if FEATURE_ZLIB
 *                is enabled the content is marked for decompression.
 *                
 *                The second run is used to remove the Content-Encoding
 *                header if the decompression was successful.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::server_content_encoding(client_state *csp, char **header)
{
#ifdef FEATURE_ZLIB
   if ((csp->_flags & CSP_FLAG_MODIFIED)
    && (csp->_content_type & (CT_GZIP | CT_DEFLATE)))
   {
      /*
       * We successfully decompressed the content,
       * and have to clean the header now, so the
       * client no longer expects compressed data..
       *
       * XXX: There is a difference between cleaning
       * and removing it completely.
       */
      errlog::log_error(LOG_LEVEL_HEADER, "Crunching: %s", *header);
      freez(*header);
      std::cout << "*header: " << *header << std::endl;
      *header = strdup("");
   }
   else if (strstr(*header, "gzip"))
   {
      /* Mark for gzip decompression */
      csp->_content_type |= CT_GZIP;
   }
   else if (strstr(*header, "deflate"))
   {
      /* Mark for zlib decompression */
      csp->_content_type |= CT_DEFLATE;
   }
   else if (strstr(*header, "compress"))
   {
      /*
       * We can't decompress this; therefore we can't filter
       * it either.
       */
      csp->_content_type |= CT_TABOO;
   }
#else /* !defined(FEATURE_ZLIB) */
   if (strstr(*header, "gzip") || strstr(*header, "compress") || strstr(*header, "deflate"))
   {
      /*
       * Body is compressed, turn off pcrs and gif filtering.
       */
      csp->_content_type |= CT_TABOO;

      /*
       * Log a warning if the user expects the content to be filtered.
       */
      if ((csp->_rlist != NULL) &&
         (!csp->_action.multi[ACTION_MULTI_FILTER].empty()))
      {
         errlog::log_error(LOG_LEVEL_INFO,
            "Compressed content detected, content filtering disabled. "
            "Consider recompiling Privoxy with zlib support or "
            "enable the prevent-compression action.");
      }
   }
#endif /* defined(FEATURE_ZLIB) */

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  server_adjust_content_length
 *
 * Description :  Adjust Content-Length header if we modified
 *                the body.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::server_adjust_content_length(client_state *csp, char **header)
{
   /* Regenerate header if the content was modified. */
   if (csp->_flags & CSP_FLAG_MODIFIED)
   {
      const size_t header_length = 50;
      freez(*header);
      *header = (char*) zalloc(header_length);
      if (*header == NULL)
      {
         return SP_ERR_MEMORY;
      }
      parsers::create_content_length_header(csp->_content_length, *header, header_length);
      errlog::log_error(LOG_LEVEL_HEADER,
         "Adjusted Content-Length to %llu", csp->_content_length);
   }

   return SP_ERR_OK;
}


#ifdef FEATURE_CONNECTION_KEEP_ALIVE
/*********************************************************************
 *
 * Function    :  server_save_content_length
 *
 * Description :  Save the Content-Length sent by the server.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::server_save_content_length(client_state *csp, char **header)
{
   unsigned long long content_length = 0;

   assert(*(*header+14) == ':');

#ifdef _WIN32
   if (1 != sscanf(*header+14, ": %I64u", &content_length))
#else
   if (1 != sscanf(*header+14, ": %llu", &content_length))
#endif
   {
      errlog::log_error(LOG_LEVEL_ERROR, "Crunching invalid header: %s", *header);
      freez(*header);
      *header = NULL;
   }
   else
   {
      csp->_expected_content_length = content_length;
      csp->_flags |= CSP_FLAG_SERVER_CONTENT_LENGTH_SET;
      csp->_flags |= CSP_FLAG_CONTENT_LENGTH_SET;
   }

   return SP_ERR_OK;
}
#endif /* def FEATURE_CONNECTION_KEEP_ALIVE */


/*********************************************************************
 *
 * Function    :  server_content_md5
 *
 * Description :  Crumble any Content-MD5 headers if the document was
 *                modified. FIXME: Should we re-compute instead?
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::server_content_md5(client_state *csp, char **header)
{
   if (csp->_flags & CSP_FLAG_MODIFIED)
   {
      errlog::log_error(LOG_LEVEL_HEADER, "Crunching Content-MD5");
      freez(*header);
      *header = NULL;
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  server_content_disposition
 *
 * Description :  If enabled, blocks or modifies the "Content-Disposition" header.
 *                Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::server_content_disposition(client_state *csp, char **header)
{
   const char *newval;

   /*
    * Are we messing with the Content-Disposition header?
    */
   if ((csp->_action._flags & ACTION_HIDE_CONTENT_DISPOSITION) == 0)
   {
      /* Me tinks not */
      return SP_ERR_OK;
   }

   newval = csp->_action._string[ACTION_STRING_CONTENT_DISPOSITION];

   if ((newval == NULL) || (0 == miscutil::strcmpic(newval, "block")))
   {
      /*
       * Blocking content-disposition header
       */
      errlog::log_error(LOG_LEVEL_HEADER, "Crunching %s!", *header);
      freez(*header);
      *header = NULL;
      return SP_ERR_OK;
   }
   else
   {  
      /*
       * Replacing Content-Disposition header
       */
      freez(*header);
      *header = strdup("Content-Disposition: ");
      miscutil::string_append(header, newval);

      if (*header != NULL)
      {
         errlog::log_error(LOG_LEVEL_HEADER,
            "Content-Disposition header crunched and replaced with: %s", *header);
      }
   }
   return (*header == NULL) ? SP_ERR_MEMORY : SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  server_last_modified
 *
 * Description :  Changes Last-Modified header to the actual date
 *                to help hide-if-modified-since.
 *                Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::server_last_modified(client_state *csp, char **header)
{
   const char *newval;
   char buf[BUFFER_SIZE];

   char newheader[50];
#ifdef HAVE_GMTIME_R
   struct tm gmt;
#endif
   struct tm *timeptr = NULL;
   time_t now, last_modified;                  
   long int days, hours, minutes, seconds;
   
   /*
    * Are we messing with the Last-Modified header?
    */
   if ((csp->_action._flags & ACTION_OVERWRITE_LAST_MODIFIED) == 0)
   {
      /*Nope*/
      return SP_ERR_OK;
   }

   newval = csp->_action._string[ACTION_STRING_LAST_MODIFIED];

   if (0 == miscutil::strcmpic(newval, "block") )
   {
      /*
       * Blocking Last-Modified header. Useless but why not.
       */
      errlog::log_error(LOG_LEVEL_HEADER, "Crunching %s!", *header);
      freez(*header);
      *header = NULL;
      return SP_ERR_OK;
   }
   else if (0 == miscutil::strcmpic(newval, "reset-to-request-time"))
   {  
      /*
       * Setting Last-Modified Header to now.
       */
      cgi::get_http_time(0, buf, sizeof(buf));
      freez(*header);
      *header = strdup("Last-Modified: ");
      miscutil::string_append(header, buf);   

      if (*header == NULL)
      {
         errlog::log_error(LOG_LEVEL_HEADER, "Insufficient memory. Last-Modified header got lost, boohoo.");  
      }
      else
      {
         errlog::log_error(LOG_LEVEL_HEADER, "Reset to present time: %s", *header);
      }
   }
   else if (0 == miscutil::strcmpic(newval, "randomize"))
   {
      const char *header_time = *header + sizeof("Last-Modified:");

      errlog::log_error(LOG_LEVEL_HEADER, "Randomizing: %s", *header);
      now = time(NULL);
#ifdef HAVE_GMTIME_R
      gmtime_r(&now, &gmt);
#elif defined(MUTEX_LOCKS_AVAILABLE)
      seeks_proxy::mutex_lock(&gmtime_mutex);
      gmtime(&now);
      seeks_proxy::mutex_unlock(&gmtime_mutex);
#else
      gmtime(&now);
#endif
      if (SP_ERR_OK != parse_header_time(header_time, &last_modified))
      {
         errlog::log_error(LOG_LEVEL_HEADER, "Couldn't parse: %s in %s (crunching!)", header_time, *header);
         freez(*header);
	 *header = NULL;
      }
      else
      {
         long int rtime = (long int)difftime(now, last_modified);
         if (rtime)
         {
            const int negative_delta = (rtime < 0);

            if (negative_delta)
            {
               rtime *= -1; 
               errlog::log_error(LOG_LEVEL_HEADER, "Server time in the future.");
            }
            rtime = parsers::pick_from_range(rtime);
            if (negative_delta)
            {
               rtime *= -1;
            }
            last_modified += rtime;
#ifdef HAVE_GMTIME_R
            timeptr = gmtime_r(&last_modified, &gmt);
#elif defined(MUTEX_LOCKS_AVAILABLE)
            seeks_proxy::mutex_lock(&gmtime_mutex);
            timeptr = gmtime(&last_modified);
            seeks_proxy::mutex_unlock(&gmtime_mutex);
#else
            timeptr = gmtime(&last_modified);
#endif
            if ((NULL == timeptr) || !strftime(newheader,
                  sizeof(newheader), "%a, %d %b %Y %H:%M:%S GMT", timeptr))
            {
               errlog::log_error(LOG_LEVEL_ERROR,
                  "Randomizing '%s' failed. Crunching the header without replacement.",
                  *header);
               freez(*header);
	       *header = NULL;
               return SP_ERR_OK;
            }

            freez(*header);
            *header = strdup("Last-Modified: ");
            miscutil::string_append(header, newheader);

            if (*header == NULL)
            {
               errlog::log_error(LOG_LEVEL_ERROR, "Insufficient memory, header crunched without replacement.");
               return SP_ERR_MEMORY;  
            }

            days    = rtime / (3600 * 24);
            hours   = rtime / 3600 % 24;
            minutes = rtime / 60 % 60;
            seconds = rtime % 60;

            errlog::log_error(LOG_LEVEL_HEADER,
               "Randomized:  %s (added %d da%s %d hou%s %d minut%s %d second%s",
               *header, days, (days == 1) ? "y" : "ys", hours, (hours == 1) ? "r" : "rs",
               minutes, (minutes == 1) ? "e" : "es", seconds, (seconds == 1) ? ")" : "s)");
         }
         else
         {
            errlog::log_error(LOG_LEVEL_HEADER, "Randomized ... or not. No time difference to work with.");
         }
      }
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  client_accept_encoding
 *
 * Description :  Rewrite the client's Accept-Encoding header so that
 *                if doesn't allow compression, if the action applies.
 *                Note: For HTTP/1.0 the absence of the header is enough.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_accept_encoding(client_state *csp, char **header)
{
   if ((csp->_action._flags & ACTION_NO_COMPRESSION) != 0)
   {
      errlog::log_error(LOG_LEVEL_HEADER, "Suppressed offer to compress content");

      freez(*header);
      *header = NULL;
      
      /* Temporarily disable the correct behaviour to
       * work around a PHP bug. 
       *
       * if (!miscutil::strcmpic(csp->_http.ver, "HTTP/1.1"))
       * {
       *    *header = strdup("Accept-Encoding: identity;q=1.0, *;q=0");
       *    if (*header == NULL)
       *    {
       *       return SP_ERR_MEMORY;
       *    }
       * }
       * 
       */
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  client_te
 *
 * Description :  Rewrite the client's TE header so that
 *                if doesn't allow compression, if the action applies.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_te(client_state *csp, char **header)
{
   if ((csp->_action._flags & ACTION_NO_COMPRESSION) != 0)
   {
      freez(*header);
      *header = NULL;
      errlog::log_error(LOG_LEVEL_HEADER, "Suppressed offer to compress transfer");
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  client_referrer
 *
 * Description :  Handle the "referer" config setting properly.
 *                Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_referrer(client_state *csp, char **header)
{
   const char *parameter;
   /* booleans for parameters we have to check multiple times */
   int parameter_conditional_block;
   int parameter_conditional_forge;
 
#ifdef FEATURE_FORCE_LOAD
   /*
    * Since the referrer can include the prefix even
    * if the request itself is non-forced, we must
    * clean it unconditionally.
    *
    * XXX: strclean is too broad
    */
   parsers::strclean(*header, FORCE_PREFIX);
#endif /* def FEATURE_FORCE_LOAD */

   if ((csp->_action._flags & ACTION_HIDE_REFERER) == 0)
   {
      /* Nothing left to do */
      return SP_ERR_OK;
   }

   parameter = csp->_action._string[ACTION_STRING_REFERER];
   assert(parameter != NULL);
   parameter_conditional_block = (0 == miscutil::strcmpic(parameter, "conditional-block"));
   parameter_conditional_forge = (0 == miscutil::strcmpic(parameter, "conditional-forge"));

   if (!parameter_conditional_block && !parameter_conditional_forge)
   {
      /*
       * As conditional-block and conditional-forge are the only
       * parameters that rely on the original referrer, we can
       * remove it now for all the others.
       */
      freez(*header);
      *header = NULL;
   }

   if (0 == miscutil::strcmpic(parameter, "block"))
   {
      errlog::log_error(LOG_LEVEL_HEADER, "Referer crunched!");
      return SP_ERR_OK;
   }
   else if (parameter_conditional_block || parameter_conditional_forge)
   {
      return handle_conditional_hide_referrer_parameter(header,
         csp->_http._hostport, parameter_conditional_block);
   }
   else if (0 == miscutil::strcmpic(parameter, "forge"))
   {
      return parsers::create_forged_referrer(header, csp->_http._hostport);
   }
   else
   {
      /* interpret parameter as user-supplied referer to fake */
      return parsers::create_fake_referrer(header, parameter);
   }
}


/*********************************************************************
 *
 * Function    :  client_accept_language
 *
 * Description :  Handle the "Accept-Language" config setting properly.
 *                Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_accept_language(client_state *csp, char **header)
{
   const char *newval;

   /*
    * Are we messing with the Accept-Language?
    */
   if ((csp->_action._flags & ACTION_HIDE_ACCEPT_LANGUAGE) == 0)
   {
      /*I don't think so*/
      return SP_ERR_OK;
   }

   newval = csp->_action._string[ACTION_STRING_LANGUAGE];

   if ((newval == NULL) || (0 == miscutil::strcmpic(newval, "block")) )
   {
      /*
       * Blocking Accept-Language header
       */
      errlog::log_error(LOG_LEVEL_HEADER, "Crunching Accept-Language!");
      freez(*header);
      *header = NULL;
      return SP_ERR_OK;
   }
   else
   {  
      /*
       * Replacing Accept-Language header
       */
      freez(*header);
      *header = strdup("Accept-Language: ");
      miscutil::string_append(header, newval);   

      if (*header == NULL)
      {
         errlog::log_error(LOG_LEVEL_ERROR,
            "Insufficient memory. Accept-Language header crunched without replacement.");  
      }
      else
      {
         errlog::log_error(LOG_LEVEL_HEADER,
            "Accept-Language header crunched and replaced with: %s", *header);
      }
   }
   return (*header == NULL) ? SP_ERR_MEMORY : SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  crunch_client_header
 *
 * Description :  Crunch client header if it matches a string supplied by the
 *                user. Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success and always succeeds
 *
 *********************************************************************/
sp_err parsers::crunch_client_header(client_state *csp, char **header)
{
   const char *crunch_pattern;

   /* Do we feel like crunching? */
   if ((csp->_action._flags & ACTION_CRUNCH_CLIENT_HEADER))
   {
      crunch_pattern = csp->_action._string[ACTION_STRING_CLIENT_HEADER];

      /* Is the current header the lucky one? */
      if (strstr(*header, crunch_pattern))
      {
         errlog::log_error(LOG_LEVEL_HEADER, "Crunching client header: %s (contains: %s)", *header, crunch_pattern);  
         freez(*header);
      }
   }
   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  client_uagent
 *
 * Description :  Handle the "user-agent" config setting properly
 *                and remember its original value to enable browser
 *                bug workarounds. Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_uagent(client_state *csp, char **header)
{
   const char *newval;

   if ((csp->_action._flags & ACTION_HIDE_USER_AGENT) == 0)
   {
      return SP_ERR_OK;
   }

   newval = csp->_action._string[ACTION_STRING_USER_AGENT];
   if (newval == NULL)
   {
      return SP_ERR_OK;
   }

   freez(*header);
   *header = strdup("User-Agent: ");
   miscutil::string_append(header, newval);

   errlog::log_error(LOG_LEVEL_HEADER, "Modified: %s", *header);

   return (*header == NULL) ? SP_ERR_MEMORY : SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  client_ua
 *
 * Description :  Handle "ua-" headers properly.  Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_ua(client_state *csp, char **header)
{
   if ((csp->_action._flags & ACTION_HIDE_USER_AGENT) != 0)
   {
      errlog::log_error(LOG_LEVEL_HEADER, "crunched User-Agent!");
      freez(*header);
      *header = NULL;
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  client_from
 *
 * Description :  Handle the "from" config setting properly.
 *                Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_from(client_state *csp, char **header)
{
   const char *newval;

   if ((csp->_action._flags & ACTION_HIDE_FROM) == 0)
   {
      return SP_ERR_OK;
   }

   freez(*header);
   *header = NULL;
   
   newval = csp->_action._string[ACTION_STRING_FROM];

   /*
    * Are we blocking the e-mail address?
    */
   if ((newval == NULL) || (0 == miscutil::strcmpic(newval, "block")) )
   {
      errlog::log_error(LOG_LEVEL_HEADER, "crunched From!");
      return SP_ERR_OK;
   }

   errlog::log_error(LOG_LEVEL_HEADER, " modified");

   *header = strdup("From: ");
   miscutil::string_append(header, newval);

   return (*header == NULL) ? SP_ERR_MEMORY : SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  client_send_cookie
 *
 * Description :  Crunches the "cookie" header if necessary.
 *                Called from `sed'.
 *
 *                XXX: Stupid name, doesn't send squat.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_send_cookie(client_state *csp, char **header)
{
   if (csp->_action._flags & ACTION_NO_COOKIE_READ)
   {
      errlog::log_error(LOG_LEVEL_HEADER, "Crunched outgoing cookie: %s", *header);
      freez(*header);
      *header = NULL;
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  client_x_forwarded
 *
 * Description :  Handle the "x-forwarded-for" config setting properly,
 *                also used in the add_client_headers list.  Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_x_forwarded(client_state *csp, char **header)
{
   if (0 != (csp->_action._flags & ACTION_CHANGE_X_FORWARDED_FOR))
   {
      const char *parameter = csp->_action._string[ACTION_STRING_CHANGE_X_FORWARDED_FOR];

      if (0 == miscutil::strcmpic(parameter, "block"))
      {
         freez(*header);
	 *header = NULL;
         errlog::log_error(LOG_LEVEL_HEADER, "crunched x-forwarded-for!");
      }
      else if (0 == miscutil::strcmpic(parameter, "add"))
      {
         miscutil::string_append(header, ", ");
         miscutil::string_append(header, csp->_ip_addr_str);

         if (*header == NULL)
         {
            return SP_ERR_MEMORY;
         }
         errlog::log_error(LOG_LEVEL_HEADER,
            "Appended client IP address to %s", *header);
         csp->_flags |= CSP_FLAG_X_FORWARDED_FOR_APPENDED;
      }
      else
      {
         errlog::log_error(LOG_LEVEL_FATAL,
            "Invalid change-x-forwarded-for parameter: '%s'", parameter);
      }
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  client_max_forwards
 *
 * Description :  If the HTTP method is OPTIONS or TRACE, subtract one
 *                from the value of the Max-Forwards header field.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_max_forwards(client_state *csp, char **header)
{
   int max_forwards;

   if ((0 == miscutil::strcmpic(csp->_http._gpc, "trace")) ||
       (0 == miscutil::strcmpic(csp->_http._gpc, "options")))
   {
      assert(*(*header+12) == ':');
      if (1 == sscanf(*header+12, ": %d", &max_forwards))
      {
         if (max_forwards > 0)
         {
            snprintf(*header, strlen(*header)+1, "Max-Forwards: %d", --max_forwards);
            errlog::log_error(LOG_LEVEL_HEADER,
               "Max-Forwards value for %s request reduced to %d.",
               csp->_http._gpc, max_forwards);
         }
         else if (max_forwards < 0)
         {
            errlog::log_error(LOG_LEVEL_ERROR, "Crunching invalid header: %s", *header);
            freez(*header);
	    *header = NULL;
         }
      }
      else
      {
         errlog::log_error(LOG_LEVEL_ERROR, "Crunching invalid header: %s", *header);
         freez(*header);
	 *header = NULL;
      }
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  client_host
 *
 * Description :  If the request URI did not contain host and
 *                port information, parse and evaluate the Host
 *                header field.
 *
 *                Also, kill ill-formed HOST: headers as sent by
 *                Apple's iTunes software when used with a proxy.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_host(client_state *csp, char **header)
{
   char *p, *q;

   /*
    * If the header field name is all upper-case, chances are that it's
    * an ill-formed one from iTunes. BTW, killing innocent headers here is
    * not a problem -- they are regenerated later.
    */
   if ((*header)[1] == 'O')
   {
      errlog::log_error(LOG_LEVEL_HEADER, "Killed all-caps Host header line: %s", *header);
      freez(*header);
      *header = NULL;
      return SP_ERR_OK;
   }

   if (!csp->_http._hostport || (*csp->_http._hostport == '*') ||  
       *csp->_http._hostport == ' ' || *csp->_http._hostport == '\0')
   {
      
      if (NULL == (p = strdup((*header)+6)))
      {
         return SP_ERR_MEMORY;
      }
      miscutil::chomp(p);
      if (NULL == (q = strdup(p)))
      {
         freez(p);
	 p = NULL;
         return SP_ERR_MEMORY;
      }

      freez(csp->_http._hostport);
      csp->_http._hostport = p;
      freez(csp->_http._host);
      csp->_http._host = q;
      q = strchr(csp->_http._host, ':');
      if (q != NULL)
      {
         /* Terminate hostname and evaluate port string */
         *q++ = '\0';
         csp->_http._port = atoi(q);
      }
      else
      {
         csp->_http._port = csp->_http._ssl ? 443 : 80;
      }

      errlog::log_error(LOG_LEVEL_HEADER, "New host and port from Host field: %s = %s:%d",
                csp->_http._hostport, csp->_http._host, csp->_http._port);
   }

   /* Signal client_host_adder() to return right away */
   csp->_flags |= CSP_FLAG_HOST_HEADER_IS_SET;

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  client_if_modified_since
 *
 * Description :  Remove or modify the If-Modified-Since header.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_if_modified_since(client_state *csp, char **header)
{
   char newheader[50];
#ifdef HAVE_GMTIME_R
   struct tm gmt;
#endif
   struct tm *timeptr = NULL;
   time_t tm = 0;                  
   const char *newval;
   long int hours, minutes, seconds;
   char * endptr;
   
   if ( 0 == miscutil::strcmpic(*header, "If-Modified-Since: Wed, 08 Jun 1955 12:00:00 GMT"))
   {
      /* 
       * The client got an error message because of a temporary problem,
       * the problem is gone and the client now tries to revalidate our
       * error message on the real server. The revalidation would always
       * end with the transmission of the whole document and there is
       * no need to expose the bogus If-Modified-Since header.
       */
      errlog::log_error(LOG_LEVEL_HEADER, "Crunching useless If-Modified-Since header.");
      freez(*header);
      *header = NULL;
   }
   else if (csp->_action._flags & ACTION_HIDE_IF_MODIFIED_SINCE)
   {
      newval = csp->_action._string[ACTION_STRING_IF_MODIFIED_SINCE];

      if ((0 == miscutil::strcmpic(newval, "block")))
      {
         errlog::log_error(LOG_LEVEL_HEADER, "Crunching %s", *header);
         freez(*header);
	 *header = NULL;
      }
      else /* add random value */
      {
         const char *header_time = *header + sizeof("If-Modified-Since:");

         if (SP_ERR_OK != parse_header_time(header_time, &tm))
         {
            errlog::log_error(LOG_LEVEL_HEADER, "Couldn't parse: %s in %s (crunching!)", header_time, *header);
            freez(*header);
	    *header = NULL;
         }
         else
         {
            long int rtime = strtol(newval, &endptr, 0);
            const int negative_range = (rtime < 0);

            if (rtime)
            {
               errlog::log_error(LOG_LEVEL_HEADER, "Randomizing: %s (random range: %d minut%s)",
                  *header, rtime, (rtime == 1 || rtime == -1) ? "e": "es");
               if (negative_range)
               {
                  rtime *= -1; 
               }
               rtime *= 60;
               rtime = parsers::pick_from_range(rtime);
            }
            else
            {
               errlog::log_error(LOG_LEVEL_ERROR, "Random range is 0. Assuming time transformation test.",
                  *header);
            }
            tm += rtime * (negative_range ? -1 : 1);
#ifdef HAVE_GMTIME_R
            timeptr = gmtime_r(&tm, &gmt);
#elif defined(MUTEX_LOCKS_AVAILABLE)
            seeks_proxy::mutex_lock(&gmtime_mutex);
            timeptr = gmtime(&tm);
            seeks_proxy::mutex_unlock(&gmtime_mutex);
#else
            timeptr = gmtime(&tm);
#endif
            if ((NULL == timeptr) || !strftime(newheader,
                  sizeof(newheader), "%a, %d %b %Y %H:%M:%S GMT", timeptr))
            {
               errlog::log_error(LOG_LEVEL_ERROR,
                  "Randomizing '%s' failed. Crunching the header without replacement.",
                  *header);
               freez(*header);
	       *header = NULL;
               return SP_ERR_OK;
            }

            freez(*header);
            *header = strdup("If-Modified-Since: ");
            miscutil::string_append(header, newheader);

            if (*header == NULL)
            {
               errlog::log_error(LOG_LEVEL_HEADER, "Insufficient memory, header crunched without replacement.");
               return SP_ERR_MEMORY;  
            }

            hours   = rtime / 3600;
            minutes = rtime / 60 % 60;
            seconds = rtime % 60;

            errlog::log_error(LOG_LEVEL_HEADER,
               "Randomized:  %s (%s %d hou%s %d minut%s %d second%s",
               *header, (negative_range) ? "subtracted" : "added", hours,
               (hours == 1) ? "r" : "rs", minutes, (minutes == 1) ? "e" : "es",
               seconds, (seconds == 1) ? ")" : "s)");
         }
      }
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  client_if_none_match
 *
 * Description :  Remove the If-None-Match header.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_if_none_match(client_state *csp, char **header)
{
   if (csp->_action._flags & ACTION_CRUNCH_IF_NONE_MATCH)
   {  
      errlog::log_error(LOG_LEVEL_HEADER, "Crunching %s", *header);
      freez(*header);
      *header = NULL;
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  client_x_filter
 *
 * Description :  Disables filtering if the client set "X-Filter: No".
 *                Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success
 *
 *********************************************************************/
sp_err parsers::client_x_filter(client_state *csp, char **header)
{
   if ( 0 == miscutil::strcmpic(*header, "X-Filter: No"))
   {
      if (!(csp->_config->_feature_flags & RUNTIME_FEATURE_HTTP_TOGGLE))
      {
         errlog::log_error(LOG_LEVEL_INFO, "Ignored the client's request to fetch without filtering.");
      }
      else
      {
         if (csp->_action._flags & ACTION_FORCE_TEXT_MODE)
         {
            errlog::log_error(LOG_LEVEL_HEADER,
               "force-text-mode overruled the client's request to fetch without filtering!");
         }
         else
         {  
            csp->_content_type = CT_TABOO; /* XXX: This hack shouldn't be necessary */
            csp->_flags |= CSP_FLAG_NO_FILTERING;
            errlog::log_error(LOG_LEVEL_HEADER, "Accepted the client's request to fetch without filtering.");
         }
         errlog::log_error(LOG_LEVEL_HEADER, "Crunching %s", *header);
         freez(*header);
	 *header = NULL;
      }
   }
   return SP_ERR_OK; 
}


/*********************************************************************
 *
 * Function    :  client_range
 *
 * Description :  Removes Range, Request-Range and If-Range headers if
 *                content filtering is enabled. If the client's version
 *                of the document has been altered by Privoxy, the server
 *                could interpret the range differently than the client
 *                intended in which case the user could end up with
 *                corrupted content.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK
 *
 *********************************************************************/
sp_err parsers::client_range(client_state *csp, char **header)
{
   if (filters::content_filters_enabled(&csp->_action))
   {
      errlog::log_error(LOG_LEVEL_HEADER, "Content filtering is enabled."
         " Crunching: \'%s\' to prevent range-mismatch problems.", *header);
      freez(*header);
      *header = NULL;
   }

   return SP_ERR_OK; 
}

/* the following functions add headers directly to the header list */

/*********************************************************************
 *
 * Function    :  client_host_adder
 *
 * Description :  Adds the Host: header field if it is missing.
 *                Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_host_adder(client_state *csp)
{
   char *p;
   sp_err err;

   if (csp->_flags & CSP_FLAG_HOST_HEADER_IS_SET)
   {
      /* Header already set by the client, nothing to do. */
      return SP_ERR_OK;
   }

   if ( !csp->_http._hostport || !*(csp->_http._hostport))
   {
      /* XXX: When does this happen and why is it OK? */
      errlog::log_error(LOG_LEVEL_INFO, "Weirdness in client_host_adder detected and ignored.");
      return SP_ERR_OK;
   }

   /*
    * remove 'user:pass@' from 'proto://user:pass@host'
    */
   if ( (p = strchr( csp->_http._hostport, '@')) != NULL )
   {
      p++;
   }
   else
   {
      p = csp->_http._hostport;
   }

   /* XXX: Just add it, we already made sure that it will be unique */
   errlog::log_error(LOG_LEVEL_HEADER, "addh-unique: Host: %s", p);
   err = miscutil::enlist_unique_header(&csp->_headers, "Host", p);
   return err;
}

/*********************************************************************
 *
 * Function    :  client_xtra_adder
 *
 * Description :  Used in the add_client_headers list.  Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_xtra_adder(client_state *csp)
{
   sp_err err;

   std::list<const char*>::const_iterator lit = csp->_action._multi[ACTION_MULTI_ADD_HEADER].begin();
   while(lit!=csp->_action._multi[ACTION_MULTI_ADD_HEADER].end())
     {
	const char *str = (*lit);
	errlog::log_error(LOG_LEVEL_HEADER, "addh: %s", str);
	err = miscutil::enlist(&csp->_headers, str);
	if (err)
	  {
	     return err;
	  }
   }
   
   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  client_x_forwarded_for_adder
 *
 * Description :  Used in the add_client_headers list.  Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_x_forwarded_for_adder(client_state *csp)
{
   char *header = NULL;
   sp_err err;

   if (!((csp->_action._flags & ACTION_CHANGE_X_FORWARDED_FOR)
         && (0 == miscutil::strcmpic(csp->_action._string[ACTION_STRING_CHANGE_X_FORWARDED_FOR], "add")))
      || (csp->_flags & CSP_FLAG_X_FORWARDED_FOR_APPENDED))
   {
      /*
       * If we aren't adding X-Forwarded-For headers,
       * or we already appended an existing X-Forwarded-For
       * header, there's nothing left to do here.
       */
      return SP_ERR_OK;
   }

   header = strdup("X-Forwarded-For: ");
   miscutil::string_append(&header, csp->_ip_addr_str);

   if (header == NULL)
   {
      return SP_ERR_MEMORY;
   }

   errlog::log_error(LOG_LEVEL_HEADER, "addh: %s", header);
   err = miscutil::enlist(&csp->_headers, header);
   freez(header);
   header = NULL;
   
   return err;
}


/*********************************************************************
 *
 * Function    :  server_connection_adder
 *
 * Description :  Adds an appropiate "Connection:" header to csp->_headers
 *                unless the header was already present. Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::server_connection_adder(client_state *csp)
{
   const unsigned int flags = csp->_flags;
   const char *response_status_line = (*csp->_headers.begin());
   static const char connection_close[] = "Connection: close";

   if ((flags & CSP_FLAG_CLIENT_HEADER_PARSING_DONE)
    && (flags & CSP_FLAG_SERVER_CONNECTION_HEADER_SET))
   {
      return SP_ERR_OK;
   }

   /*
    * XXX: if we downgraded the response, this check will fail.
    */
   if ((csp->_config->_feature_flags &
        RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE)
    && (NULL != response_status_line)
    && !miscutil::strncmpic(response_status_line, "HTTP/1.1", 8)
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
    && !(csp->_flags & CSP_FLAG_SERVER_SOCKET_TAINTED)
#endif
       )
   {
      errlog::log_error(LOG_LEVEL_HEADER, "A HTTP/1.1 response "
         "without Connection header implies keep-alive.");
      csp->_flags |= CSP_FLAG_SERVER_CONNECTION_KEEP_ALIVE;
      return SP_ERR_OK;
   }

   errlog::log_error(LOG_LEVEL_HEADER, "Adding: %s", connection_close);

   return miscutil::enlist(&csp->_headers, connection_close);
}


#ifdef FEATURE_CONNECTION_KEEP_ALIVE
/*********************************************************************
 *
 * Function    :  server_proxy_connection_adder
 *
 * Description :  Adds a "Proxy-Connection: keep-alive" header to
 *                csp->_headers if the client asked for keep-alive.
 *                XXX: We should reuse existant ones.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::server_proxy_connection_adder(client_state *csp)
{
   static const char proxy_connection_header[] = "Proxy-Connection: keep-alive";
   sp_err err = SP_ERR_OK;

   if ((csp->_flags & CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE)
    && !(csp->_flags & CSP_FLAG_SERVER_SOCKET_TAINTED)
    && !(csp->_flags & CSP_FLAG_SERVER_PROXY_CONNECTION_HEADER_SET))
   {
      errlog::log_error(LOG_LEVEL_HEADER, "Adding: %s", proxy_connection_header);
      err = miscutil::enlist(&csp->_headers, proxy_connection_header);
   }

   return err;
}
#endif /* FEATURE_CONNECTION_KEEP_ALIVE */


/*********************************************************************
 *
 * Function    :  client_connection_header_adder
 *
 * Description :  Adds a proper "Connection:" header to csp->_headers
 *                unless the header was already present. Called from `sed'.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::client_connection_header_adder(client_state *csp)
{
   static const char connection_close[] = "Connection: close";

   if (!(csp->_flags & CSP_FLAG_CLIENT_HEADER_PARSING_DONE)
     && (csp->_flags & CSP_FLAG_CLIENT_CONNECTION_HEADER_SET))
   {
      return SP_ERR_OK;
   }

#ifdef FEATURE_CONNECTION_KEEP_ALIVE
   if ((csp->_config->_feature_flags & RUNTIME_FEATURE_CONNECTION_KEEP_ALIVE)
       && (csp->_http._ssl == 0)
      && !miscutil::strcmpic(csp->_http._ver, "HTTP/1.1"))
   {
      csp->_flags |= CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE;
      return SP_ERR_OK;
   }
#endif /* FEATURE_CONNECTION_KEEP_ALIVE */

   errlog::log_error(LOG_LEVEL_HEADER, "Adding: %s", connection_close);

   return miscutil::enlist(&csp->_headers, connection_close);
}


/*********************************************************************
 *
 * Function    :  server_http
 *
 * Description :  - Save the HTTP Status into csp->_http.status
 *                - Set CT_TABOO to prevent filtering if the answer
 *                  is a partial range (HTTP status 206)
 *                - Rewrite HTTP/1.1 answers to HTTP/1.0 if +downgrade
 *                  action applies.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::server_http(client_state *csp, char **header)
{
   sscanf(*header, "HTTP/%*d.%*d %d", &(csp->_http._status));
   if (csp->_http._status == 206)
   {
      csp->_content_type = CT_TABOO;
   }

   if ((csp->_action._flags & ACTION_DOWNGRADE) != 0)
   {
      /* XXX: Should we do a real validity check here? */
      if (strlen(*header) > 8)
      {
         (*header)[7] = '0';
         errlog::log_error(LOG_LEVEL_HEADER, "Downgraded answer to HTTP/1.0");
      }
      else
      {
         /*
          * XXX: Should we block the request or
          * miscutil::enlist a valid status code line here?
          */
         errlog::log_error(LOG_LEVEL_INFO, "Malformed server response detected. "
            "Downgrading to HTTP/1.0 impossible.");
      }
   }

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  server_set_cookie
 *
 * Description :  Handle the server "cookie" header properly.
 *                Log cookie to the jar file.  Then "crunch",
 *                accept or rewrite it to a session cookie.
 *                Called from `sed'.
 *
 *                TODO: Allow the user to specify a new expiration
 *                time to cause the cookie to expire even before the
 *                browser is closed.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  header = On input, pointer to header to modify.
 *                On output, pointer to the modified header, or NULL
 *                to remove the header.  This function frees the
 *                original string if necessary.
 *
 * Returns     :  SP_ERR_OK on success, or
 *                SP_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
sp_err parsers::server_set_cookie(client_state *csp, char **header)
{
   time_t now;
   time_t cookie_time; 

   time(&now);

   if ((csp->_action._flags & ACTION_NO_COOKIE_SET) != 0)
   {
      errlog::log_error(LOG_LEVEL_HEADER, "Crunching incoming cookie: %s", *header);
      freez(*header);
      *header = NULL;
   }
   else if ((csp->_action._flags & ACTION_NO_COOKIE_KEEP) != 0)
   {
      /* Flag whether or not to log a message */
      int changed = 0;

      /* A variable to store the tag we're working on */
      char *cur_tag;

      /* Skip "Set-Cookie:" (11 characters) in header */
      cur_tag = *header + 11;

      /* skip whitespace between "Set-Cookie:" and value */
      while (*cur_tag && isspace(*cur_tag))
      {
         cur_tag++;
      }

      /* Loop through each tag in the cookie */
      while (*cur_tag)
      {
         /* Find next tag */
         char *next_tag = strchr(cur_tag, ';');
         if (next_tag != NULL)
         {
            /* Skip the ';' character itself */
            next_tag++;

            /* skip whitespace ";" and start of tag */
            while (*next_tag && isspace(*next_tag))
            {
               next_tag++;
            }
         }
         else
         {
            /* "Next tag" is the end of the string */
            next_tag = cur_tag + strlen(cur_tag);
         }

         /*
          * Check the expiration date to see
          * if the cookie is still valid, if yes,
          * rewrite it to a session cookie.
          */
         if ((miscutil::strncmpic(cur_tag, "expires=", 8) == 0) && *(cur_tag + 8))
         {
            char *expiration_date = cur_tag + 8; /* Skip "[Ee]xpires=" */

            /* Did we detect the date properly? */
            if (SP_ERR_OK != parse_header_time(expiration_date, &cookie_time))
            {
               /*
                * Nope, treat it as if it was still valid.
                *
                * XXX: Should we remove the whole cookie instead?
                */
               errlog::log_error(LOG_LEVEL_ERROR,
				 "Can't parse \'%s\', send by %s. Unsupported time format?", 
				 cur_tag, csp->_http._url);
               parsers::string_move(cur_tag, next_tag);
               changed = 1;
            }
            else
            {
               /*
                * Yes. Check if the cookie is still valid.
                *
                * If the cookie is already expired it's probably
                * a delete cookie and even if it isn't, the browser
                * will discard it anyway.
                */

               /*
                * XXX: timegm() isn't available on some AmigaOS
                * versions and our replacement doesn't work.
                *
                * Our options are to either:
                *
                * - disable session-cookies-only completely if timegm
                *   is missing,
                *
                * - to simply remove all expired tags, like it has
                *   been done until Privoxy 3.0.6 and to live with
                *    the consequence that it can cause login/logout
                *   problems on servers that don't validate their
                *   input properly, or
                *
                * - to replace it with mktime in which
                *   case there is a slight chance of valid cookies
                *   passing as already expired.
                *
                *   This is the way it's currently done and it's not
                *   as bad as it sounds. If the missing GMT offset is
                *   enough to change the result of the expiration check
                *   the cookie will be only valid for a few hours
                *   anyway, which in many cases will be shorter
                *   than a browser session.
                */
               if (cookie_time - now < 0)
               {
                  errlog::log_error(LOG_LEVEL_HEADER,
                     "Cookie \'%s\' is already expired and can pass unmodified.", *header);
                  /* Just in case some clown sets more then one expiration date */
                  cur_tag = next_tag;
               }
               else
               {
                  /*
                   * Still valid, delete expiration date by copying
                   * the rest of the string over it.
                   */
                  parsers::string_move(cur_tag, next_tag);

                  /* That changed the header, need to issue a log message */
                  changed = 1;

                  /*
                   * Note that the next tag has now been moved to *cur_tag,
                   * so we do not need to update the cur_tag pointer.
                   */
               }
            }

         }
         else
         {
            /* Move on to next cookie tag */
            cur_tag = next_tag;
         }
      }

      if (changed)
      {
         assert(NULL != *header);
         errlog::log_error(LOG_LEVEL_HEADER, "Cookie rewritten to a temporary one: %s",
            *header);
      }
   }

   return SP_ERR_OK;
}


#ifdef FEATURE_FORCE_LOAD
/*********************************************************************
 *
 * Function    :  strclean
 *
 * Description :  In-Situ-Eliminate all occurances of substring in
 *                string
 *
 * Parameters  :
 *          1  :  string = string to clean
 *          2  :  substring = substring to eliminate
 *
 * Returns     :  Number of eliminations
 *
 *********************************************************************/
int parsers::strclean(char *string, const char *substring)
{
   int hits = 0;
   size_t len;
   char *pos, *p;

   len = strlen(substring);

   while((pos = strstr(string, substring)) != NULL)
   {
      p = pos + len;
      do
      {
         *(p - len) = *p;
      }
      while (*p++ != '\0');

      hits++;
   }

   return(hits);
}
#endif /* def FEATURE_FORCE_LOAD */


/*********************************************************************
 *
 * Function    :  parse_header_time
 *
 * Description :  Parses time formats used in HTTP header strings
 *                to get the numerical respresentation.
 *
 * Parameters  :
 *          1  :  header_time = HTTP header time as string. 
 *          2  :  result = storage for header_time in seconds
 *
 * Returns     :  SP_ERR_OK if the time format was recognized, or
 *                SP_ERR_PARSE otherwise.
 *
 *********************************************************************/
sp_err parsers::parse_header_time(const char *header_time, time_t *result)
{
   struct tm gmt;

   /*
    * Zero out gmt to prevent time zone offsets.
    *
    * While this is only necessary on some platforms
    * (mingw32 for example), I don't know how to
    * detect these automatically and doing it everywhere
    * shouldn't hurt.
    */
   memset(&gmt, 0, sizeof(gmt));

                            /* Tue, 02 Jun 2037 20:00:00 */
   if ((NULL == strptime(header_time, "%a, %d %b %Y %H:%M:%S", &gmt))
                            /* Tue, 02-Jun-2037 20:00:00 */
    && (NULL == strptime(header_time, "%a, %d-%b-%Y %H:%M:%S", &gmt))
                            /* Tue, 02-Jun-37 20:00:00 */
    && (NULL == strptime(header_time, "%a, %d-%b-%y %H:%M:%S", &gmt))
                        /* Tuesday, 02-Jun-2037 20:00:00 */
    && (NULL == strptime(header_time, "%A, %d-%b-%Y %H:%M:%S", &gmt))
                        /* Tuesday Jun 02 20:00:00 2037 */
    && (NULL == strptime(header_time, "%A %b %d %H:%M:%S %Y", &gmt)))
   {
      return SP_ERR_PARSE;
   }

   *result = timegm(&gmt);

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  get_destination_from_headers
 *
 * Description :  Parse the "Host:" header to get the request's destination.
 *                Only needed if the client's request was forcefully
 *                redirected into Privoxy.
 *
 *                Code mainly copied from client_host() which is currently
 *                run too late for this purpose.
 *
 * Parameters  :
 *          1  :  headers = List of headers (one of them hopefully being
 *                the "Host:" header)
 *          2  :  http = storage for the result (host, port and hostport). 
 *
 * Returns     :  SP_ERR_MEMORY in case of memory problems,
 *                SP_ERR_PARSE if the host header couldn't be found,
 *                SP_ERR_OK otherwise.
 *
 *********************************************************************/
sp_err parsers::get_destination_from_headers(const std::list<const char*> *headers, 
					     http_request *http)
{
   char *q;
   char *p;
   char *host;

   host = parsers::get_header_value(headers, "Host:");

   if (NULL == host)
   {
      errlog::log_error(LOG_LEVEL_ERROR, "No \"Host:\" header found.");
      return SP_ERR_PARSE;
   }

   if (NULL == (p = strdup((host))))
   {
      errlog::log_error(LOG_LEVEL_ERROR, "Out of memory while parsing \"Host:\" header");
      return SP_ERR_MEMORY;
   }
   miscutil::chomp(p);
   if (NULL == (q = strdup(p)))
   {
      freez(p);
      p = NULL;
      errlog::log_error(LOG_LEVEL_ERROR, "Out of memory while parsing \"Host:\" header");
      return SP_ERR_MEMORY;
   }

   freez(http->_hostport);
   http->_hostport = p;
   freez(http->_host);
   http->_host = q;
   q = strchr(http->_host, ':');
   if (q != NULL)
   {
      /* Terminate hostname and evaluate port string */
      *q++ = '\0';
      http->_port = atoi(q);
   }
   else
   {
      http->_port = http->_ssl ? 443 : 80;
   }

   /* Rebuild request URL */
   freez(http->_url);
   http->_url = strdup(http->_ssl ? "https://" : "http://");
   miscutil::string_append(&http->_url, http->_hostport);
   miscutil::string_append(&http->_url, http->_path);
   if (http->_url == NULL)
   {
      return SP_ERR_MEMORY;
   }

   errlog::log_error(LOG_LEVEL_HEADER, "Destination extracted from \"Host:\" header. New request URL: %s",
      http->_url);

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  create_forged_referrer
 *
 * Description :  Helper for client_referrer to forge a referer as
 *                'http://[hostname:port/' to fool stupid
 *                checks for in-site links 
 *
 * Parameters  :
 *          1  :  header   = Pointer to header pointer
 *          2  :  hostport = Host and optionally port as string
 *
 * Returns     :  SP_ERR_OK in case of success, or
 *                SP_ERR_MEMORY in case of memory problems.
 *
 *********************************************************************/
sp_err parsers::create_forged_referrer(char **header, const char *hostport)
{
    assert(NULL == *header);

    *header = strdup("Referer: http://");
    miscutil::string_append(header, hostport);
    miscutil::string_append(header, "/");

    if (NULL == *header)
    {
       return SP_ERR_MEMORY;
    }

    errlog::log_error(LOG_LEVEL_HEADER, "Referer forged to: %s", *header);

    return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  create_fake_referrer
 *
 * Description :  Helper for client_referrer to create a fake referrer
 *                based on a string supplied by the user.
 *
 * Parameters  :
 *          1  :  header   = Pointer to header pointer
 *          2  :  hosthost = Referrer to fake
 *
 * Returns     :  SP_ERR_OK in case of success, or
 *                SP_ERR_MEMORY in case of memory problems.
 *
 *********************************************************************/
sp_err parsers::create_fake_referrer(char **header, const char *fake_referrer)
{
   assert(NULL == *header);

   if ((0 != miscutil::strncmpic(fake_referrer, "http://", 7)) && (0 != miscutil::strncmpic(fake_referrer, "https://", 8)))
   {
      errlog::log_error(LOG_LEVEL_HEADER,
         "Parameter: +hide-referrer{%s} is a bad idea, but I don't care.", fake_referrer);
   }
   *header = strdup("Referer: ");
   miscutil::string_append(header, fake_referrer);

   if (NULL == *header)
   {
      return SP_ERR_MEMORY;
   }

   errlog::log_error(LOG_LEVEL_HEADER, "Referer replaced with: %s", *header);

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  handle_conditional_hide_referrer_parameter
 *
 * Description :  Helper for client_referrer to crunch or forge
 *                the referrer header if the host has changed.
 *
 * Parameters  :
 *          1  :  header = Pointer to header pointer
 *          2  :  host   = The target host (may include the port)
 *          3  :  parameter_conditional_block = Boolean to signal
 *                if we're in conditional-block mode. If not set,
 *                we're in conditional-forge mode.
 *
 * Returns     :  SP_ERR_OK in case of success, or
 *                SP_ERR_MEMORY in case of memory problems.
 *
 *********************************************************************/
sp_err parsers::handle_conditional_hide_referrer_parameter(char **header,
   const char *host, const int parameter_conditional_block)
{
   char *referer = strdup(*header);
   const size_t hostlenght = strlen(host);
   const char *referer_url = NULL;

   if (NULL == referer)
   {
      freez(*header);
      *header = NULL;
      return SP_ERR_MEMORY;
   }

   /* referer begins with 'Referer: http[s]://' */
   if ((hostlenght+17) < strlen(referer))
   {
      /*
       * Shorten referer to make sure the referer is blocked
       * if www.example.org/www.example.com-shall-see-the-referer/
       * links to www.example.com/
       */
      referer[hostlenght+17] = '\0';
   }
   referer_url = strstr(referer, "http://");
   if ((NULL == referer_url) || (NULL == strstr(referer_url, host)))
   {
      /* Host has changed, Referer is invalid or a https URL. */
      if (parameter_conditional_block)
      {
         errlog::log_error(LOG_LEVEL_HEADER, "New host is: %s. Crunching %s!", host, *header);
         freez(*header);
	 *header = NULL;
      }
      else
      {
         freez(*header);
	 *header = NULL;
         freez(referer);
	 referer = NULL;
         return parsers::create_forged_referrer(header, host);
      }
   }
   freez(referer);
   referer = NULL;

   return SP_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  create_content_length_header
 *
 * Description :  Creates a Content-Length header.
 *
 * Parameters  :
 *          1  :  content_length = The content length to be used in the header.
 *          2  :  header = Allocated space to safe the header.
 *          3  :  buffer_length = The length of the allocated space.
 *
 * Returns     :  void
 *
 *********************************************************************/
void parsers::create_content_length_header(unsigned long long content_length,
					   char *header, size_t buffer_length)
     {
   snprintf(header, buffer_length, "Content-Length: %llu", content_length);
}

/*********************************************************************
 *
 * Function    :  pick_from_range
 *
 * Description :  Pick a positive number out of a given range.
 *                Should only be used if randomness would be nice,
 *                but isn't really necessary.
 *
 * Parameters  :
 *          1  :  range: Highest possible number to pick.
 *
 * Returns     :  Picked number.
 *
 *********************************************************************/
long int parsers::pick_from_range(long int range)
{
   long int number;
#ifdef _WIN32
   static unsigned long seed = 0;
#endif /* def _WIN32 */
   
   assert(range != 0);
   assert(range > 0);
   
   if (range <= 0) return 0;
   
#ifdef HAVE_RANDOM
   number = random() % range + 1;
#elif defined(MUTEX_LOCKS_AVAILABLE)
   seeks_proxy::mutex_lock(&rand_mutex);
# ifdef _WIN32
   if (!seed)
     {
	seed = (unsigned long)(GetCurrentThreadId()+GetTickCount());
     }
   srand(seed);
   seed = (unsigned long)((rand() << 16) + rand());
# endif /* def _WIN32 */
   number = (unsigned long)((rand() << 16) + (rand())) % (unsigned long)(range + 1);
   seeks_proxy::mutex_unlock(&rand_mutex);
#else
   /*
    * XXX: Which platforms reach this and are there
    * better options than just using rand() and hoping
    * that it's safe?
    */
   errlog::log_error(LOG_LEVEL_INFO, "No thread-safe PRNG available? Header time randomization "
		     "might cause crashes, predictable results or even combine these fine options.");
   number = rand() % (long int)(range + 1);   
#endif /* (def HAVE_RANDOM) */
   
   return number;
}

   
   
} /* end of namespace. */
