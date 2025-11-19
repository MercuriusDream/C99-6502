#ifndef ADDRESSING_H
#define ADDRESSING_H

#include "types.h"

MEM_TWO_WORDS addr_zp();
MEM_TWO_WORDS addr_zpx();
MEM_TWO_WORDS addr_zpy();
MEM_TWO_WORDS addr_abs();
MEM_TWO_WORDS addr_absx(int* page_cross);
MEM_TWO_WORDS addr_absy(int* page_cross);
MEM_TWO_WORDS addr_ind();
MEM_TWO_WORDS addr_indx();
MEM_TWO_WORDS addr_indy(int* page_cross);
MEM_TWO_WORDS addr_imm();
MEM_TWO_WORDS addr_rel();
MEM_TWO_WORDS addr_zp_ind();
MEM_TWO_WORDS addr_abs_ind_x();
MEM_TWO_WORDS addr_resolve(ADDR_MODE mode, int* page_cross, int* has_ea);

#endif