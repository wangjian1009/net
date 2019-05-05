#include "net_dns_task.h"
#include "net_dns_task_step.h"
#include "net_dns_task_ctx.h"
#include "net_dns_source_i.h"
#include "net_dns_task_builder_i.h"

net_dns_task_builder_t
net_dns_task_builder_create(
    net_dns_manage_t manage,
    uint32_t capacity,
    net_dns_task_builder_init_fun_t init,
    net_dns_task_builder_fini_fun_t fini,
    net_dns_task_build_fun_t build)
{
    net_dns_task_builder_t builder = mem_alloc(manage->m_alloc, sizeof(struct net_dns_task_builder) + capacity);
    if (builder == NULL) {
        CPE_ERROR(manage->m_em, "dns-cli: builder: alloc fail!");
        return NULL;
    }

    builder->m_manage = manage;
    builder->m_capacity = capacity;
    builder->m_init = init;
    builder->m_fini = fini;
    builder->m_build = build;

    if (builder->m_init && builder->m_init(builder) != 0) {
        mem_free(manage->m_alloc, builder);
        return NULL;
    }
    
    TAILQ_INSERT_TAIL(&manage->m_builders, builder, m_next);
    
    return builder;
}

void net_dns_task_builder_free(net_dns_task_builder_t builder) {
    net_dns_manage_t manage = builder->m_manage;

    if (builder->m_fini) builder->m_fini(builder);

    if (manage->m_builder_internal == builder) {
        manage->m_builder_internal = NULL;
    }

    if (manage->m_builder_default == builder) {
        manage->m_builder_default = manage->m_builder_internal;
    }
    
    TAILQ_REMOVE(&manage->m_builders, builder, m_next);

    mem_free(manage->m_alloc, builder);
}

static int net_dns_task_build_internal(net_dns_task_builder_t builder, net_dns_task_t task, const char * policy) {
    net_dns_manage_t manage = builder->m_manage;

    if (TAILQ_EMPTY(&manage->m_sources)) {
        CPE_ERROR(manage->m_em, "dns-cli: builder: no source");
        return -1;
    }
    
    net_dns_source_t source;

    TAILQ_FOREACH(source, &manage->m_sources, m_next) {
        net_dns_task_step_t step = net_dns_task_step_create(task, net_dns_task_step_complete_all);
        if (step == NULL) {
            CPE_ERROR(manage->m_em, "dns-cli: builder: create step fail");
            return -1;
        }

        net_dns_task_ctx_t ctx = net_dns_task_ctx_create(step, source);
        if (ctx == NULL) {
            CPE_ERROR(manage->m_em, "dns-cli: builder: create ctx fail");
            return -1;
        }
    }

    return 0;
}

net_dns_task_builder_t
net_dns_task_builder_internal_create(net_dns_manage_t manage) {
    return net_dns_task_builder_create(manage, 0, NULL, NULL, net_dns_task_build_internal);
}

void * net_dns_task_builder_data(net_dns_task_builder_t builder) {
    return builder + 1;
}

net_dns_task_builder_t net_dns_task_builder_from_data(void * data) {
    return ((net_dns_task_builder_t)data) - 1;
}

int net_dns_task_builder_build(net_dns_task_builder_t builder, net_dns_task_t task, const char * policy) {
    return builder->m_build(builder, task, policy);
}
