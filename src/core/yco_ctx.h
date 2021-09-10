#ifndef _YCO_CTX_H_
#define _YCO_CTX_H_

void yco_push_ctx(void *regs);
void yco_use_ctx(void *regs);
void yco_use_ctx_run(void *regs, void *funcs);

#endif //_YCO_CTX_H_
