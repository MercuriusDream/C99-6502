#ifndef INSTRUCTIONS_IMPLEMENTATION
#define INSTRUCTIONS_IMPLEMENTATION

#include "types.h"
#include "cpu.h"

// HERE COMES THE MOUNTAIN!

void ADC(void);
void SBC(void);
void LDA(void);
void STA(void);
void LDX(void);
void LDY(void);
void STX(void);
void STY(void);
void BRK(void);
void BPL_(void);
void BMI_(void);
void BVC_(void);
void BVS_(void);
void BCC_(void);
void BCS_(void);
void BNE_(void);
void BEQ_(void);
void TAX(void);
void TXA(void);
void TAY(void);
void TYA(void);
void TSX(void);
void TXS(void);
void PHA(void);
void PLA(void);
void PHP(void);
void PLP(void);
void INX(void);
void DEX(void);
void INY(void);
void DEY(void);
void INC(void);
void DEC(void);
void INC_A(void);
void DEC_A(void);
void CLC(void);
void SEC(void);
void CLI(void);
void SEI(void);
void CLD(void);
void SED(void);
void CLV(void);
void NOP(void);
void BRK(void);
void AND(void);
void ORA(void);
void EOR(void);
void ASL(void);
void LSR(void);
void ROL(void);
void ROR(void);
void CMP(void);
void CPX(void);
void CPY(void);
void JMP(void);
void JSR(void);
void RTS(void);
void RTI(void);
void BIT(void);

// Illegal/Undocumented opcodes (stable on real hardware; Therefore implementing)

void LAX(void);
void SAX(void);
void DCP(void);
void ISC(void);
void SLO(void);
void RLA(void);
void SRE(void);
void RRA(void);
void NOP_READ(void);
void ANC(void);
void ALR(void);
void ARR(void);
void ALT_SBC(void);
void op_kil(void);

// CMOS 65C02 instructions
void BRA_(void);
void PHX(void);
void PHY(void);
void PLX(void);
void PLY(void);
void STZ(void);
void TRB(void);
void TSB(void);
void WAI(void);
void STP(void);

// Rockwell/WDC 65C02 bit manipulation instructions
void RMB(void);
void SMB(void);
void BBR_(void);
void BBS_(void);

#endif