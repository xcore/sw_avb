#ifndef __get_core_id_from_chanend_h__
#define __get_core_id_from_chanend_h__


#if __XC__

static inline unsigned int get_core_id_from_chanend(chanend c) {
 unsigned int ci;
 asm("shr %0, %1, 16":"=r"(ci):"r"(c));
 return ci;
}

#endif


#endif //__get_core_id_from_chanend_h__
