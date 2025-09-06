#ifndef LIBS_ML_ML_INTERNAL_H
#define LIBS_ML_ML_INTERNAL_H

#include <features.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef __STDC_WANT_IEC_60559_BFP_EXT__
#define __STDC_WANT_IEC_60559_BFP_EXT__ 1
#endif

#ifndef __STDC_WANT_IEC_60559_EXT__
#define __STDC_WANT_IEC_60559_EXT__ 1
#endif

#ifndef __STDC_WANT_LIB_EXT2__
#define __STDC_WANT_LIB_EXT2__ 1
#endif

#ifndef __STDC_WANT_IEC_60559_TYPES_EXT__
#define __STDC_WANT_IEC_60559_TYPES_EXT__ 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ml.h"
#include "ml_features.h"
#include "classifiers.h"
#include "training.h"
#include "models.h"

#include "../metadata/metadata.h"
#include "../fuzzyhash/fuzzy_hash.h"

#ifdef __cplusplus
extern "C" {
#endif

char* strdup_safe(const char* str);

#ifdef __cplusplus
}
#endif

#endif // LIBS_ML_ML_INTERNAL_H
