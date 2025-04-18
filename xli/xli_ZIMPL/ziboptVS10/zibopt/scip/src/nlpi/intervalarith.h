/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the program and library             */
/*         SCIP --- Solving Constraint Integer Programs                      */
/*                                                                           */
/*    Copyright (C) 2002-2010 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SCIP is distributed under the terms of the ZIB Academic License.         */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License              */
/*  along with SCIP; see the file COPYING. If not email to scip@zib.de.      */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#pragma ident "@(#) $Id: intervalarith.h,v 1.1 2010/09/17 17:31:44 bzfviger Exp $"

/**@file   nlpi/intervalarith.h
 * @brief  C++ extensions to interval arithmetics for provable bounds
 * @author Stefan Vigerske
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_INTERVALARITH_HPP__
#define __SCIP_INTERVALARITH_HPP__

#include "scip/intervalarith.h"

/** an interval that extends the SCIP_INTERVAL struct
 * by various methods to allow calculating with intervals as with ordinary numbers */
class SCIPInterval : public SCIP_INTERVAL {
public:
   /** value to use for infinity
    * currently a global variable, thus use with care!
    */
   static SCIP_Real infinity;

   /** default constructor -> gives entire interval */
   SCIPInterval()
   {
      SCIPintervalSetBounds(this, -infinity, infinity);
   }

   /* constructor for an SCIP_INTERVAL struct */
   SCIPInterval(
      const SCIP_INTERVAL& x                 /**< interval to copy */
      )
   {
      SCIPintervalSetBounds(this, x.inf, x.sup);
   }

   /** constructor for an interval giving pointwise bounds */
   SCIPInterval(
      SCIP_Real          infinum,            /**< lower bound of interval */
      SCIP_Real          supremum            /**< upper bound of interval */
      )
   {
      SCIPintervalSetBounds(this, infinum, supremum);
   }

   /** constructor for a singleton */
   SCIPInterval(
      SCIP_Real          number              /**< number to be represented by interval */
      )
   {
      SCIPintervalSet(this, number);
   }

   /** sets interval bounds */
   void setBounds(
      SCIP_Real          newinf,             /**< lower bound of interval */
      SCIP_Real          newsup              /**< upper bound of interval */
      )
   {
      SCIPintervalSetBounds(this, newinf, newsup);
   }

   /** returns whether this interval is equal to another one */
   bool operator==(
      const SCIP_INTERVAL& y                 /**< interval to compare with */
      ) const
   {
      if( SCIPintervalIsEmpty(*this) && !SCIPintervalIsEmpty(y) )
         return false;
      if( this->inf <= -infinity && y.inf > -infinity )
         return false;
      if( this->sup >=  infinity && y.sup <  infinity )
         return false;
      return (this->inf == y.inf) && (this->sup == y.sup);
   }

   /** returns whether this interval is not equal to another one */
   bool operator!=(
      const SCIP_INTERVAL& y                 /**< interval to compare with */
      ) const
   {
      return !operator==(y);
   }

   /** returns whether this interval is equal to a given number */
   bool operator==(
      const SCIP_Real&   y                   /**< number to compare with */
      ) const
   {
      return ( (inf == y) && (sup == y) ) ||
             ( sup <= -infinity && y <= -infinity ) ||
             ( inf >=  infinity && y >=  infinity );
   }

   /** adds another interval to this one */
   SCIPInterval& operator+=(
      const SCIPInterval& y                  /**< interval to add in */
      )
   {
      SCIPintervalAdd(SCIPInterval::infinity, this, *this, y);
      return *this;
   }

   /** substracts an interval from this one */
   SCIPInterval& operator-=(
      const SCIPInterval& y                  /**< interval to substract */
      )
   {
      SCIPintervalSub(SCIPInterval::infinity, this, *this, y);
      return *this;
   }

   /** multiplies an interval with this one */
   SCIPInterval& operator*=(
      const SCIPInterval& y                  /**< interval to multiply with */
      )
   {
      SCIPintervalMul(SCIPInterval::infinity, this, *this, y);
      return *this;
   }

   /** divides this interval by another one */
   SCIPInterval& operator/=(
      const SCIPInterval& y                  /**< interval to divide by */
      )
   {
      SCIPintervalDiv(SCIPInterval::infinity, this, *this, y);
      return *this;
   }

   /** assigns a number to this interval */
   SCIPInterval& operator=(
      const SCIP_Real& y                     /**< new value for both interval bounds */
      )
   {
      SCIPintervalSet(this, y);
      return *this;
   }

   /** assign an interval to this interval */
   SCIPInterval& operator=(
      const SCIP_INTERVAL& y                 /**< new value for this interval */
      )
   {
      SCIPintervalSetBounds(this, y.inf, y.sup);
      return *this;
   }

};

/** addition of two intervals */
inline
SCIPInterval operator+(
   const SCIPInterval&   x,                  /**< first operand */
   const SCIPInterval&   y                   /**< second operand */
   )
{
   SCIPInterval resultant;

   SCIPintervalAdd(SCIPInterval::infinity, &resultant, x, y);

   return resultant;
}

/** substraction for two intervals */
inline
SCIPInterval operator-(
   const SCIPInterval&   x,                  /**< first operand */
   const SCIPInterval&   y                   /**< second operand */
   )
{
   SCIPInterval resultant;

   SCIPintervalSub(SCIPInterval::infinity, &resultant, x, y);

   return resultant;
}

/** negation of an interval */
inline
SCIPInterval operator-(
   const SCIPInterval&   y                   /**< operand */
   )
{
   SCIPInterval resultant;

   SCIPintervalSetBounds(&resultant, -y.sup, -y.inf);

   return resultant;
}

/** multiplication of two intervals */
inline
SCIPInterval operator*(
   const SCIPInterval&   x,                  /**< first operand */
   const SCIPInterval&   y                   /**< second operand */
   )
{
   SCIPInterval resultant;

   SCIPintervalMul(SCIPInterval::infinity, &resultant, x, y);

   return resultant;
}

/** division for two intervals */
inline
SCIPInterval operator/(
   const SCIPInterval&   x,                  /**< first operand */
   const SCIPInterval&   y                   /**< second operand */
   )
{
   SCIPInterval resultant;

   SCIPintervalDiv(SCIPInterval::infinity, &resultant, x, y);

   return resultant;
}

/** cosinus of an interval */
inline
SCIPInterval cos(
   const SCIPInterval&   x                   /**< operand */
   )
{
   /* @todo implement cosinus for intervals */
   SCIPwarningMessage("Cosinus of interval not implemented. Returning trivial interval [-1,1].\n");

   return SCIPInterval(-1., 1.);
}

/** exponential of an interval */
inline
SCIPInterval exp(
   const SCIPInterval&   x                   /**< operand */
   )
{
   SCIPInterval resultant;

   SCIPintervalExp(SCIPInterval::infinity, &resultant, x);

   return resultant;
}

/** natural logarithm of an interval */
inline
SCIPInterval log(
   const SCIPInterval&   x                   /**< operand */
   )
{
   SCIPInterval resultant;

   SCIPintervalLog(SCIPInterval::infinity, &resultant, x);

   return resultant;
}

/** power of an interval to another interval */
inline
SCIPInterval pow(
   const SCIPInterval&   x,                  /**< first operand */
   const SCIPInterval&   y                   /**< second operand */
   )
{
   SCIPInterval resultant;

   SCIPintervalPower(SCIPInterval::infinity, &resultant, x, y);

   return resultant;
}

/** sinus of an interval */
inline
SCIPInterval sin(
   const SCIPInterval&   x                   /**< operand */
   )
{
   /* @todo implement cosinus for intervals */
   SCIPwarningMessage("Sinus of interval not implemented. Returning trivial interval [-1,1].\n");

   return SCIPInterval(-1., 1.);
}

/** square root of an interval */
inline
SCIPInterval sqrt(
   const SCIPInterval&   x                   /**< operand */
   )
{
   SCIPInterval resultant;

   SCIPintervalSquareRoot(SCIPInterval::infinity, &resultant, x);

   return resultant;
}

/** absolute value of an interval */
inline
SCIPInterval abs(
   const SCIPInterval&   x                   /**< operand */
   )
{
   SCIPInterval resultant;

   SCIPintervalAbs(&resultant, x);

   return resultant;
}

/** sign of an interval */
inline
SCIPInterval sign(
   const SCIPInterval&   x                   /**< operand */
   )
{
   SCIPInterval resultant;

   SCIPintervalSign(&resultant, x);

   return resultant;
}

/** macro to easy definition of so far unimplemented interval functions */
#define SCIP_INTERVALARITH_UNDEFFUNC(function)                                  \
inline                                                                          \
SCIPInterval function(                                                          \
   const SCIPInterval&   x                   /**< operand */                    \
   )                                                                            \
{                                                                               \
   SCIPwarningMessage("Error: " #function " not implemented for intervals.\n"); \
   return SCIPInterval();                                                       \
}

SCIP_INTERVALARITH_UNDEFFUNC(acos)
SCIP_INTERVALARITH_UNDEFFUNC(asin)
SCIP_INTERVALARITH_UNDEFFUNC(atan)
SCIP_INTERVALARITH_UNDEFFUNC(cosh)
SCIP_INTERVALARITH_UNDEFFUNC(sinh)
SCIP_INTERVALARITH_UNDEFFUNC(erf)
#undef SCIP_INTERVALARITH_UNDEFFUNC

#endif
