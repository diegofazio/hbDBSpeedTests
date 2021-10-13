/*  $Id$  */

/*
 * Harbour Project source code:
 * Harbour Leto singly-linked lists functions
 *
 * Copyright 2012 Pavel Tsarenko <tpe2 / at / mail.ru>
 * www - http://www.harbour-project.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA (or visit the web site http://www.gnu.org/).
 *
 * As a special exception, the Harbour Project gives permission for
 * additional uses of the text contained in its release of Harbour.
 *
 * The exception is that, if you link the Harbour libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the Harbour library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by the Harbour
 * Project under the name Harbour.  If you copy code from other
 * Harbour Project or Free Software Foundation releases into a copy of
 * Harbour, as the General Public License permits, the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for Harbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
 */

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbthread.h"
#include "srvleto.h"

void letoListInit( PLETO_LIST pList, ULONG ulSize )
{
   pList->ulSize = ulSize;
   pList->pItem  = NULL;
   pList->pMutex = hb_threadMutexCreate();
}

void letoClearList( PLETO_LIST pList )
{
   PLETO_LIST_ITEM pItem, pLast;

   hb_threadMutexLock( pList->pMutex );
   pItem = pList->pItem;
   while( pItem )
   {
      pLast = pItem;
      pItem = pItem->pNext;
      hb_xfree( pLast );
   }
   pList->pItem = NULL;
   hb_threadMutexUnlock( pList->pMutex );
}

void letoListFree( PLETO_LIST pList )
{
   letoClearList( pList );
   if( pList->pMutex )
      hb_itemRelease( pList->pMutex );
   pList->pMutex = NULL;
}

void * letoAddToList( PLETO_LIST pList )
{
   PLETO_LIST_ITEM pItem;

   hb_threadMutexLock( pList->pMutex );
   pItem = hb_xgrab( pList->ulSize + sizeof( PLETO_LIST_ITEM ) );
   pItem->pNext = NULL;
   memset( pItem + 1, 0, pList->ulSize );
   if( pList->pItem )
   {
      PLETO_LIST_ITEM pLast = pList->pItem;
      while( pLast->pNext) pLast = pLast->pNext;
      pLast->pNext = pItem;
   }
   else
   {
      pList->pItem = pItem;
   }
   hb_threadMutexUnlock( pList->pMutex );
   return pItem + 1;
}

void * letoGetListItem( PLETO_LIST pList, USHORT uiNum )
{
   if( pList->pItem )
   {
      PLETO_LIST_ITEM pItem = pList->pItem;
      while( uiNum && pItem )
      {
         pItem = pItem->pNext;
         uiNum --;
      }
      return pItem ? pItem + 1 : NULL;
   }
   else
      return NULL;
}

BOOL letoDelFromList( PLETO_LIST pList, USHORT uiNum )
{
   BOOL bRet = FALSE;

   if( pList->pItem )
   {
      PLETO_LIST_ITEM pItem = pList->pItem;
      hb_threadMutexLock( pList->pMutex );
      if( uiNum )
      {
         while( uiNum > 1 && pItem )
         {
            pItem = pItem->pNext;
            uiNum --;
         }
         if( pItem )
         {
            PLETO_LIST_ITEM pLast = pItem;
            pItem = pLast->pNext;
            pLast->pNext = ( pItem ) ? pItem->pNext : NULL;
         }
      }
      else
      {
         pList->pItem = pItem->pNext;
      }
      if( pItem )
      {
         hb_xfree( pItem );
         bRet = TRUE;
      }
      hb_threadMutexUnlock( pList->pMutex );
   }
   return bRet;
}

BOOL letoIsRecInList( PLETO_LIST pList, ULONG ulRecNo )
{
   PLETO_LOCK_ITEM pItem;

   pItem = ( PLETO_LOCK_ITEM ) pList->pItem;

   while( pItem && pItem->ulRecNo <= ulRecNo )
      if( pItem->ulRecNo == ulRecNo )
         return TRUE;
      else
         pItem = pItem->pNext;
   return FALSE;
}

void letoAddRecToList( PLETO_LIST pList, ULONG ulRecNo )
{
   if( pList->pItem )
   {
      PLETO_LOCK_ITEM pItem, pLast = NULL;
      BOOL bFound = FALSE;

      hb_threadMutexLock( pList->pMutex );
      pItem = ( PLETO_LOCK_ITEM ) pList->pItem;
      while( pItem && pItem->ulRecNo <= ulRecNo )
         if( pItem->ulRecNo == ulRecNo )
         {
            bFound = TRUE;
            break;
         }
         else
         {
            pLast = pItem;
            pItem = pItem->pNext;
         }
      if( ! bFound )
      {
         PLETO_LOCK_ITEM pNew = hb_xgrab( sizeof( LETO_LOCK_ITEM ) );
         pNew->ulRecNo = ulRecNo;
         if( pLast )
         {
            pNew->pNext = pLast->pNext;
            pLast->pNext = pNew;
         }
         else
         {
            pNew->pNext = ( PLETO_LOCK_ITEM ) pList->pItem;
            pList->pItem = ( PLETO_LIST_ITEM ) pNew;
         }
      }
      hb_threadMutexUnlock( pList->pMutex );
   }
   else
   {
      hb_threadMutexLock( pList->pMutex );
      pList->pItem = hb_xgrab( sizeof( LETO_LOCK_ITEM ) );
      ( ( PLETO_LOCK_ITEM ) pList->pItem )->ulRecNo = ulRecNo;
      pList->pItem->pNext = NULL;
      hb_threadMutexUnlock( pList->pMutex );
   }
}

BOOL letoDelRecFromList( PLETO_LIST pList, ULONG ulRecNo )
{
   BOOL bRet = FALSE;

   if( pList->pItem )
   {
      PLETO_LOCK_ITEM pItem, pLast = NULL;

      hb_threadMutexLock( pList->pMutex );
      pItem = ( PLETO_LOCK_ITEM ) pList->pItem;
      while( pItem && pItem->ulRecNo <= ulRecNo )
         if( pItem->ulRecNo == ulRecNo )
         {
            if( pLast )
               pLast->pNext = pItem->pNext;
            else
               pList->pItem = ( PLETO_LIST_ITEM ) pItem->pNext;
            hb_xfree( pItem );
            bRet = TRUE;
            break;
         }      
         else
         {
            pLast = pItem;
            pItem = pItem->pNext;
         }
      hb_threadMutexUnlock( pList->pMutex );
   }
   return bRet;
}
