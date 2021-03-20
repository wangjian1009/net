#ifndef PROMETHEUS_PROCESS_LIMITS_I_H
#define PROMETHEUS_PROCESS_LIMITS_I_H
#include "prometheus_process_provider_i.h"

struct prometheus_process_limits_row {
    char * limit; /**< Pointer to a string */
    int    soft;  /**< Soft value */
    int    hard;  /**< Hard value */
    char * units; /**< Units  */
};

struct prometheus_process_limits_current_row {
    char * limit; /**< Pointer to a string */
    int    soft;  /**< Soft value */
    int    hard;  /**< Hard value */
    char * units; /**< Units  */
};

prometheus_process_limits_row_t
prometheus_process_limits_row_create(
    prometheus_process_provider_t provider,
    const char * limit, const int soft, const int hard, const char * units);

void prometheus_process_limits_row_free(prometheus_process_limits_row_t row);

prometheus_process_limits_current_row_t prometheus_process_limits_current_row_new(void);
int prometheus_process_limits_current_row_set_limit(prometheus_process_limits_current_row_t * self, char * limit);
int prometheus_process_limits_current_row_set_units(prometheus_process_limits_current_row_t * self, char * units);
int prometheus_process_limits_current_row_clear(prometheus_process_limits_current_row_t * self);
int prometheus_process_limits_current_row_destroy(prometheus_process_limits_current_row_t * self);

void prometheus_process_limits_load(
    prometheus_process_provider_t provider, mem_buffer_t buffer, const char * path);

/* prometheus_map_t * prometheus_process_limits(prometheus_process_limits_file_t * f); */

/* uint8_t prometheus_process_limits_rdp_file( */
/*     prometheus_process_limits_file_t * f, prometheus_map_t * data, */
/*     prometheus_process_limits_current_row_t * current_row); */
/* uint8_t prometheus_process_limits_rdp_first_line( */
/*     prometheus_process_limits_file_t * f, prometheus_map_t * data, */
/*     prometheus_process_limits_current_row_t * current_row); */
/* uint8_t prometheus_process_limits_rdp_character( */
/*     prometheus_process_limits_file_t * f, prometheus_map_t * data, */
/*     prometheus_process_limits_current_row_t * current_row); */
/* uint8_t prometheus_process_limits_rdp_letter( */
/*     prometheus_process_limits_file_t * f, prometheus_map_t * data, */
/*     prometheus_process_limits_current_row_t * current_row); */
/* uint8_t prometheus_process_limits_rdp_digit( */
/*     prometheus_process_limits_file_t * f, prometheus_map_t * data, */
/*     prometheus_process_limits_current_row_t * current_row); */
/* uint8_t prometheus_process_limits_rdp_data_line( */
/*     prometheus_process_limits_file_t * f, prometheus_map_t * data, */
/*     prometheus_process_limits_current_row_t * current_row); */
/* uint8_t prometheus_process_limits_rdp_limit( */
/*     prometheus_process_limits_file_t * f, prometheus_map_t * data, */
/*     prometheus_process_limits_current_row_t * current_row); */
/* uint8_t prometheus_process_limits_rdp_word_and_space( */
/*     prometheus_process_limits_file_t * f, prometheus_map_t * data, */
/*     prometheus_process_limits_current_row_t * current_row); */
/* uint8_t prometheus_process_limits_rdp_word( */
/*     prometheus_process_limits_file_t * f, prometheus_map_t * data, */
/*     prometheus_process_limits_current_row_t * current_row); */
/* uint8_t prometheus_process_limits_rdp_soft_limit( */
/*     prometheus_process_limits_file_t * f, prometheus_map_t * data, */
/*     prometheus_process_limits_current_row_t * current_row); */

/* uint8_t prometheus_process_limits_rdp_hard_limit( */
/*     prometheus_process_limits_file_t * f, prometheus_map_t * data, */
/*     prometheus_process_limits_current_row_t * current_row); */
/* uint8_t prometheus_process_limits_rdp_units( */
/*     prometheus_process_limits_file_t * f, prometheus_map_t * data, */
/*     prometheus_process_limits_current_row_t * current_row); */

int prometheus_process_limits_rdp_next_token(prometheus_process_limits_file_t * f);
uint8_t prometheus_process_limits_rdp_match(prometheus_process_limits_file_t * f, const char * token);

#endif
