#include "cpe/pal/pal_errno.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/pal/pal_unistd.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "prometheus_gauge.h"
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

void prometheus_process_limits_row_init(
    prometheus_process_provider_t provider, prometheus_process_limits_row_t row)
{
    row->limit = NULL;
    row->soft = 0;
    row->hard = 0;
    row->units = NULL;
}

void prometheus_process_limits_row_fini(
    prometheus_process_provider_t provider, prometheus_process_limits_row_t row)
{
    if (row->limit) {
        mem_free(provider->m_alloc, row->limit);
        row->limit = NULL;
    }

    if (row->units) {
        mem_free(provider->m_alloc, row->units);
        row->units = NULL;
    }
    row->soft = 0;
    row->hard = 0;
}

void prometheus_process_limits_row_set_units(
    prometheus_process_provider_t provider, prometheus_process_limits_row_t row, const char * units)
{
}

void prometheus_process_limits_row_set_limit(
    prometheus_process_provider_t provider, prometheus_process_limits_row_t row, const char * units)
{
}

/**
 * @brief Returns a map. Each key is a key in /proc/[pid]/limits. Each value is a pointer to a
 * prometheus_process_limits_row_t. Returns NULL upon failure.
 *
 * EBNF
 *
 * limits_file = first_line , data_line , { data_line } ;
 * first_line = character, { character } , "\n" ;
 * character = " " | letter | digit ;
 * letter = "A" | "B" | "C" | "D" | "E" | "F" | "G"
 *        | "H" | "I" | "J" | "K" | "L" | "M" | "N"
 *        | "O" | "P" | "Q" | "R" | "S" | "T" | "U"
 *        | "V" | "W" | "X" | "Y" | "Z" | "a" | "b"
 *        | "c" | "d" | "e" | "f" | "g" | "h" | "i"
 *        | "j" | "k" | "l" | "m" | "n" | "o" | "p"
 *        | "q" | "r" | "s" | "t" | "u" | "v" | "w"
 *        | "x" | "y" | "z" ;
 * digit = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
 * data_line = limit , space, space, { " " }, soft_limit, " ", " ", { " " }, hard_limit, " ", " ", { " " }, { units }, {
 * space_char }, "\n" ; space_char = " " | "\t" ; limit = { word_and_space } , word ; word_and_space = word, " " ; word
 * = letter, { letter } ; soft_limit = ( digit, { digit } ) | "unlimited" ; hard_limit = soft_limit ; units = word ;
 */
int prometheus_process_limits_parse(
    prometheus_process_provider_t provider,
    const char * data, uint32_t data_size,
    void * on_row_ctx, prometheus_process_on_row_fun_t on_row)
{
    struct prometheus_process_limits_parse_ctx parse_ctx = {
        provider,
        data, data_size, 0,
        on_row_ctx, on_row
    };

    struct prometheus_process_limits_row row;
    prometheus_process_limits_row_init(provider, &row);
    
    if (!prometheus_process_limits_rdp_file(&parse_ctx, &row)) {
        prometheus_process_limits_row_fini(provider, &row);
        return -1;
    }

    prometheus_process_limits_row_fini(provider, &row);
    return 0;
}

uint8_t prometheus_process_limits_rdp_file(
    prometheus_process_limits_parse_ctx_t ctx, prometheus_process_limits_row_t row)
{
    if (!prometheus_process_limits_rdp_first_line(ctx, row)) return 0;

    while (ctx->m_index < ctx->m_size - 1) {
        if (!prometheus_process_limits_rdp_data_line(ctx, row)) return 0;
    }

    return 1;
}

uint8_t prometheus_process_limits_rdp_first_line(
    prometheus_process_limits_parse_ctx_t ctx, prometheus_process_limits_row_t row)
{
    while (prometheus_process_limits_rdp_character(ctx, row)) {
    }
    if (ctx->m_buf[ctx->m_index] == '\n') {
        ctx->m_index++;
        return 1;
    }

    return 0;
}

uint8_t prometheus_process_limits_rdp_character(
    prometheus_process_limits_parse_ctx_t ctx, prometheus_process_limits_row_t row)
{
    if (prometheus_process_limits_rdp_letter(ctx, row)) return 1;
    if (prometheus_process_limits_rdp_digit(ctx, row)) return 1;
    if (ctx->m_buf[ctx->m_index] == ' ' && ctx->m_buf[ctx->m_index] < ctx->m_size - 1) {
        ctx->m_index++;
        return 1;
    }
    return 0;
}

uint8_t prometheus_process_limits_rdp_letter(
    prometheus_process_limits_parse_ctx_t ctx, prometheus_process_limits_row_t row)
{
    if (ctx->m_index >= ctx->m_size - 1) return 0;
    unsigned int size = sizeof(PROM_PROCESS_LIMITS_RDP_LETTERS);
    for (int i = 0; i < size; i++) {
        int letter = PROM_PROCESS_LIMITS_RDP_LETTERS[i];
        int in_buff = ctx->m_buf[ctx->m_index];
        if (letter == in_buff) {
            ctx->m_index++;
            return 1;
        }
    }
    return 0;
}

uint8_t prometheus_process_limits_rdp_digit(
    prometheus_process_limits_parse_ctx_t ctx, prometheus_process_limits_row_t row)
{
    if (ctx->m_index >= ctx->m_size - 1) return 0;
    unsigned int size = sizeof(PROM_PROCESS_LIMITS_RDP_DIGITS);
    for (int i = 0; i < size; i++) {
        int digit = PROM_PROCESS_LIMITS_RDP_DIGITS[i];
        int in_buff = ctx->m_buf[ctx->m_index];
        if (digit == in_buff) {
            ctx->m_index++;
            return 1;
        }
    }
    return 0;
}

uint8_t prometheus_process_limits_rdp_data_line(
    prometheus_process_limits_parse_ctx_t ctx, prometheus_process_limits_row_t row)
{
    // Process and parse data line, loading relevant data into the row
    if (!prometheus_process_limits_rdp_limit(ctx, row)) return 0;
    prometheus_process_limits_rdp_next_token(ctx);
    if (!prometheus_process_limits_rdp_soft_limit(ctx, row)) return 0;
    prometheus_process_limits_rdp_next_token(ctx);
    if (!prometheus_process_limits_rdp_hard_limit(ctx, row)) return 0;
    prometheus_process_limits_rdp_next_token(ctx);
    prometheus_process_limits_rdp_units(ctx, row);

    // Load data from the current row into the map
    ctx->m_on_row(ctx->m_on_row_ctx, row);
    prometheus_process_limits_row_fini(ctx->m_provider, row);

    // Progress to the next token
    prometheus_process_limits_rdp_next_token(ctx);
    return 1;
}

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
uint8_t prometheus_process_limits_rdp_limit(
    prometheus_process_limits_parse_ctx_t ctx, prometheus_process_limits_row_t row)
{
    size_t current_index = ctx->m_index;
    while (prometheus_process_limits_rdp_word_and_space(ctx, row)) {
    }

    if (prometheus_process_limits_rdp_word(ctx, row)) {
        size_t size = ctx->m_index - current_index + 1; // Add one for \0
        char limit[size];
        for (int i = 0; i < size - 1; i++) {
            limit[i] = ctx->m_buf[current_index + i];
        }
        limit[size - 1] = '\0';
        prometheus_process_limits_row_set_limit(ctx->m_provider, row, limit);
        return 1;
    }
    return 0;
}

/**
 * @brief EBNF: word_and_space = letter, { letter }, " " ;
 */
uint8_t prometheus_process_limits_rdp_word_and_space(
    prometheus_process_limits_parse_ctx_t ctx, prometheus_process_limits_row_t row)
{
    size_t current_index = ctx->m_index;
    if (prometheus_process_limits_rdp_word(ctx, row) && ctx->m_buf[ctx->m_index] == ' ') {
        ctx->m_index++;
        if (ctx->m_index + 1 < ctx->m_size && ctx->m_buf[ctx->m_index + 1] != ' ' && ctx->m_buf[ctx->m_index + 1] != '\t') {
            return 1;
        }
    }
    ctx->m_index = current_index;
    return 0;
}

uint8_t prometheus_process_limits_rdp_word(
    prometheus_process_limits_parse_ctx_t ctx, prometheus_process_limits_row_t row)
{
    size_t original_index = ctx->m_index;
    while (prometheus_process_limits_rdp_letter(ctx, row)) {
    }
    return (ctx->m_index - original_index) > 0;
}

static uint8_t prometheus_process_limits_rdp_generic_limit(
    prometheus_process_limits_parse_ctx_t ctx, prometheus_process_limits_row_t row,
    prometheus_process_limit_rdp_limit_type_t type)
{
    size_t current_index = ctx->m_index;
    int value = 0;
    if (prometheus_process_limits_rdp_match(ctx, PROM_PROCESS_LIMITS_RDP_UNLIMITED)) {
        value = -1;
    } else {
        while (prometheus_process_limits_rdp_digit(ctx, row)) {
        }
        size_t num_digits = ctx->m_index - current_index + 1;
        if (num_digits <= 0) return 0;

        char buf[num_digits + 1];

        for (size_t i = 0; i < num_digits - 1; i++) {
            buf[i] = ctx->m_buf[current_index + i];
        }
        buf[num_digits - 1] = '\0';
        value = atoi(buf);
        ctx->m_index += num_digits;
    }

    switch (type) {
    case PROM_PROCESS_LIMITS_RDP_SOFT:
        row->soft = value;
        break;
    case PROM_PROCESS_LIMITS_RDP_HARD:
        row->hard = value;
        break;
    }

    return 1;
}

uint8_t prometheus_process_limits_rdp_soft_limit(
    prometheus_process_limits_parse_ctx_t ctx, prometheus_process_limits_row_t row)
{
    return prometheus_process_limits_rdp_generic_limit(ctx, row, PROM_PROCESS_LIMITS_RDP_SOFT);
}

uint8_t prometheus_process_limits_rdp_hard_limit(
    prometheus_process_limits_parse_ctx_t ctx, prometheus_process_limits_row_t row)
{
    return prometheus_process_limits_rdp_generic_limit(ctx, row, PROM_PROCESS_LIMITS_RDP_HARD);
}

uint8_t prometheus_process_limits_rdp_units(
    prometheus_process_limits_parse_ctx_t ctx, prometheus_process_limits_row_t row)
{
    size_t current_index = ctx->m_index;
    if (prometheus_process_limits_rdp_word(ctx, row)) {
        size_t num_chars = ctx->m_index - current_index + 1;
        char buf[num_chars];
        for (size_t i = 0; i < num_chars - 1; i++) {
            buf[i] = ctx->m_buf[current_index + i];
        }
        buf[num_chars - 1] = '\0';
        prometheus_process_limits_row_set_units(ctx->m_provider, row, buf);
        return 1;
    }
    return 0;
}

int prometheus_process_limits_rdp_next_token(prometheus_process_limits_parse_ctx_t ctx) {
    while (ctx->m_buf[ctx->m_index] == ' ' || ctx->m_buf[ctx->m_index] == '\n' || ctx->m_buf[ctx->m_index] == '\t') {
        ctx->m_index++;
    }
    return 0;
}

uint8_t prometheus_process_limits_rdp_match(prometheus_process_limits_parse_ctx_t ctx, const char * token) {
    size_t current_index = ctx->m_index;
    for (size_t i = 0; i < strlen(token); i++) {
        if (ctx->m_buf[current_index + i] != token[i]) {
            return 0;
        }
    }
    ctx->m_index += strlen(token);
    return 1;
}
