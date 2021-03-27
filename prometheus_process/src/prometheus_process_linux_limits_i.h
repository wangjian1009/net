#ifndef PROMETHEUS_PROCESS_LINUX_LIMITS_I_H
#define PROMETHEUS_PROCESS_LINUX_LIMITS_I_H
#include "prometheus_process_provider_i.h"

struct prometheus_process_linux_limits_row {
    char * limit; /* Pointer to a string */
    int    soft;  /* Soft value */
    int    hard;  /* Hard value */
    char * units; /* Units  */
};

typedef void (*prometheus_process_linux_limits_on_row_fun_t)(void * ctx, struct prometheus_process_linux_limits_row * row);

struct prometheus_process_linux_limits_parse_ctx {
    prometheus_process_provider_t m_provider;
    const char * m_buf;
    size_t m_size;
    size_t m_index;
    void * m_on_row_ctx;
    prometheus_process_linux_limits_on_row_fun_t m_on_row;
};

void prometheus_process_linux_limits_row_init(prometheus_process_provider_t provider, prometheus_process_linux_limits_row_t row);
void prometheus_process_linux_limits_row_fini(prometheus_process_provider_t provider, prometheus_process_linux_limits_row_t row);
void prometheus_process_linux_limits_row_set_units(
    prometheus_process_provider_t provider, prometheus_process_linux_limits_row_t row, const char * units);
void prometheus_process_linux_limits_row_set_limit(
    prometheus_process_provider_t provider, prometheus_process_linux_limits_row_t row, const char * limit);

/*limits*/
int prometheus_process_linux_limits_load(
    prometheus_process_provider_t provider,
    void * ctx, prometheus_process_linux_limits_on_row_fun_t on_row);

int prometheus_process_linux_limits_parse(
    prometheus_process_provider_t provider,
    const char * data, uint32_t data_size,
    void * ctx, prometheus_process_linux_limits_on_row_fun_t on_row);

uint8_t prometheus_process_linux_limits_rdp_first_line(
    prometheus_process_linux_limits_parse_ctx_t ctx, prometheus_process_linux_limits_row_t row);

uint8_t prometheus_process_linux_limits_rdp_character(
    prometheus_process_linux_limits_parse_ctx_t ctx, prometheus_process_linux_limits_row_t row);

uint8_t prometheus_process_linux_limits_rdp_letter(
    prometheus_process_linux_limits_parse_ctx_t ctx, prometheus_process_linux_limits_row_t row);

uint8_t prometheus_process_linux_limits_rdp_digit(
    prometheus_process_linux_limits_parse_ctx_t ctx, prometheus_process_linux_limits_row_t row);

uint8_t prometheus_process_linux_limits_rdp_file(
    prometheus_process_linux_limits_parse_ctx_t ctx, prometheus_process_linux_limits_row_t row);
uint8_t prometheus_process_linux_limits_rdp_data_line(
    prometheus_process_linux_limits_parse_ctx_t ctx, prometheus_process_linux_limits_row_t row);
uint8_t prometheus_process_linux_limits_rdp_limit(
    prometheus_process_linux_limits_parse_ctx_t ctx, prometheus_process_linux_limits_row_t row);
uint8_t prometheus_process_linux_limits_rdp_word_and_space(
    prometheus_process_linux_limits_parse_ctx_t ctx, prometheus_process_linux_limits_row_t row);
uint8_t prometheus_process_linux_limits_rdp_word(
    prometheus_process_linux_limits_parse_ctx_t ctx, prometheus_process_linux_limits_row_t row);
uint8_t prometheus_process_linux_limits_rdp_soft_limit(
    prometheus_process_linux_limits_parse_ctx_t ctx, prometheus_process_linux_limits_row_t row);
uint8_t prometheus_process_linux_limits_rdp_hard_limit(
    prometheus_process_linux_limits_parse_ctx_t ctx, prometheus_process_linux_limits_row_t row);
uint8_t prometheus_process_linux_limits_rdp_units(
    prometheus_process_linux_limits_parse_ctx_t ctx, prometheus_process_linux_limits_row_t row);

int prometheus_process_linux_limits_rdp_next_token(prometheus_process_linux_limits_parse_ctx_t ctx);
uint8_t prometheus_process_linux_limits_rdp_match(prometheus_process_linux_limits_parse_ctx_t ctx, const char * token);

#endif
