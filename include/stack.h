#ifndef STACK_H
#define STACK_H

#include "types.h"

void push8(MEM_WORD DATA);
void push16(MEM_TWO_WORDS DATA);

MEM_WORD pop8();
MEM_TWO_WORDS pop16();

#endif