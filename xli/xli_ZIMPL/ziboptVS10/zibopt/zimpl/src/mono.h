/* $Id: mono.h,v 1.8 2010/09/30 10:39:10 bzfkocht Exp $ */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*   File....: mono.h                                                        */
/*   Name....: Monom Functions                                               */
/*   Author..: Thorsten Koch                                                 */
/*   Copyright by Author, All rights reserved                                */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*
 * Copyright (C) 2007-2010 by Thorsten Koch <koch@zib.de>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef _MONO_H_
#define _MONO_H_

enum mono_function { MFUN_SQRT = -2, MFUN_NONE = 0, MFUN_TRUE = 1, MFUN_FALSE = 2, MFUN_LOG = 3, MFUN_EXP = 4 };

typedef enum   mono_function MFun;
typedef struct mono_element  MonoElem;
typedef struct mono          Mono;

struct mono_element
{
   SID
   Entry*    entry;
   MonoElem* next;
};

struct mono
{
   SID
   int       count; // Only needed to hopefully compare monoms faster.
   MFun      fun;
   Numb*     coeff;
   MonoElem  first;
};

/*lint -sem(        mono_new, 1p == 1 && 2p == 1, @p == 1) */
extern Mono*        mono_new(const Numb* coeff, const Entry* entry, MFun fun);
/*lint -sem(        mono_is_valid, 1p == 1) */
extern Bool         mono_is_valid(const Mono* mono);
/*lint -sem(        mono_free, custodial(1), 1p == 1) */
extern void         mono_free(Mono* mono);
/*lint -sem(        mono_is_valid, 1p == 1 && 2p == 1) */
extern void         mono_mul_entry(Mono* mono, const Entry* entry);
/*lint -sem(        mono_copy, 1p == 1, @p == 1) */
extern Mono*        mono_copy(const Mono* mono);
/*lint -sem(        mono_mul_coeff, 1p == 1 && 2p == 1) */
extern void         mono_mul_coeff(Mono* term, const Numb* value);
/*lint -sem(        mono_add_coeff, 1p == 1 && 2p == 1) */
extern void         mono_add_coeff(Mono* term, const Numb* value);
/*lint -sem(        mono_is_valid, 1p == 1) */
extern unsigned int mono_hash(const Mono* mono);
/*lint -sem(        mono_equal, 1p == 1 && 2p == 1) */
extern Bool         mono_equal(const Mono* ma, const Mono* mb);
/*lint -sem(        mono_mul, 1p == 1 && 2p == 1, @p == 1) */
extern Mono*        mono_mul(const Mono* ma, const Mono* mb);
/*lint -sem(        mono_neg, 1p == 1) */
extern void         mono_neg(Mono* mono);
/*lint -sem(        mono_is_linear, 1p == 1) */
extern Bool         mono_is_linear(const Mono* mono);
/*lint -sem(        mono_get_count, 1p == 1, @n >= 1) */
extern int          mono_get_degree(const Mono* mono);
/*lint -sem(        mono_get_count, 1p == 1, @p == 1) */
extern const Numb*  mono_get_coeff(const Mono* mono);
/*lint -sem(        mono_set_function, 1p == 1) */
extern void         mono_set_function(Mono* mono, MFun f);
/*lint -sem(        mono_get_function, 1p == 1) */
extern MFun         mono_get_function(const Mono* mono);
/*lint -sem(        mono_get_var, 1p == 1 && 2n >= 0, @p == 1) */
extern Var*         mono_get_var(const Mono* mono, int idx);
/*lint -sem(        mono_print, 1p == 1) */
extern void         mono_print(FILE* fp, const Mono* mono, Bool print_symbol_index);

#endif /* _MONO_H_ */

