#include <assert.h>
#include "cpe/pal/pal_stdio.h"
#include "net_http_svr_request_parser.h"
#include "net_http_svr_request_i.h"

#if defined CALLBACK
#undef CALLBACK
#endif

static int unhex[] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
                     ,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
                     ,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
                     , 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1
                     ,-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1
                     ,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
                     ,-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1
                     ,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
                     };
#define TRUE 1
#define FALSE 0
#define MIN(a,b) (a < b ? a : b)

#define REMAINING (pe - p)
#define CURRENT (parser->current_request)
#define CONTENT_LENGTH (parser->current_request->content_length)
#define CALLBACK(FOR)                                                                  \
    if (CURRENT && parser->FOR##_mark) {                                               \
        net_http_svr_request_on_##FOR(CURRENT, parser->FOR##_mark, p - parser->FOR##_mark); \
    }
#define HEADER_CALLBACK(FOR)                                                                                         \
    if (CURRENT && parser->FOR##_mark) {                                                                             \
        net_http_svr_request_on_##FOR(CURRENT, parser->FOR##_mark, p - parser->FOR##_mark, CURRENT->m_number_of_headers); \
    }
#define END_REQUEST                           \
    if (CURRENT)                              \
        net_http_svr_request_on_complete(CURRENT); \
    CURRENT = NULL;

%%{
  machine net_http_svr_request_parser;

  action mark_header_field   { parser->header_field_mark   = p; }
  action mark_header_value   { parser->header_value_mark   = p; }
  action mark_fragment       { parser->fragment_mark       = p; }
  action mark_query_string   { parser->query_string_mark   = p; }
  action mark_request_path   { parser->path_mark           = p; }
  action mark_request_uri    { parser->uri_mark            = p; }

  action write_field { 
    HEADER_CALLBACK(header_field);
    parser->header_field_mark = NULL;
  }

  action write_value {
    HEADER_CALLBACK(header_value);
    parser->header_value_mark = NULL;
  }

  action request_uri { 
    CALLBACK(uri);
    parser->uri_mark = NULL;
  }

  action fragment { 
    CALLBACK(fragment);
    parser->fragment_mark = NULL;
  }

  action query_string { 
    CALLBACK(query_string);
    parser->query_string_mark = NULL;
  }

  action request_path {
    CALLBACK(path);
    parser->path_mark = NULL;
  }

  action content_length {
    if(CURRENT){
      CURRENT->m_content_length *= 10;
      CURRENT->m_content_length += *p - '0';
    }
  }

  action use_identity_encoding { if(CURRENT) CURRENT->m_transfer_encoding = net_http_svr_request_transfer_encoding_identity; }
  action use_chunked_encoding { if(CURRENT) CURRENT->m_transfer_encoding = net_http_svr_request_transfer_encoding_chunked; }

  action set_keep_alive { if(CURRENT) CURRENT->m_keep_alive = TRUE; }
  action set_not_keep_alive { if(CURRENT) CURRENT->m_keep_alive = FALSE; }

  action expect_continue {
    if(CURRENT) CURRENT->m_expect_continue = TRUE;
  }

  action trailer {
    /* not implemenetd yet. (do requests even have trailing headers?) */
  }

  action version_major {
    if(CURRENT) {
      CURRENT->m_version_major *= 10;
      CURRENT->m_version_major += *p - '0';
    }
  }

  action version_minor {
  	if(CURRENT) {
      CURRENT->m_version_minor *= 10;
      CURRENT->m_version_minor += *p - '0';
    }
  }

  action end_header_line {
    if(CURRENT) CURRENT->m_number_of_headers++;
  }

  action end_headers {
    if(CURRENT)
        net_http_svr_request_on_head_complete(CURRENT);
  }

  action add_to_chunk_size {
    parser->chunk_size *= 16;
    parser->chunk_size += unhex[(int)*p];
  }

  action skip_chunk_data {
    skip_body(&p, parser, MIN(parser->chunk_size, REMAINING));
    fhold; 
    if(parser->chunk_size > REMAINING) {
      fbreak;
    } else {
      fgoto chunk_end; 
    }
  }

  action end_chunked_body {
    END_REQUEST;
    fnext main;
  }

  action start_req {
    assert(CURRENT == NULL);
    CURRENT = parser->new_request(parser->data);
  }

  action body_logic {
    if(CURRENT) { 
      if(CURRENT->m_transfer_encoding == net_http_svr_request_transfer_encoding_chunked) {
        fnext ChunkedBody;
      } else {
        /* this is pretty stupid. i'd prefer to combine this with skip_chunk_data */
        parser->chunk_size = CURRENT->m_content_length;
        p += 1;  
        skip_body(&p, parser, MIN(REMAINING, CURRENT->m_content_length));
        fhold;
        if(parser->chunk_size > REMAINING) {
          fbreak;
        } 
      }
    }
  }

#
##
###
#### HTTP/1.1 STATE MACHINE
###
##   RequestHeaders and character types are from
#    Zed Shaw's beautiful Mongrel parser.

  CRLF = "\r\n";

# character types
  CTL = (cntrl | 127);
  safe = ("$" | "-" | "_" | ".");
  extra = ("!" | "*" | "'" | "(" | ")" | ",");
  reserved = (";" | "/" | "?" | ":" | "@" | "&" | "=" | "+");
  unsafe = (CTL | " " | "\"" | "#" | "%" | "<" | ">");
  national = any -- (alpha | digit | reserved | extra | safe | unsafe);
  unreserved = (alpha | digit | safe | extra | national);
  escape = ("%" xdigit xdigit);
  uchar = (unreserved | escape);
  pchar = (uchar | ":" | "@" | "&" | "=" | "+");
  tspecials = ("(" | ")" | "<" | ">" | "@" | "," | ";" | ":" | "\\" | "\"" | "/" | "[" | "]" | "?" | "=" | "{" | "}" | " " | "\t");

# elements
  token = (ascii -- (CTL | tspecials));
  quote = "\"";
#  qdtext = token -- "\""; 
#  quoted_pair = "\" ascii;
#  quoted_string = "\"" (qdtext | quoted_pair )* "\"";

#  headers

  Method = ( "COPY"      %{ if(CURRENT) CURRENT->m_method = net_http_svr_request_method_copy;      }
           | "DELETE"    %{ if(CURRENT) CURRENT->m_method = net_http_svr_request_method_delete;    }
           | "GET"       %{ if(CURRENT) CURRENT->m_method = net_http_svr_request_method_get;       }
           | "HEAD"      %{ if(CURRENT) CURRENT->m_method = net_http_svr_request_method_head;      }
           | "LOCK"      %{ if(CURRENT) CURRENT->m_method = net_http_svr_request_method_lock;      }
           | "MKCOL"     %{ if(CURRENT) CURRENT->m_method = net_http_svr_request_method_mkcol;     }
           | "MOVE"      %{ if(CURRENT) CURRENT->m_method = net_http_svr_request_method_move;      }
           | "OPTIONS"   %{ if(CURRENT) CURRENT->m_method = net_http_svr_request_method_options;   }
           | "POST"      %{ if(CURRENT) CURRENT->m_method = net_http_svr_request_method_post;      }
           | "PROPFIND"  %{ if(CURRENT) CURRENT->m_method = net_http_svr_request_method_propfind;  }
           | "PROPPATCH" %{ if(CURRENT) CURRENT->m_method = net_http_svr_request_method_proppatch; }
           | "PUT"       %{ if(CURRENT) CURRENT->m_method = net_http_svr_request_method_put;       }
           | "TRACE"     %{ if(CURRENT) CURRENT->m_method = net_http_svr_request_method_trace;     }
           | "UNLOCK"    %{ if(CURRENT) CURRENT->m_method = net_http_svr_request_method_unlock;    }
           ); # Not allowing extension methods

  HTTP_Version = "HTTP/" digit+ $version_major "." digit+ $version_minor;

  scheme = ( alpha | digit | "+" | "-" | "." )* ;
  absolute_uri = (scheme ":" (uchar | reserved )*);
  path = ( pchar+ ( "/" pchar* )* ) ;
  query = ( uchar | reserved )* >mark_query_string %query_string ;
  param = ( pchar | "/" )* ;
  params = ( param ( ";" param )* ) ;
  rel_path = ( path? (";" params)? ) ;
  absolute_path = ( "/"+ rel_path ) >mark_request_path %request_path ("?" query)?;
  Request_URI = ( "*" | absolute_uri | absolute_path ) >mark_request_uri %request_uri;
  Fragment = ( uchar | reserved )* >mark_fragment %fragment;

  field_name = ( token -- ":" )+;
  Field_Name = field_name >mark_header_field %write_field;

  field_value = ((any - " ") any*)?;
  Field_Value = field_value >mark_header_value %write_value;

  hsep = ":" " "*;
  header = (field_name hsep field_value) :> CRLF;
  Header = ( ("Content-Length"i hsep digit+ $content_length)
           | ("Connection"i hsep 
               ( "Keep-Alive"i %set_keep_alive
               | "close"i %set_not_keep_alive
               )
             )
           | ("Transfer-Encoding"i %use_chunked_encoding hsep "identity" %use_identity_encoding)
         # | ("Expect"i hsep "100-continue"i %expect_continue)
         # | ("Trailer"i hsep field_value %trailer)
           | (Field_Name hsep Field_Value)
           ) :> CRLF;

  Request_Line = ( Method " " Request_URI ("#" Fragment)? " " HTTP_Version CRLF ) ;
  RequestHeader = Request_Line (Header %end_header_line)* :> CRLF @end_headers;

# chunked message
  trailing_headers = header*;
  #chunk_ext_val   = token | quoted_string;
  chunk_ext_val = token*;
  chunk_ext_name = token*;
  chunk_extension = ( ";" " "* chunk_ext_name ("=" chunk_ext_val)? )*;
  last_chunk = "0"+ chunk_extension CRLF;
  chunk_size = (xdigit* [1-9a-fA-F] xdigit*) $add_to_chunk_size;
  chunk_end  = CRLF;
  chunk_body = any >skip_chunk_data;
  chunk_begin = chunk_size chunk_extension CRLF;
  chunk = chunk_begin chunk_body chunk_end;
  ChunkedBody := chunk* last_chunk trailing_headers CRLF @end_chunked_body;

  Request = RequestHeader >start_req @body_logic;

  main := Request*; # sequence of requests (for keep-alive)
}%%

%% write data;

static void
skip_body(const char** p, net_http_svr_request_parser* parser, size_t nskip) {
    if (CURRENT  && nskip > 0) {
        net_http_svr_request_on_body(CURRENT, *p, nskip);
    }
    if (CURRENT) CURRENT->m_body_read += nskip;
    parser->chunk_size -= nskip;
    *p += nskip;
    if (0 == parser->chunk_size) {
        parser->eating = FALSE;
        if (CURRENT && CURRENT->m_transfer_encoding == net_http_svr_request_transfer_encoding_identity) {
            END_REQUEST;
        }
    } else {
        parser->eating = TRUE;
    }
}

void net_http_svr_request_parser_init(net_http_svr_request_parser* parser) {
    int cs = 0;
    %% write init;
    parser->cs = cs;

    parser->chunk_size = 0;
    parser->eating = 0;
    parser->current_request = NULL;
    parser->header_field_mark = NULL;
    parser->header_value_mark = NULL;
    parser->query_string_mark = NULL;
    parser->path_mark = NULL;
    parser->uri_mark = NULL;
    parser->fragment_mark = NULL;
    parser->new_request = NULL;
}

/** exec **/
size_t net_http_svr_request_parser_execute(net_http_svr_request_parser* parser, const char* buffer, size_t len) {
    const char *p, *pe;
    int cs = parser->cs;

    assert(parser->new_request && "undefined callback");

    p = buffer;
    pe = buffer + len;

    if (0 < parser->chunk_size && parser->eating) {
        /* eat body */
        size_t eat = MIN(len, parser->chunk_size);
        skip_body(&p, parser, eat);
    }

    if (parser->header_field_mark) parser->header_field_mark = buffer;
    if (parser->header_value_mark) parser->header_value_mark = buffer;
    if (parser->fragment_mark) parser->fragment_mark = buffer;
    if (parser->query_string_mark) parser->query_string_mark = buffer;
    if (parser->path_mark) parser->path_mark = buffer;
    if (parser->uri_mark) parser->uri_mark = buffer;

    %% write exec;

    parser->cs = cs;

    HEADER_CALLBACK(header_field);
    HEADER_CALLBACK(header_value);
    CALLBACK(fragment);
    CALLBACK(query_string);
    CALLBACK(path);
    CALLBACK(uri);

    assert(p <= pe && "buffer overflow after parsing execute");

    return (p - buffer);
}

int net_http_svr_request_parser_has_error(net_http_svr_request_parser *parser) {
    return parser->cs == net_http_svr_request_parser_error;
}

int net_http_svr_request_parser_is_finished(net_http_svr_request_parser *parser) {
    return parser->cs == net_http_svr_request_parser_first_final;
}

