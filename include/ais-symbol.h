#ifndef __AIS_SYMBOL_H
#       define __AIS_SYMBOL_H

#include <stddef.h>
#include <stdint.h>
#include "ais.h"

#define AIS_MAX_PARAM 10

typedef struct {
        uint32_t type;
        char *name;
} ais_func_parameter_t;

typedef struct {
        uint32_t arity;
        ais_func_parameter_t parm[AIS_MAX_PARAM];
} ais_func_prototype_t;

typedef struct {
        char *name;
        ais_func_prototype_t *func;
} ais_symbol_t;

#endif
