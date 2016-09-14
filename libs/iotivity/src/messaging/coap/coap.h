/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

#ifndef COAP_H
#define COAP_H

#include "conf.h"
#include "constants.h"
#include <stddef.h> /* for size_t */
#include <stdint.h>

/* OIC stack headers */
#include "config.h"
#include "oc_buffer.h"
#include "port/oc_connectivity.h"
#include "port/oc_log.h"
#include "port/oc_random.h"

#ifndef MAX
#define MAX(n, m) (((n) < (m)) ? (m) : (n))
#endif

#ifndef MIN
#define MIN(n, m) (((n) < (m)) ? (n) : (m))
#endif

#ifndef ABS
#define ABS(n) (((n) < 0) ? -(n) : (n))
#endif

#define COAP_MAX_PACKET_SIZE (COAP_MAX_HEADER_SIZE + MAX_PAYLOAD_SIZE)

/* MAX_PAYLOAD_SIZE can be different from 2^x so we need to get next lower 2^x
 * for COAP_MAX_BLOCK_SIZE */
#ifndef COAP_MAX_BLOCK_SIZE
#define COAP_MAX_BLOCK_SIZE                                                    \
  (MAX_PAYLOAD_SIZE < 32                                                       \
     ? 16                                                                      \
     : (MAX_PAYLOAD_SIZE < 64                                                  \
          ? 32                                                                 \
          : (MAX_PAYLOAD_SIZE < 128                                            \
               ? 64                                                            \
               : (MAX_PAYLOAD_SIZE < 256                                       \
                    ? 128                                                      \
                    : (MAX_PAYLOAD_SIZE < 512                                  \
                         ? 256                                                 \
                         : (MAX_PAYLOAD_SIZE < 1024                            \
                              ? 512                                            \
                              : (MAX_PAYLOAD_SIZE < 2048 ? 1024 : 2048)))))))
#endif /* COAP_MAX_BLOCK_SIZE */

/* bitmap for set options */
enum
{
  OPTION_MAP_SIZE = sizeof(uint8_t) * 8
};

#define SET_OPTION(packet, opt)                                                \
  ((packet)->options[opt / OPTION_MAP_SIZE] |= 1 << (opt % OPTION_MAP_SIZE))
#define IS_OPTION(packet, opt)                                                 \
  ((packet)->options[opt / OPTION_MAP_SIZE] & (1 << (opt % OPTION_MAP_SIZE)))

/* parsed message struct */
typedef struct
{
  uint8_t *buffer; /* pointer to CoAP header / incoming packet buffer / memory
                      to serialize packet */

  uint8_t version;
  coap_message_type_t type;
  uint8_t code;
  uint16_t mid;

  uint8_t token_len;
  uint8_t token[COAP_TOKEN_LEN];

  uint8_t options[COAP_OPTION_SIZE1 / OPTION_MAP_SIZE +
                  1]; /* bitmap to check if option is set */

  uint16_t content_format; /* parse options once and store; allows setting
                              options in random order  */
  uint32_t max_age;
  uint8_t etag_len;
  uint8_t etag[COAP_ETAG_LEN];
  size_t proxy_uri_len;
  const char *proxy_uri;
  size_t proxy_scheme_len;
  const char *proxy_scheme;
  size_t uri_host_len;
  const char *uri_host;
  size_t location_path_len;
  const char *location_path;
  uint16_t uri_port;
  size_t location_query_len;
  const char *location_query;
  size_t uri_path_len;
  const char *uri_path;
  int32_t observe;
  uint16_t accept;
  uint8_t if_match_len;
  uint8_t if_match[COAP_ETAG_LEN];
  uint32_t block2_num;
  uint8_t block2_more;
  uint16_t block2_size;
  uint32_t block2_offset;
  uint32_t block1_num;
  uint8_t block1_more;
  uint16_t block1_size;
  uint32_t block1_offset;
  uint32_t size2;
  uint32_t size1;
  size_t uri_query_len;
  const char *uri_query;
  uint8_t if_none_match;

  uint16_t payload_len;
  uint8_t *payload;
} coap_packet_t;

/* option format serialization */
#define COAP_SERIALIZE_INT_OPTION(number, field, text)                         \
  if (IS_OPTION(coap_pkt, number)) {                                           \
    LOG(text " [%u]\n", (unsigned int)coap_pkt->field);                        \
    option += coap_serialize_int_option(number, current_number, option,        \
                                        coap_pkt->field);                      \
    current_number = number;                                                   \
  }
#define COAP_SERIALIZE_BYTE_OPTION(number, field, text)                        \
  if (IS_OPTION(coap_pkt, number)) {                                           \
    LOG(text " %u [0x%02X%02X%02X%02X%02X%02X%02X%02X]\n",                     \
        (unsigned int)coap_pkt->field##_len, coap_pkt->field[0],               \
        coap_pkt->field[1], coap_pkt->field[2], coap_pkt->field[3],            \
        coap_pkt->field[4], coap_pkt->field[5], coap_pkt->field[6],            \
        coap_pkt->field[7]); /* FIXME always prints 8 bytes */                 \
    option += coap_serialize_array_option(number, current_number, option,      \
                                          coap_pkt->field,                     \
                                          coap_pkt->field##_len, '\0');        \
    current_number = number;                                                   \
  }
#define COAP_SERIALIZE_STRING_OPTION(number, field, splitter, text)            \
  if (IS_OPTION(coap_pkt, number)) {                                           \
    LOG(text " [%.*s]\n", (int)coap_pkt->field##_len, coap_pkt->field);        \
    option += coap_serialize_array_option(number, current_number, option,      \
                                          (uint8_t *)coap_pkt->field,          \
                                          coap_pkt->field##_len, splitter);    \
    current_number = number;                                                   \
  }
#define COAP_SERIALIZE_BLOCK_OPTION(number, field, text)                       \
  if (IS_OPTION(coap_pkt, number)) {                                           \
    LOG(text " [%lu%s (%u B/blk)]\n", (unsigned long)coap_pkt->field##_num,    \
        coap_pkt->field##_more ? "+" : "", coap_pkt->field##_size);            \
    uint32_t block = coap_pkt->field##_num << 4;                               \
    if (coap_pkt->field##_more) {                                              \
      block |= 0x8;                                                            \
    }                                                                          \
    block |= 0xF & coap_log_2(coap_pkt->field##_size / 16);                    \
    LOG(text " encoded: 0x%lX\n", (unsigned long)block);                       \
    option +=                                                                  \
      coap_serialize_int_option(number, current_number, option, block);        \
    current_number = number;                                                   \
  }

/* to store error code and human-readable payload */
extern coap_status_t erbium_status_code;
extern char *coap_error_message;

void coap_init_connection(void);
uint16_t coap_get_mid(void);

void coap_init_message(void *packet, coap_message_type_t type, uint8_t code,
                       uint16_t mid);
size_t coap_serialize_message(void *packet, uint8_t *buffer);
void coap_send_message(oc_message_t *message);
coap_status_t coap_parse_message(void *request, uint8_t *data,
                                 uint16_t data_len);

int coap_get_query_variable(void *packet, const char *name,
                            const char **output);
int coap_get_post_variable(void *packet, const char *name, const char **output);

/*---------------------------------------------------------------------------*/

int coap_set_status_code(void *packet, unsigned int code);

int coap_set_token(void *packet, const uint8_t *token, size_t token_len);

int coap_get_header_content_format(void *packet, unsigned int *format);
int coap_set_header_content_format(void *packet, unsigned int format);

int coap_get_header_accept(void *packet, unsigned int *accept);
int coap_set_header_accept(void *packet, unsigned int accept);

int coap_get_header_max_age(void *packet, uint32_t *age);
int coap_set_header_max_age(void *packet, uint32_t age);

int coap_get_header_etag(void *packet, const uint8_t **etag);
int coap_set_header_etag(void *packet, const uint8_t *etag, size_t etag_len);

int coap_get_header_if_match(void *packet, const uint8_t **etag);
int coap_set_header_if_match(void *packet, const uint8_t *etag,
                             size_t etag_len);

int coap_get_header_if_none_match(void *packet);
int coap_set_header_if_none_match(void *packet);

int coap_get_header_proxy_uri(
  void *packet,
  const char **uri); /* in-place string might not be 0-terminated. */
int coap_set_header_proxy_uri(void *packet, const char *uri);

int coap_get_header_proxy_scheme(
  void *packet,
  const char **scheme); /* in-place string might not be 0-terminated. */
int coap_set_header_proxy_scheme(void *packet, const char *scheme);

int coap_get_header_uri_host(
  void *packet,
  const char **host); /* in-place string might not be 0-terminated. */
int coap_set_header_uri_host(void *packet, const char *host);

int coap_get_header_uri_path(
  void *packet,
  const char **path); /* in-place string might not be 0-terminated. */
int coap_set_header_uri_path(void *packet, const char *path);

int coap_get_header_uri_query(
  void *packet,
  const char **query); /* in-place string might not be 0-terminated. */
int coap_set_header_uri_query(void *packet, const char *query);

int coap_get_header_location_path(
  void *packet,
  const char **path); /* in-place string might not be 0-terminated. */
int coap_set_header_location_path(void *packet,
                                  const char *path); /* also splits optional
                                                        query into
                                                        Location-Query option.
                                                        */

int coap_get_header_location_query(
  void *packet,
  const char **query); /* in-place string might not be 0-terminated. */
int coap_set_header_location_query(void *packet, const char *query);

int coap_get_header_observe(void *packet, uint32_t *observe);
int coap_set_header_observe(void *packet, uint32_t observe);

int coap_get_header_block2(void *packet, uint32_t *num, uint8_t *more,
                           uint16_t *size, uint32_t *offset);
int coap_set_header_block2(void *packet, uint32_t num, uint8_t more,
                           uint16_t size);

int coap_get_header_block1(void *packet, uint32_t *num, uint8_t *more,
                           uint16_t *size, uint32_t *offset);
int coap_set_header_block1(void *packet, uint32_t num, uint8_t more,
                           uint16_t size);

int coap_get_header_size2(void *packet, uint32_t *size);
int coap_set_header_size2(void *packet, uint32_t size);

int coap_get_header_size1(void *packet, uint32_t *size);
int coap_set_header_size1(void *packet, uint32_t size);

int coap_get_payload(void *packet, const uint8_t **payload);
int coap_set_payload(void *packet, const void *payload, size_t length);

#endif /* COAP_H */
