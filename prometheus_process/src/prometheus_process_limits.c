#include "cpe/pal/pal_errno.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/pal/pal_unistd.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/string_utils.h"
#include "prometheus_gauge.h"
//#include "prometheus_map_i.h"
#include "prometheus_process_limits_i.h"

const char PROM_PROCESS_LIMITS_RDP_LETTERS[] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

const char PROM_PROCESS_LIMITS_RDP_DIGITS[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

const char * PROM_PROCESS_LIMITS_RDP_UNLIMITED = "unlimited";

typedef enum prometheus_process_limit_rdp_limit_type {
    PROM_PROCESS_LIMITS_RDP_SOFT,
    PROM_PROCESS_LIMITS_RDP_HARD
} prometheus_process_limit_rdp_limit_type_t;

prometheus_process_limits_row_t
prometheus_process_limits_row_create(
    prometheus_process_provider_t provider,
    const char * limit, const int soft, const int hard, const char * units)
{
    prometheus_process_limits_row_t self = mem_alloc(provider->m_alloc, sizeof(struct prometheus_process_limits_row));

    self->limit = cpe_str_mem_dup(provider->m_alloc, limit);
    self->units = cpe_str_mem_dup(provider->m_alloc, units);
    self->soft = soft;
    self->hard = hard;

    return self;
}

void prometheus_process_limits_row_free(prometheus_process_limits_row_t row) {
    prometheus_process_provider_t provider = NULL;
    
    /* if (row->limits) { */
        
    /* } */
    
    /* prometheus_free((void *)self->limit); */
    /* self->limit = NULL; */

    /* prometheus_free((void *)self->units); */
    /* self->units = NULL; */

    /* prometheus_free(self); */
    /* self = NULL; */
    /* return 0; */
}

/* prometheus_process_limits_current_row_t */
/* prometheus_process_limits_current_row_new(void) { */
/*     prometheus_process_limits_current_row_t row = (prometheus_process_limits_current_row_t *)prometheus_malloc(sizeof(prometheus_process_limits_current_row_t)); */

/*     self->limit = NULL; */
/*     self->soft = 0; */
/*     self->hard = 0; */
/*     self->units = NULL; */
/*     return self; */
/* } */

/* int prometheus_process_limits_current_row_set_limit(prometheus_process_limits_current_row_t row, char * limit) { */
/*     PROM_ASSERT(self != NULL); */
/*     self->limit = prometheus_strdup(limit); */
/*     return 0; */
/* } */

/* int prometheus_process_limits_current_row_set_units(prometheus_process_limits_current_row_t row, char * units) { */
/*     PROM_ASSERT(self != NULL); */
/*     self->units = prometheus_strdup(units); */
/*     return 0; */
/* } */

/* int prometheus_process_limits_current_row_clear(prometheus_process_limits_current_row_t row) { */
/*     PROM_ASSERT(self != NULL); */
/*     if (self->limit) { */
/*         prometheus_free((void *)self->limit); */
/*         self->limit = NULL; */
/*     } */
/*     if (self->units) { */
/*         prometheus_free((void *)self->units); */
/*         self->units = NULL; */
/*     } */
/*     self->soft = 0; */
/*     self->hard = 0; */
/*     return 0; */
/* } */

/* int prometheus_process_limits_current_row_destroy(prometheus_process_limits_current_row_t row) { */
/*     PROM_ASSERT(self != NULL); */
/*     if (self == NULL) return 0; */
/*     int r = 0; */

/*     r = prometheus_process_limits_current_row_clear(self); */
/*     prometheus_free(self); */
/*     self = NULL; */
/*     return r; */
/* } */

/* //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// */
/* // prometheus_process_limits_file_t */

/* prometheus_process_limits_file_t * prometheus_process_limits_file_new(const char * path) { */
/*     if (path) { */
/*         return prometheus_procfs_buf_new(path); */
/*     } else { */
/*         int pid = (int)getpid(); */
/*         char path[255]; */
/*         sprintf(path, "/proc/%d/limits", pid); */
/*         return prometheus_procfs_buf_new(path); */
/*     } */
/* } */

/* int prometheus_process_limits_file_destroy(prometheus_process_limits_file_t * self) { */
/*     PROM_ASSERT(self != NULL); */
/*     int r = 0; */

/*     if (self == NULL) return 0; */
/*     r = prometheus_procfs_buf_destroy(self); */
/*     self = NULL; */
/*     return r; */
/* } */

/* static void prometheus_process_limits_file_free_map_item_fn(void * gen) { */
/*     prometheus_process_limits_row_t * row = (prometheus_process_limits_row_t *)gen; */
/*     prometheus_process_limits_row_destroy(row); */
/*     row = NULL; */
/* } */

/* /\** */
/*  * @brief Returns a map. Each key is a key in /proc/[pid]/limits. Each value is a pointer to a */
/*  * prometheus_process_limits_row_t. Returns NULL upon failure. */
/*  * */
/*  * EBNF */
/*  * */
/*  * limits_file = first_line , data_line , { data_line } ; */
/*  * first_line = character, { character } , "\n" ; */
/*  * character = " " | letter | digit ; */
/*  * letter = "A" | "B" | "C" | "D" | "E" | "F" | "G" */
/*  *        | "H" | "I" | "J" | "K" | "L" | "M" | "N" */
/*  *        | "O" | "P" | "Q" | "R" | "S" | "T" | "U" */
/*  *        | "V" | "W" | "X" | "Y" | "Z" | "a" | "b" */
/*  *        | "c" | "d" | "e" | "f" | "g" | "h" | "i" */
/*  *        | "j" | "k" | "l" | "m" | "n" | "o" | "p" */
/*  *        | "q" | "r" | "s" | "t" | "u" | "v" | "w" */
/*  *        | "x" | "y" | "z" ; */
/*  * digit = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ; */
/*  * data_line = limit , space, space, { " " }, soft_limit, " ", " ", { " " }, hard_limit, " ", " ", { " " }, { units }, { */
/*  * space_char }, "\n" ; space_char = " " | "\t" ; limit = { word_and_space } , word ; word_and_space = word, " " ; word */
/*  * = letter, { letter } ; soft_limit = ( digit, { digit } ) | "unlimited" ; hard_limit = soft_limit ; units = word ; */
/*  *\/ */
/* /\* prometheus_map_t * prometheus_process_limits(prometheus_process_limits_file_t f) { *\/ */
/* /\*     prometheus_map_t * m = prometheus_map_new(); *\/ */
/* /\*     if (m == NULL) return NULL; *\/ */
/* /\*     int r = 0; *\/ */

/* /\*     r = prometheus_map_set_free_value_fn(m, &prometheus_process_limits_file_free_map_item_fn); *\/ */
/* /\*     if (r) { *\/ */
/* /\*         prometheus_map_destroy(m); *\/ */
/* /\*         m = NULL; *\/ */
/* /\*         return NULL; *\/ */
/* /\*     } *\/ */

/* /\*     prometheus_process_limits_current_row_t * current_row = prometheus_process_limits_current_row_new(); *\/ */
/* /\*     if (!prometheus_process_limits_rdp_file(f, m, current_row)) { *\/ */
/* /\*         prometheus_process_limits_current_row_destroy(current_row); *\/ */
/* /\*         current_row = NULL; *\/ */
/* /\*         prometheus_map_destroy(m); *\/ */
/* /\*         m = NULL; *\/ */
/* /\*         return NULL; *\/ */
/* /\*     } *\/ */
/* /\*     r = prometheus_process_limits_current_row_destroy(current_row); *\/ */
/* /\*     current_row = NULL; *\/ */
/* /\*     if (r) return NULL; *\/ */
/* /\*     return m; *\/ */
/* /\* } *\/ */

/* uint8_t prometheus_process_limits_rdp_file(prometheus_process_limits_file_t f, prometheus_map_t * map, */
/*     prometheus_process_limits_current_row_t * current_row) { */
/*     if (!prometheus_process_limits_rdp_first_line(f, map, current_row)) return false; */

/*     while (f->index < f->size - 1) { */
/*         if (!prometheus_process_limits_rdp_data_line(f, map, current_row)) return false; */
/*     } */

/*     return true; */
/* } */

/* uint8_t prometheus_process_limits_rdp_first_line(prometheus_process_limits_file_t f, prometheus_map_t * map, */
/*     prometheus_process_limits_current_row_t * current_row) { */
/*     while (prometheus_process_limits_rdp_character(f, map, current_row)) { */
/*     } */
/*     if (f->buf[f->index] == '\n') { */
/*         f->index++; */
/*         return true; */
/*     } */
/*     return false; */
/* } */

/* uint8_t prometheus_process_limits_rdp_character(prometheus_process_limits_file_t f, prometheus_map_t * map, */
/*     prometheus_process_limits_current_row_t * current_row) { */
/*     if (prometheus_process_limits_rdp_letter(f, map, current_row)) return true; */
/*     if (prometheus_process_limits_rdp_digit(f, map, current_row)) return true; */
/*     if (f->buf[f->index] == ' ' && f->buf[f->index] < f->size - 1) { */
/*         f->index++; */
/*         return true; */
/*     } */
/*     return false; */
/* } */

/* uint8_t prometheus_process_limits_rdp_letter(prometheus_process_limits_file_t f, prometheus_map_t * map, */
/*     prometheus_process_limits_current_row_t * current_row) { */
/*     if (f->index >= f->size - 1) return false; */
/*     unsigned int size = sizeof(PROM_PROCESS_LIMITS_RDP_LETTERS); */
/*     for (int i = 0; i < size; i++) { */
/*         int letter = PROM_PROCESS_LIMITS_RDP_LETTERS[i]; */
/*         int in_buff = f->buf[f->index]; */
/*         if (letter == in_buff) { */
/*             f->index++; */
/*             return true; */
/*         } */
/*     } */
/*     return false; */
/* } */

/* uint8_t prometheus_process_limits_rdp_digit(prometheus_process_limits_file_t f, prometheus_map_t * map, */
/*     prometheus_process_limits_current_row_t * current_row) { */
/*     if (f->index >= f->size - 1) return false; */
/*     unsigned int size = sizeof(PROM_PROCESS_LIMITS_RDP_DIGITS); */
/*     for (int i = 0; i < size; i++) { */
/*         int digit = PROM_PROCESS_LIMITS_RDP_DIGITS[i]; */
/*         int in_buff = f->buf[f->index]; */
/*         if (digit == in_buff) { */
/*             f->index++; */
/*             return true; */
/*         } */
/*     } */
/*     return false; */
/* } */

/* uint8_t prometheus_process_limits_rdp_data_line(prometheus_process_limits_file_t f, prometheus_map_t * map, */
/*     prometheus_process_limits_current_row_t * current_row) { */
/*     // Process and parse data line, loading relevant data into the current_row */
/*     if (!prometheus_process_limits_rdp_limit(f, map, current_row)) return false; */
/*     prometheus_process_limits_rdp_next_token(f); */
/*     if (!prometheus_process_limits_rdp_soft_limit(f, map, current_row)) return false; */
/*     prometheus_process_limits_rdp_next_token(f); */
/*     if (!prometheus_process_limits_rdp_hard_limit(f, map, current_row)) return false; */
/*     prometheus_process_limits_rdp_next_token(f); */
/*     prometheus_process_limits_rdp_units(f, map, current_row); */

/*     // Load data from the current row into the map */
/*     const char * limit = (const char *)current_row->limit; */
/*     int soft = current_row->soft; */
/*     int hard = current_row->hard; */
/*     const char * units = (const char *)current_row->units; */
/*     prometheus_process_limits_row_t * row = prometheus_process_limits_row_new(limit, soft, hard, units); */
/*     prometheus_map_set(map, limit, row); */
/*     prometheus_process_limits_current_row_clear(current_row); */

/*     // Progress to the next token */
/*     prometheus_process_limits_rdp_next_token(f); */
/*     return true; */
/* } */

/* /\** */
/*  * @brief EBNF: space_char = " " | "\t" ; */
/*  *\/ */
/* uint8_t prometheus_process_limits_rdp_space_char(prometheus_process_limits_file_t f, prometheus_map_t * map, */
/*     prometheus_process_limits_current_row_t * current_row) { */
/*     char c = f->buf[f->index]; */
/*     if (c == ' ' || c == '\t') { */
/*         f->index++; */
/*         return true; */
/*     } */
/*     return false; */
/* } */

/* /\** */
/*  * @brief EBNF: limit = { word_and_space } , word ; */
/*  *\/ */
/* uint8_t prometheus_process_limits_rdp_limit(prometheus_process_limits_file_t f, prometheus_map_t * map, */
/*     prometheus_process_limits_current_row_t * current_row) { */
/*     size_t current_index = f->index; */
/*     while (prometheus_process_limits_rdp_word_and_space(f, map, current_row)) { */
/*     } */

/*     if (prometheus_process_limits_rdp_word(f, map, current_row)) { */
/*         size_t size = f->index - current_index + 1; // Add one for \0 */
/*         char limit[size]; */
/*         for (int i = 0; i < size - 1; i++) { */
/*             limit[i] = f->buf[current_index + i]; */
/*         } */
/*         limit[size - 1] = '\0'; */
/*         prometheus_process_limits_current_row_set_limit(current_row, limit); */
/*         return true; */
/*     } */
/*     return false; */
/* } */

/* /\** */
/*  * @brief EBNF: word_and_space = letter, { letter }, " " ; */
/*  *\/ */
/* uint8_t prometheus_process_limits_rdp_word_and_space(prometheus_process_limits_file_t f, prometheus_map_t * map, */
/*     prometheus_process_limits_current_row_t * current_row) { */
/*     size_t current_index = f->index; */
/*     if (prometheus_process_limits_rdp_word(f, map, current_row) && f->buf[f->index] == ' ') { */
/*         f->index++; */
/*         if (f->index + 1 < f->size && f->buf[f->index + 1] != ' ' && f->buf[f->index + 1] != '\t') { */
/*             return true; */
/*         } */
/*     } */
/*     f->index = current_index; */
/*     return false; */
/* } */

/* uint8_t prometheus_process_limits_rdp_word(prometheus_process_limits_file_t f, prometheus_map_t * map, */
/*     prometheus_process_limits_current_row_t * current_row) { */
/*     size_t original_index = f->index; */
/*     while (prometheus_process_limits_rdp_letter(f, map, current_row)) { */
/*     } */
/*     return (f->index - original_index) > 0; */
/* } */

/* static uint8_t prometheus_process_limits_rdp_generic_limit(prometheus_process_limits_file_t f, prometheus_map_t * map, */
/*     prometheus_process_limits_current_row_t * current_row, */
/*     prometheus_process_limit_rdp_limit_type_t type) { */
/*     size_t current_index = f->index; */
/*     int value = 0; */
/*     if (prometheus_process_limits_rdp_match(f, PROM_PROCESS_LIMITS_RDP_UNLIMITED)) { */
/*         value = -1; */
/*     } else { */
/*         while (prometheus_process_limits_rdp_digit(f, map, current_row)) { */
/*         } */
/*         size_t num_digits = f->index - current_index + 1; */
/*         if (num_digits <= 0) return false; */

/*         char buf[num_digits + 1]; */

/*         for (size_t i = 0; i < num_digits - 1; i++) { */
/*             buf[i] = f->buf[current_index + i]; */
/*         } */
/*         buf[num_digits - 1] = '\0'; */
/*         value = atoi(buf); */
/*         f->index += num_digits; */
/*     } */

/*     switch (type) { */
/*     case PROM_PROCESS_LIMITS_RDP_SOFT: */
/*         current_row->soft = value; */
/*     case PROM_PROCESS_LIMITS_RDP_HARD: */
/*         current_row->hard = value; */
/*     } */

/*     return true; */
/* } */

/* uint8_t prometheus_process_limits_rdp_soft_limit(prometheus_process_limits_file_t f, prometheus_map_t * map, */
/*     prometheus_process_limits_current_row_t * current_row) { */
/*     return prometheus_process_limits_rdp_generic_limit(f, map, current_row, PROM_PROCESS_LIMITS_RDP_SOFT); */
/* } */

/* uint8_t prometheus_process_limits_rdp_hard_limit(prometheus_process_limits_file_t f, prometheus_map_t * map, */
/*     prometheus_process_limits_current_row_t * current_row) { */
/*     return prometheus_process_limits_rdp_generic_limit(f, map, current_row, PROM_PROCESS_LIMITS_RDP_HARD); */
/* } */

/* uint8_t prometheus_process_limits_rdp_units(prometheus_process_limits_file_t f, prometheus_map_t * map, */
/*     prometheus_process_limits_current_row_t * current_row) { */
/*     size_t current_index = f->index; */
/*     if (prometheus_process_limits_rdp_word(f, map, current_row)) { */
/*         size_t num_chars = f->index - current_index + 1; */
/*         char buf[num_chars]; */
/*         for (size_t i = 0; i < num_chars - 1; i++) { */
/*             buf[i] = f->buf[current_index + i]; */
/*         } */
/*         buf[num_chars - 1] = '\0'; */
/*         prometheus_process_limits_current_row_set_units(current_row, buf); */
/*         return true; */
/*     } */
/*     return false; */
/* } */

/* int prometheus_process_limits_rdp_next_token(prometheus_process_limits_file_t f) { */
/*     while (f->buf[f->index] == ' ' || f->buf[f->index] == '\n' || f->buf[f->index] == '\t') { */
/*         f->index++; */
/*     } */
/*     return 0; */
/* } */

/* uint8_t prometheus_process_limits_rdp_match(prometheus_process_limits_file_t f, const char * token) { */
/*     size_t current_index = f->index; */
/*     for (size_t i = 0; i < strlen(token); i++) { */
/*         if (f->buf[current_index + i] != token[i]) { */
/*             return false; */
/*         } */
/*     } */
/*     f->index += strlen(token); */
/*     return true; */
/* } */
