#ifndef PARSING_H
#define PARSING_H

#include "../utils/config.h"

typedef struct ParsedMessage ParsedMessage;

void extract_props(const char *src, ParsedMessage *msg);

#endif
