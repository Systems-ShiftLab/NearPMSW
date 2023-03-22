// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * ut_pmem2_map.h -- utility helper functions for libpmem2 map tests
 */

#ifndef UT_PMEM2_MAP_H
#define UT_PMEM2_MAP_H 1

/* a pmem2_map() that can't return NULL */
#define PMEM2_MAP(cfg, src, map)					\
	ut_pmem2_map(__FILE__, __LINE__, __func__, cfg, src, map)

void ut_pmem2_map(const char *file, int line, const char *func,
	struct pmem2_config *cfg, struct pmem2_source *src,
	struct pmem2_map **map);

#endif /* UT_PMEM2_MAP_H */
