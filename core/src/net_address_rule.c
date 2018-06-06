#include "net_address_rule_i.h"

net_address_rule_t
net_address_rule_create(net_address_matcher_t matcher, const char * str_rule) {
    net_address_rule_t rule = mem_alloc(matcher->m_schedule->m_alloc, sizeof(struct net_address_rule));
    if (rule == NULL) {
        CPE_ERROR(matcher->m_schedule->m_em, "net_address_rule_create: alloc fail!");
        return NULL;
    }

    int reerr;
    PCRE2_SIZE reerroffset;
    rule->m_pattern_re = pcre2_compile((PCRE2_SPTR)str_rule, PCRE2_ZERO_TERMINATED, 0, &reerr, &reerroffset, NULL);
    if (rule->m_pattern_re == NULL) {
        CPE_ERROR(
            matcher->m_schedule->m_em, "Regex compilation of \"%s\" failed: %d, offset %d",
            str_rule, reerr, (int)reerroffset);
        mem_free(matcher->m_schedule->m_alloc, rule);
        return NULL;
    }
    
    TAILQ_INSERT_TAIL(&matcher->m_rules, rule, m_next);
    return rule;
}

void net_address_rule_free(net_address_matcher_t matcher, net_address_rule_t rule) {
    pcre2_code_free(rule->m_pattern_re);

    TAILQ_REMOVE(&matcher->m_rules, rule, m_next);

    mem_free(matcher->m_schedule->m_alloc, rule);
}

net_address_rule_t
net_address_rule_lookup(net_address_matcher_t matcher, const char * address) {
    net_address_rule_t rule;

    TAILQ_FOREACH(rule, &matcher->m_rules, m_next) {
        int rc = pcre2_match(rule->m_pattern_re, (PCRE2_SPTR)address, PCRE2_ZERO_TERMINATED, 0, 0, NULL, NULL);
        if (rc >= 0) return NULL;
    }

    return NULL;
}
