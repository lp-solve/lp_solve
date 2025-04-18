/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the class library                   */
/*       SoPlex --- the Sequential object-oriented simPlex.                  */
/*                                                                           */
/*    Copyright (C) 1996      Roland Wunderling                              */
/*                  1996-2010 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SoPlex is distributed under the terms of the ZIB Academic Licence.       */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License              */
/*  along with SoPlex; see the file COPYING. If not email to soplex@zib.de.  */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#pragma ident "@(#) $Id: datahashtable.h,v 1.29 2010/10/01 19:30:47 bzfwinkm Exp $"

/**@file  datahashtable.h
 * @brief Generic hash table for data objects.
 */
#ifndef _DATAHASHTABLE_H_
#define _DATAHASHTABLE_H_

#include <iostream>
#include <assert.h>

#include "spxdefines.h"
#include "message.h"

namespace soplex
{
/**@brief   Generic hash table for data objects.
   @ingroup Elementary

   Class DataHashTable provides a generic hash table for 
   \ref DataObjects "Data Objects",
   i.e., a map that maps arguments called \a HashItems to values called \a Infos.
   HashItem and Info types are passed as template arguments. HashItems
   must provide a comparison operator==().  Furthermore, both the HashItem and
   Info must be data objects in the sense that the assignment operator is
   equivalent to a <tt>memcpy()</tt> of the structure and no destructor is 
   required.
   
   The construction of a DataHashTable requires a \em hash \em function that
   assigns an integer value to every HashItem.  Provided this, pairs of a
   HashItem and a Info can be added to the DataHashTable. No more
   than one Info can be assigned to the same HashItem at a time. The Info
   to a HashItem can be accessed through the subscript operator[]() with
   the Info object as a subscript.
   
   The maximum number of elemens a DataHashTable can hold can be
   specified upon construction and may be reset with reMax() later on.
   Further, a value hash size value is required. This value must be less then 
   the maximum number of elements and must not have a common dominator with 
   the maximum number of elements. If not specified explicitely, it
   is set automatically to a reasonable value. 

   The implementation relies on an array of DataHashTable::Element%s, from
   now on referred to as elements. Upon construction, all elements are
   marked as \c FREE. When an entry is added
   to the DataHashTable, the hash value is computed by calling #m_hashfun
   for its HashItem. If this array element is unused, it is
   taken right away. Otherwise, the array index is incremented by
   the hash size (modulo the element array size()) until an unused element
   is found.
   
   Removing elements is simply done by marking it as \c RELEASED. Hence,
   when searching for an element, the search loop may not stop, when a
   \c RELEASED element is encountered. However, such an element may be
   reused when adding a new element to the DataHashTable. 
   
   Further, memory management with resizing of the element array is
   straight forward.
*/
template < class HashItem, class Info >
class DataHashTable
{
private:

   //-----------------------------------
   /**@name Types */
   //@{
   /// template class for elements stored in the hash table
   template < class ElemHashItem, class ElemInfo >
   class Element
   {
   public:
      ///
      ElemHashItem    item;
      ///
      ElemInfo        info;
      /// States of an element
      enum states
      {
         FREE,            ///< element has never been used
         RELEASED,        ///< element had been used, but released
         USED             ///< element is in use
      } stat;
   };
   typedef Element< HashItem, Info > Elem;
   //@}

   //-----------------------------------
   /**@name Data */
   //@{
   /// stores all elements of the hash table
   DataArray < Elem > m_elem;   
   /// increment added to hash index, if allready used
   int m_hashsize;        
   /// current number of entries in the hash table
   int m_used;            
   /// pointer to hash function (mapping: \a HashItem -> int)
   int (*m_hashfun) (const HashItem*);  
   /// memory is \ref soplex::DataHashTable::reMax() "reMax()"ed by this factor if a new element does't fit
   Real m_memfactor;  
   //@}

public:

   //-----------------------------------
   /**@name Access / modification */
   //@{
   /// Is item \p h present in DataHashTable?
   bool has(const HashItem& h) const
   {
      return index(h) >= 0;
   }

   /// returns const pointer to \a Info of \a HashItem \p h or 0, 
   /// if item is not found.
   /** Returns a pointer to \a Info component of hash element \p h or a zero
    *  pointer if element \p h is not in the table.
    */
   const Info* get(const HashItem& h) const
   {
      int i = index(h);

      return (i >= 0) ? &m_elem[i].info : 0;
   }
   /// references \a Info of \a HashItem \p h.
   /** Index operator for accessing the \a Info associated to
    *  \a HashItem \p h. It is required that \p h belongs to the
    *  DataHashTable, otherwise it core dumps. Methods has() or
    *  get() can be used for inquiring wheater \p h belongs to the
    *  DataHashTable or not.
    */
   const Info& operator[](const HashItem& h) const
   {
      assert(has(h));

      return m_elem[index(h)].info;
   }
   /// adds a new entry to the hash table.
   /** Adds a new entry consisting of \a HashItem \p h and \a Info \p info to the
    *  DataHashTable. No entry with HashItem \p h must be in the
    *  DataHashTable yet. After completion, \p info may be accessed via get() or
    *  operator[]() with \p h as parameter. The DataHashTable is #reMax()%ed
    *  if it becomes neccessary.
    */
   void add(const HashItem& h, const Info& info)
   {
      assert(!has(h));

      if (m_used >= m_elem.size())
         reMax(int(m_memfactor * m_used) + 1);

      assert(m_used < m_elem.size());

      int i;

      for(i = (*m_hashfun)(&h) % m_elem.size();
          m_elem[i].stat == Elem::USED;
          i = (i + m_hashsize) % m_elem.size())
         ;

      assert(m_elem[i].stat != Elem::USED);

      m_elem[i].stat = Elem::USED;
      m_elem[i].info = info;
      m_elem[i].item = h;

      m_used++;

      assert(has(h));
   }

   /// remove \a HashItem \p h from the DataHashTable.
   void remove(const HashItem& h)
   {
      assert(has(h));
      m_elem[index(h)].stat = Elem::RELEASED;
      m_used--;
      assert(!has(h));
   }

   /// remove all entries from DataHashTable.
   void clear()
   {
      for(int i = 0; i < m_elem.size(); i++)
         m_elem[i].stat = Elem::FREE;
      m_used = 0;
   }
   /// reset size of the DataHashTable.
   /** Reset the maximum number of elements of a DataHashTable to \p newSize.
    *  However, if \p newSize < #m_used, it is resized to #m_used only. 
    *  If \p newHashSize < 1, a new hash size is computed automatically. 
    *  Otherwise, the specified value will be taken.
    */
   void reMax (int newSize = -1, int newHashSize = 0)
   {
      DataArray< Elem > save(m_elem);

      m_elem.reSize(newSize < m_used ? m_used : newSize);

      clear();

      m_hashsize = (newHashSize < 1) ? autoHashSize() : newHashSize;

      for(int i = 0; i < save.size(); i++)
         if (save[i].stat == Elem::USED)
            add(save[i].item, save[i].info);
   }
   //@}

   //-----------------------------------
   /**@name Debugging */
   //@{
#ifndef NO_CONSISTENCY_CHECKS
   /// checks whether DataHashTable is consistent
   bool isConsistent() const
   {
      int total = 0;

      for(int i = 0; i < m_elem.size(); i++)
      {
         if (m_elem[i].stat == Elem::USED)
         {
            total++;
            if (!has(m_elem[i].item))
               return MSGinconsistent("DataHashTable");
         }
      }
      if (total != m_used)
         return MSGinconsistent("DataHashTable");

      return m_elem.isConsistent();
   }
#endif
   //@}

   //-----------------------------------
   /**@name Construction / destruction */
   //@{
   /// default constructor.
   /** Allocates a DataHashTable for \p maxsize entries using \p hashfun
    *  as hash function. If \p hashsize > 0, #m_hashsize is set to the 
    *  specified value, otherwise a suitable hash size is computed 
    *  automatically. Parameter \p factor is used for memory management: 
    *  If more than \p maxsize entries are added to the DataHashTable, it 
    *  will automatically be #reMax()%ed by a factor of \p factor.
    *
    *  @param hashfun      pointer to hash function.
    *  @param maxsize      maximum number of hash elements.
    *  @param hashsize     hash size.
    *  @param factor       factor for increasing data block.
    */
   explicit DataHashTable(
      int (*hashfun)(const HashItem*), 
      int maxsize  = 256, 
      int hashsize = 0, 
      Real factor  = 2.0)
      : m_elem(maxsize)
      , m_hashfun(hashfun)
      , m_memfactor(factor)
   {
      clear();

      m_hashsize = (hashsize < 1) ? autoHashSize() : hashsize;

      assert(m_memfactor > 1.0);
      assert(isConsistent());
   }

   /// assignment operator.
   DataHashTable& operator=(const DataHashTable& base)
   {
      m_elem = base.m_elem;
      m_hashfun = base.hashfun;
      m_memfactor = base.m_memfactor;
      m_used = base.m_used;
      m_hashsize = base.m_hashsize;

      assert(m_memfactor > 1.0);
      assert(isConsistent());
   }

   /// copy constructor.
   DataHashTable(const DataHashTable& base)
      : m_elem(base.m_elem)
      , m_hashfun(base.m_hashfun)
      , m_memfactor(base.m_memfactor)
      , m_used(base.m_used)
      , m_hashsize(base.hashsize)
   {
      assert(m_memfactor > 1.0);
      assert(isConsistent());
   }
   //@}

private:

   //-----------------------------------
   /**@name Helpers */
   //@{
   /// automatically computes a good \ref soplex::DataHashTable::m_hashsize "m_hashsize".
   /** Computes a good #m_hashsize as the product of all prime numbers 
    *  not divisors of the number of elements that are <= 
    *  the maximum divisor of the number of elemens.
    *  @return good value for #m_hashsize
    */
   int autoHashSize() const
   {
      DataArray< bool > prime(m_elem.size());
      int hashsize = 1;
      int maxsize  = m_elem.size();
      int i;

      for (i = 2; i < maxsize; i++)
         prime[i] = true;

      for (i = 2; i < maxsize; ++i)
      {
         if (prime[i])
         {
            for (int j = i; j < maxsize; j += i)
               prime[j] = false;

            if (m_elem.size() % i != 0)
            {
               hashsize *= i;

               if (hashsize > maxsize)
               {
                  hashsize /= i;
                  break;
               }
            }
         }
      }
      return hashsize;
   }

   /// returns hash index of \a HashItem \p h or -1, if \p h is not present.
   /** Using the hash function #m_hashfun, the hash value of \p h 
    *  is calculated.
    *  Starting with this hash index, every #m_hashsize%-th element is
    *  compared with \p h until \p h is found or all elements have been checked.
    *
    *  @param  h  \a HashItem, for which the hash index should be calculated
    *  @return hash index of \p h or -1, 
    *          if \p h is not a member of the hash table
    */
   int index(const HashItem& h) const
   {
      if (m_used == 0)
         return -1;

      assert(m_elem.size() > 0);

      int i = (*m_hashfun)(&h) % m_elem.size();
      int j = i;
      
      while(m_elem[i].stat != Elem::FREE)
      {
         if (  (m_elem[i].stat == Elem::USED)
            && (m_elem[i].item == h))
            return i;

         i = (i + m_hashsize) % m_elem.size();

         if (i == j)
            break;
      }
      return -1;
   }
   //@}

};
} // namespace soplex
#endif // _DATAHASHTABLE_H_

//-----------------------------------------------------------------------------
//Emacs Local Variables:
//Emacs mode:c++
//Emacs c-basic-offset:3
//Emacs tab-width:8
//Emacs indent-tabs-mode:nil
//Emacs End:
//-----------------------------------------------------------------------------





