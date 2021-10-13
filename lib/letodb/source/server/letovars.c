/*  $Id$ */

/*
 * Harbour Project source code:
 * Leto db server functions
 *
 * Copyright 2010 Alexander S. Kresin <alex / at / belacy.belgorod.su>
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
#include "hbapifs.h"
#include "hbapicls.h"
#include "hbapierr.h"
#include "math.h"
#include "srvleto.h"

#define VARGROUPS_ALLOC     10
#define VARS_ALLOC          10

#define LETOVAR_LOG '1'
#define LETOVAR_NUM '2'
#define LETOVAR_STR '3'

struct leto_struLogical
{
   BOOL value;
};

struct leto_struLong
{
   long int value;
};

struct leto_struString
{
   ULONG length;
   ULONG allocated;
   char * value;
};

typedef struct
{
   char *      szName;
   USHORT   uiNameLen;
   USHORT      uiUser;
   char          type;
   char         cFlag;
   union
   {
      struct leto_struLogical   asLogical;
      struct leto_struLong      asLong;
      struct leto_struString    asString;
   } item;
} LETO_VAR;

typedef struct
{
   char *      szName;
   USHORT   uiNameLen;
   USHORT     uiItems;
   USHORT     uiAlloc;
   LETO_VAR *  pItems;
} LETO_VARGROUPS;

static LETO_VARGROUPS * pVarGroups = NULL;
static USHORT uiVarGroupsAlloc = 0;
static USHORT uiVarGroupsCurr = 0;
static ULONG  ulVarsCurr = 0;

extern const char * szOk;
extern const char * szErr2;
extern const char * szErr3;
extern const char * szErr4;
extern const char * szErr101;
extern const char * szErrAcc;

extern ULONG  ulVarsMax;
extern USHORT uiVarLenMax;
extern USHORT uiVarsOwnMax;

extern PUSERSTRU s_users;

extern int leto_GetParam( const char *szData, const char **pp2, const char **pp3, const char **pp4, const char **pp5 );
extern void leto_SendAnswer( PUSERSTRU pUStru, const char* szData, ULONG ulLen );
extern void leto_wUsLog( PUSERSTRU pUStru, const char* s, int n );

static LETO_VAR * leto_var_create( PUSERSTRU pUStru, LETO_VARGROUPS * pGroup, const char * pp1, const char * pp2, char cFlag )
{
   LETO_VAR * pItem;
   USHORT uiVarGroupLen, uiVarLen;

   if( ( cFlag & LETO_VOWN ) && ( uiVarsOwnMax <= pUStru->uiVarsOwnCurr ) )
      return NULL;

   if( !pGroup )
   {
      if( !pVarGroups )
      {
         uiVarGroupsAlloc = VARGROUPS_ALLOC;
         pVarGroups = (LETO_VARGROUPS*) hb_xgrab( sizeof(LETO_VARGROUPS) * uiVarGroupsAlloc );
         memset( pVarGroups, 0, sizeof(LETO_VARGROUPS) * uiVarGroupsAlloc );
      }
      else if( uiVarGroupsAlloc == uiVarGroupsCurr )
      {
         pVarGroups = (LETO_VARGROUPS*) hb_xrealloc( pVarGroups, sizeof(LETO_VARGROUPS) * ( uiVarGroupsAlloc+VARGROUPS_ALLOC ) );
         memset( pVarGroups+uiVarGroupsAlloc, 0, sizeof(LETO_VARGROUPS) * VARGROUPS_ALLOC );
         uiVarGroupsAlloc += VARGROUPS_ALLOC;
      }
      // pGroup = pVarGroups + uiVarGroupsCurr;
      pGroup = pVarGroups;
      while( ( pGroup - pVarGroups ) < uiVarGroupsAlloc && pGroup->szName )
         pGroup ++;

      if( ( pGroup - pVarGroups ) < uiVarGroupsAlloc )
      {
         uiVarGroupsCurr ++;

         uiVarGroupLen = strlen( pp1 );
         pGroup->szName = (char*) hb_xgrab( uiVarGroupLen + 1 );
         memcpy( pGroup->szName, pp1, uiVarGroupLen );
         pGroup->szName[uiVarGroupLen] = '\0';
         pGroup->uiNameLen = uiVarGroupLen;
         pGroup->uiAlloc = VARS_ALLOC;
         pGroup->pItems = (LETO_VAR*) hb_xgrab( sizeof(LETO_VAR) * pGroup->uiAlloc );
         memset( pGroup->pItems, 0, sizeof(LETO_VAR) * pGroup->uiAlloc );
         ulVarsCurr += VARS_ALLOC;
      }
      else
         return NULL;
   }
   if( pGroup->uiAlloc == pGroup->uiItems )
   {
      pGroup->pItems = (LETO_VAR*) hb_xrealloc( pGroup->pItems, sizeof(LETO_VAR) * ( pGroup->uiAlloc+VARS_ALLOC ) );
      memset( pGroup->pItems+pGroup->uiAlloc, 0, sizeof(LETO_VAR) * VARS_ALLOC );
      pGroup->uiAlloc += VARS_ALLOC;
      ulVarsCurr += VARS_ALLOC;
   }
   // pItem = pGroup->pItems + pGroup->uiItems;
   pItem = pGroup->pItems;
   while( ( pItem - pGroup->pItems) < pGroup->uiAlloc && pItem->szName )
      pItem ++;

   if( ( pItem - pGroup->pItems) < pGroup->uiAlloc )
   {
      pGroup->uiItems ++;

      memset( pItem, 0, sizeof(LETO_VAR) );
      uiVarLen = strlen( pp2 );
      pItem->szName = (char*) hb_xgrab( uiVarLen + 1 );
      memcpy( pItem->szName, pp2, uiVarLen );
      pItem->szName[uiVarLen] = '\0';
      pItem->uiNameLen = uiVarLen;
      pItem->cFlag = cFlag;
      if( pItem->cFlag & LETO_VOWN )
      {
         pItem->uiUser = pUStru - s_users + 1;
         if( !pUStru->pVarLink )
         {
            pUStru->pVarLink = (VAR_LINK*) malloc( sizeof(VAR_LINK)*uiVarsOwnMax );
            memset( pUStru->pVarLink, 0, sizeof(VAR_LINK)*uiVarsOwnMax );
         }
         (pUStru->pVarLink + pUStru->uiVarsOwnCurr)->uiGroup = ( pGroup - pVarGroups ) + 1;
         (pUStru->pVarLink + pUStru->uiVarsOwnCurr)->uiVar = ( pItem - pGroup->pItems ) + 1;
         pUStru->uiVarsOwnCurr ++;
      }
      return pItem;
   }
   else
      return NULL;
}

static long int leto_var_len( LETO_VAR * pItem )
{
   if( pItem )
   {
      switch( pItem->type )
      {
         case LETOVAR_LOG:
            return 1;
         case LETOVAR_NUM:
         {
            char szLong[64];
            hb_snprintf( szLong, 64, "%ld", pItem->item.asLong.value );
            return strlen( szLong );
         }
         case LETOVAR_STR:
            return pItem->item.asString.length;
      }
   }
   return -1;
}

static char * leto_var_get( LETO_VAR * pItem, ULONG * ulLen, char * pData )
{
   char * ptr;
   long int lVarLen = leto_var_len( pItem );

   if( lVarLen >= 0 )
   {
      if( !pData )
         pData = ( char * ) malloc( 4 + lVarLen );
      lVarLen ++;  // '\0'
      ptr = pData;
      if( *ulLen > 0 && *ulLen < (ULONG)lVarLen )
         lVarLen = (long int) *ulLen;
      switch( pItem->type )
      {
         case LETOVAR_LOG:
         {
            *(ptr+3) = (pItem->item.asLogical.value)? '1' : '0';
            break;
         }
         case LETOVAR_NUM:
         {
            hb_snprintf( ptr+3, lVarLen, "%ld", pItem->item.asLong.value );
            break;
         }
         case LETOVAR_STR:
         {
            memcpy( ptr+3, pItem->item.asString.value, lVarLen );
            break;
         }
      }
      lVarLen --;  // '\0'
      *ptr++ = '+'; *ptr++ = pItem->type; *ptr++ = ';';
      *( ptr + lVarLen ) = ';';
      *ulLen = (ULONG)(4 + lVarLen);
   }
   return pData;
}

static void leto_var_del( PUSERSTRU pUStru, LETO_VARGROUPS * pGroup, USHORT uiItem )
{
   LETO_VAR * pItem = pGroup->pItems + uiItem;

   if( pItem->szName )
   {
      hb_xfree( pItem->szName );
      if( pItem->type == LETOVAR_STR && pItem->item.asString.value )
      {
         hb_xfree( pItem->item.asString.value );
      }
      if( pUStru && pUStru->pVarLink && (pItem->cFlag & LETO_VOWN) )
      {
         USHORT uiGroup = ( pGroup - pVarGroups ) + 1;
         USHORT ui, uiItem = ( pItem - pGroup->pItems ) + 1;
         VAR_LINK * pVLink = pUStru->pVarLink;
         for( ui=0; ui<pUStru->uiVarsOwnCurr; ui++,pVLink++ )
            if( pVLink->uiGroup == uiGroup && pVLink->uiVar == uiItem )
            {
               memmove( pVLink, pVLink+1, sizeof(VAR_LINK)*(pUStru->uiVarsOwnCurr-ui-1) );
               pUStru->uiVarsOwnCurr --;
               break;
            }
      }
      memset( pItem, 0, sizeof(LETO_VAR) );
      /*
      for( uiItem++; uiItem < pGroup->uiItems; uiItem++ )
      {
         memcpy( pGroup->pItems+uiItem-1, pGroup->pItems+uiItem, sizeof(LETO_VAR) );
      }
      */
      pGroup->uiItems --;
   }
}

static void leto_var_delgroup( PUSERSTRU pUStru, LETO_VARGROUPS * pGroup )
{
   USHORT uiItem;

   ulVarsCurr -= pGroup->uiAlloc;
   hb_xfree( pGroup->szName );
   uiItem = 0;
   while( uiItem < pGroup->uiAlloc )
   {
      leto_var_del( pUStru, pGroup, uiItem );
      uiItem ++;
   }
   hb_xfree( pGroup->pItems );
   memset( pGroup, 0, sizeof(LETO_VARGROUPS) );
   /*
   for( uiGroup++; uiGroup < uiVarGroupsCurr; uiGroup++ )
   {
      memcpy( pVarGroups+uiGroup-1, pVarGroups+uiGroup, sizeof(LETO_VARGROUPS) );
   }
   */
   uiVarGroupsCurr --;
}

static LETO_VAR * leto_var_find( const char * pVarGroup, const char * pVar, LETO_VARGROUPS ** ppGroup, USHORT *puiItem )
{
   LETO_VARGROUPS * pGroup;
   LETO_VAR * pItem = NULL;
   USHORT uiGroup, uiItem, ui;
   USHORT uiVarGroupLen, uiVarLen=0;

   uiVarGroupLen = strlen( pVarGroup );

   for( uiGroup=0; uiGroup<uiVarGroupsAlloc; uiGroup++ )
      if( (pVarGroups+uiGroup)->uiNameLen == uiVarGroupLen && 
            !strcmp( pVarGroup, (pVarGroups+uiGroup)->szName ) )
      {
         *ppGroup = pGroup = pVarGroups+uiGroup;
         if( *pVar )
         {
            if( ! uiVarLen )
               uiVarLen = strlen( pVar );
            for( uiItem=0,ui=0; uiItem<pGroup->uiAlloc; uiItem++ )
            {
               if( (pGroup->pItems+uiItem)->szName )
               {
                  if( (pGroup->pItems+uiItem)->uiNameLen == uiVarLen &&
                        !strcmp( pVar, (pGroup->pItems+uiItem)->szName ) )
                  {
                     pItem = pGroup->pItems+uiItem;
                     break;
                  }
                  if( ++ui > pGroup->uiItems )
                     break;
               }
            }
            *puiItem = uiItem;
         }
         break;
      }
   return pItem;
}

static void leto_var_set_str( LETO_VAR * pItem, const char* pStr )
{
   ULONG ulLen = strlen(pStr);

   if( pItem->item.asString.allocated && pItem->item.asString.allocated <= ulLen )
   {
      hb_xfree( pItem->item.asString.value );
      pItem->item.asString.allocated = 0;
   }
   if( !pItem->item.asString.allocated )
   {
      pItem->item.asString.value = (char*) hb_xgrab( ulLen + 1 );
      pItem->item.asString.allocated = ulLen + 1;
   }
   pItem->item.asString.length = ulLen;
   memcpy( pItem->item.asString.value, pStr, ulLen );
   pItem->item.asString.value[ulLen] = '\0';
}

static BOOL leto_var_accessdeny( PUSERSTRU pUStru, LETO_VAR * pItem, char cFlag )
{
   return ( pItem->cFlag & LETO_VOWN ) &&
          ( pItem->cFlag & cFlag ) &&
          ( pItem->uiUser != (pUStru - s_users + 1) );
}

void leto_Variables( PUSERSTRU pUStru, char* szData )
{
   char * pData = NULL;
   char * pVarGroup, * pVar, * pp3, * ptr;
   char cFlag1, cFlag2;
   int nParam = leto_GetParam( szData, (const char**) &pVarGroup, (const char**) &pVar, (const char**) &pp3, (const char**) &ptr );
   USHORT uiGroup, uiItem = 0, uiLen, ui;
   ULONG ulLen;
   LETO_VARGROUPS * pGroup = NULL;
   LETO_VAR * pItem = NULL;
   long int lValue;

   if( nParam < 3 )
      leto_SendAnswer( pUStru, szErr2, 4 );
   else
   {
      if( pVarGroups && *pVarGroup )
      {
         pItem = leto_var_find( pVarGroup, pVar, &pGroup, &uiItem );
      }
      if( !strcmp( szData,"set" ) )
      {
         cFlag1 = *(ptr+1); cFlag2 = *(ptr+2);
         if( nParam < 5 || !*pVarGroup || !*pVar || *ptr < '1' || cFlag1 < ' ' || cFlag2 < ' ' )
            leto_SendAnswer( pUStru, szErr2, 4 );
         else if( !pItem && ( !(cFlag1 & LETO_VCREAT) || ulVarsCurr >= ulVarsMax ) )
            leto_SendAnswer( pUStru, szErr3, 4 );
         else
         {
            if( !pItem )
               pItem = leto_var_create( pUStru, pGroup, pVarGroup, pVar, cFlag1 );
            if( !pItem )
               leto_SendAnswer( pUStru, szErr3, 4 );
            else if( pItem->type >= '1' && pItem->type != *ptr )
               leto_SendAnswer( pUStru, szErr4, 4 );
            else if( leto_var_accessdeny(pUStru, pItem, LETO_VDENYWR + LETO_VCREAT ) )
               leto_SendAnswer( pUStru, szErrAcc, 4 );
            else
            {
               if( *ptr == LETOVAR_STR && ((USHORT)(strlen(pp3))) > uiVarLenMax )
                  leto_SendAnswer( pUStru, szErr3, 4 );
               else if( *ptr > LETOVAR_STR )
                  leto_SendAnswer( pUStru, szErr2, 4 );
               else
               {
                  ULONG ulRetLen = 0;

                  if( cFlag2 & LETO_VPREVIOUS )
                     pData = leto_var_get( pItem, &ulRetLen, NULL );
                  if( *ptr == LETOVAR_LOG )
                  {
                     pItem->item.asLogical.value = ( *pp3 != '0' );
                  }
                  else if( *ptr == LETOVAR_NUM )
                  {
                     sscanf( pp3, "%ld", &lValue );
                     pItem->item.asLong.value = lValue;
                  }
                  else if( *ptr == LETOVAR_STR )
                  {
                     leto_var_set_str( pItem, pp3 );
                  }
                  pItem->type = *ptr;
                  if( ( cFlag2 & LETO_VPREVIOUS ) && pData )
                     leto_SendAnswer( pUStru, pData, ulRetLen );
                  else
                     leto_SendAnswer( pUStru, szOk, 4 );
               }
            }
         }
      }
      else if( !strcmp( szData,"get" ) )
      {
         ulLen = 0;
         if( !pItem )
            leto_SendAnswer( pUStru, szErr3, 4 );
         else if( leto_var_accessdeny(pUStru, pItem, LETO_VDENYRD ) )
            leto_SendAnswer( pUStru, szErrAcc, 4 );
         else if( ( pData = leto_var_get( pItem, &ulLen, NULL ) ) == NULL )
            leto_SendAnswer( pUStru, szErr4, 4 );
         else
            leto_SendAnswer( pUStru, pData, ulLen );
      }
      else if( !strcmp( szData,"inc" ) || !strcmp( szData,"dec" ) )
      {
         BOOL bInc = ( *szData == 'i' );
         cFlag1 = *(pp3+1); cFlag2 = *(pp3+2);
         if( nParam < 4 || !*pVarGroup || !*pVar || *pp3 != '2' || cFlag1 < ' ' || cFlag2 < ' ' )
            leto_SendAnswer( pUStru, szErr2, 4 );
         else if( !pItem && ( !(cFlag1 & LETO_VCREAT) || ulVarsCurr >= ulVarsMax ) )
            leto_SendAnswer( pUStru, szErr3, 4 );
         else
         {
            if( !pItem )
               pItem = leto_var_create( pUStru, pGroup, pVarGroup, pVar, cFlag1 );
            if( !pItem )
               leto_SendAnswer( pUStru, szErr3, 4 );
            else if( pItem->type != LETOVAR_NUM )
               leto_SendAnswer( pUStru, szErr4, 4 );
            else if( leto_var_accessdeny(pUStru, pItem, LETO_VDENYWR ) )
               leto_SendAnswer( pUStru, szErrAcc, 4 );
            else
            {
               ulLen = 0;
               if( ( cFlag2 & LETO_VPREVIOUS ) && ( pData = leto_var_get( pItem, &ulLen, NULL ) ) != NULL )
                  leto_SendAnswer( pUStru, pData, ulLen );
               else
                  leto_SendAnswer( pUStru, szOk, 4 );
               if( bInc )
                  pItem->item.asLong.value ++;
               else
                  pItem->item.asLong.value --;
            }
         }
      }
      else if( !strcmp( szData,"del" ) )
      {
         if( !pItem )
         {
            if( *pVar )
               leto_SendAnswer( pUStru, szErr3, 4 );
            else  {
               if( pGroup )
                  leto_var_delgroup( pUStru, pGroup );
               leto_SendAnswer( pUStru, szOk, 4 );
            }
         }
         else
         {
            leto_var_del( pUStru, pGroup, uiItem );
            leto_SendAnswer( pUStru, szOk, 4 );
         }
      }
      else if( !strcmp( szData,"list" ) )
      {
         ulLen = 0;
         if( *pVarGroup )
         {
            if( pGroup )
            {
               // vars in group

               ULONG ulVarLen;
               unsigned int uiMaxLen = 0;

               if( *pp3 && strlen(pp3) < 4 )
                  sscanf( pp3, "%u", &uiMaxLen );
               for( uiItem=0,ui=0; uiItem<pGroup->uiAlloc; uiItem++ )
               {
                  if( (pGroup->pItems+uiItem)->szName )
                  {
                     ulLen += strlen( (pGroup->pItems+uiItem)->szName ) + 1 + 4 + uiMaxLen;
                     if( ++ui > pGroup->uiItems )
                        break;
                  }
               }
               ptr = pData = ( char * ) malloc( ulLen + 8 );
               *ptr++ = '+';
               for( uiItem=0,ui=0; uiItem<pGroup->uiAlloc; uiItem++ )
               {
                  pItem = pGroup->pItems+uiItem;
                  if( pItem->szName )
                  {
                     uiLen = strlen( pItem->szName );
                     memcpy( ptr, pItem->szName, uiLen );
                     ptr += uiLen;
                     if( uiMaxLen )
                     {
                        if( leto_var_accessdeny(pUStru, pItem, LETO_VDENYRD ) )
                        {
                           *ptr++ = ';'; *ptr++ = 'C'; *ptr++ = ';'; *ptr++ = ';';
                        }
                        else
                        {
                           ulVarLen = uiMaxLen;
                           leto_var_get( pItem, &ulVarLen, ptr );
                           *ptr = ';';
                           ptr += ulVarLen;
                        }
                     }
                     else
                        *ptr++ = ';';
                     if( ++ui > pGroup->uiItems )
                        break;
                  }
               }
               *ptr++ = '\r'; *ptr++ = '\n'; *ptr = '\0';
               leto_SendAnswer( pUStru, pData, ptr - pData );
            }
            else
               leto_SendAnswer( pUStru, szErr2, 4 );
         }
         else
         {
            // all vars (all group)
            for( uiGroup=0; uiGroup<uiVarGroupsAlloc; uiGroup++ )
            {
               if( (pVarGroups+uiGroup)->szName )
                  ulLen += strlen( (pVarGroups+uiGroup)->szName ) + 1;
            }
            ptr = pData = ( char * ) malloc( ulLen + 8 );
            *ptr++ = '+';
            for( uiGroup=0; uiGroup<uiVarGroupsCurr; uiGroup++ )
            {
               if( (pVarGroups+uiGroup)->szName )
               {
                  uiLen = strlen( (pVarGroups+uiGroup)->szName );
                  memcpy( ptr, (pVarGroups+uiGroup)->szName, uiLen );
                  ptr += uiLen;
                  *ptr++ = ';';
               }
            }
            leto_SendAnswer( pUStru, pData, ulLen+1 );
         }
      }
      else
         leto_SendAnswer( pUStru, szErr2, 4 );
   }

   if( pData )
      free( pData );
   pUStru->bAnswerSent = 1;
}

void leto_varsown_release( PUSERSTRU pUStru )
{
   if( pUStru->pVarLink )
   {
      if( pUStru->uiVarsOwnCurr )
      {
         VAR_LINK * pVLink = pUStru->pVarLink;
         USHORT ui;
         for( ui=0; ui<pUStru->uiVarsOwnCurr; ui++,pVLink++ )
         {
            if( pVLink->uiGroup && pVLink->uiVar )
            {
               leto_var_del( NULL, pVarGroups + pVLink->uiGroup - 1, pVLink->uiVar - 1 );
            }
         }
      }
      free( pUStru->pVarLink );
      pUStru->pVarLink = NULL;
   }

}

void leto_vars_release( void )
{
   LETO_VARGROUPS * pGroup = pVarGroups;
   USHORT ui = 0, ui1;

   if( pVarGroups )
   {
      while( ui < uiVarGroupsAlloc )
      {
         if( pGroup->szName )
         {
            hb_xfree( pGroup->szName );
            ui1 = 0;
            while( ui1 < pGroup->uiAlloc )
            {
               leto_var_del( NULL, pGroup, ui1 );
               ui1 ++;
            }
            hb_xfree( pGroup->pItems );
         }
         ui ++;
         pGroup ++;
      }
      hb_xfree( pVarGroups );
   }

}

static PHB_ITEM leto_var_ret( LETO_VAR * pItem )
{
   PHB_ITEM pReturn = hb_itemNew( NULL );

   switch( pItem->type )
   {
      case LETOVAR_LOG:
      {
         hb_itemPutL( pReturn, pItem->item.asLogical.value );
         break;
      }
      case LETOVAR_NUM:
      {
         hb_itemPutNL( pReturn, pItem->item.asLong.value );
         break;
      }
      case LETOVAR_STR:
      {
         hb_itemPutCL( pReturn, pItem->item.asString.value, pItem->item.asString.length );
         break;
      }
  }
  return pReturn;
}

HB_FUNC( LETO_VARGET )
{
   PUSERSTRU pUStru = pUStru = s_users + hb_parni( 1 );
   const char * pVarGroup = hb_parc( 2 );
   const char * pVar = hb_parc( 3 );
   LETO_VAR * pItem = NULL;

   if( pVarGroup && pVar )
   {
      LETO_VARGROUPS * pGroup;
      USHORT uiItem;
      pItem = leto_var_find( pVarGroup, pVar, &pGroup, &uiItem );
      HB_SYMBOL_UNUSED( pGroup );
   }

   if( pItem && ! leto_var_accessdeny(pUStru, pItem, LETO_VDENYRD ) )
   {
      hb_itemReturnRelease( leto_var_ret( pItem ) );
   }
   else
      hb_ret();
}

HB_FUNC( LETO_VARSET )
{
   PUSERSTRU pUStru = pUStru = s_users + hb_parni( 1 );
   const char * pVarGroup = hb_parc( 2 );
   const char * pVar = hb_parc( 3 );
   LETO_VAR * pItem = NULL;

   if( pVarGroup && pVar )
   {
      LETO_VARGROUPS * pGroup = NULL;
      USHORT uiItem;
      char cFlag1 = HB_ISNUM(5) ? hb_parni(5): 0;
      pItem = leto_var_find( pVarGroup, pVar, &pGroup, &uiItem );

      if( !pItem )
         pItem = leto_var_create( pUStru, pGroup, pVarGroup, pVar, cFlag1 );
      if( pItem && ! leto_var_accessdeny(pUStru, pItem, LETO_VDENYWR + LETO_VCREAT ) )
      {
         if( HB_ISLOG(4) )
         {
            pItem->item.asLogical.value = hb_parl(4);
            pItem->type = LETOVAR_LOG;
         }
         else if( HB_ISNUM(4) )
         {
            pItem->item.asLong.value = hb_parnl(4);
            pItem->type = LETOVAR_NUM;
         }
         else if( HB_ISCHAR(4) && (USHORT) hb_parclen(4) <= uiVarLenMax )
         {
            leto_var_set_str( pItem, hb_parc(4) );
            pItem->type = LETOVAR_STR;
         }
      }
   }

   if( pItem )
      leto_var_ret( pItem );
   else
      hb_ret();
}

static void leto_var_incdec( BOOL bInc )
{
   PUSERSTRU pUStru = pUStru = s_users + hb_parni( 1 );
   const char * pVarGroup = hb_parc( 2 );
   const char * pVar = hb_parc( 3 );
   LETO_VAR * pItem = NULL;

   if( pVarGroup && pVar )
   {
      LETO_VARGROUPS * pGroup;
      USHORT uiItem;
      char cFlag1 = HB_ISNUM(4) ? hb_parni(4): 0;
      pItem = leto_var_find( pVarGroup, pVar, &pGroup, &uiItem );

      if( !pItem )
         pItem = leto_var_create( pUStru, pGroup, pVarGroup, pVar, cFlag1 );
      if( pItem && pItem->type == LETOVAR_NUM && ! leto_var_accessdeny(pUStru, pItem, LETO_VDENYWR ) )
      {
         if( bInc )
            pItem->item.asLong.value ++;
         else
            pItem->item.asLong.value --;
      }
   }

   if( pItem )
      leto_var_ret( pItem );
   else
      hb_ret();
}

HB_FUNC( LETO_VARINCR )
{
   leto_var_incdec( TRUE );
}

HB_FUNC( LETO_VARDECR )
{
   leto_var_incdec( FALSE );
}

HB_FUNC( LETO_VARDEL )
{
   PUSERSTRU pUStru = pUStru = s_users + hb_parni( 1 );
   const char * pVarGroup = hb_parc( 2 );
   const char * pVar = hb_parc( 3 );
   LETO_VAR * pItem;
   BOOL bRet = FALSE;

   if( pVarGroup )
   {
      LETO_VARGROUPS * pGroup = NULL;
      USHORT uiItem;
      pItem = leto_var_find( pVarGroup, pVar, &pGroup, &uiItem );
      if( pItem )
      {
         leto_var_del( pUStru, pGroup, uiItem );
         bRet = TRUE;
      }
      else if( pGroup )
      {
         leto_var_delgroup( pUStru, pGroup );
         bRet = TRUE;
      }
   }

   hb_retl( bRet );
}

HB_FUNC( LETO_VARGETLIST )
{
   PUSERSTRU pUStru = pUStru = s_users + hb_parni( 1 );
   const char * pVarGroup = hb_parc( 2 );
   BOOL bValue = ( HB_ISLOG(3) ? hb_parl(3) : FALSE );
//   LETO_VAR * pItem;
   USHORT uiItem;
   USHORT uiPos = 0;
   PHB_ITEM pArray = hb_itemNew( NULL );

   hb_arrayNew( pArray, 0 );

   if( pVarGroup )
   {
      LETO_VARGROUPS * pGroup = NULL;
      leto_var_find( pVarGroup, NULL, &pGroup, &uiItem );
      if( pGroup )
      {
         LETO_VAR * pItem;
         PHB_ITEM pSubarray = NULL;

         if( bValue )
            pSubarray = hb_itemNew( NULL );

         for( uiItem=0; uiItem<pGroup->uiAlloc; uiItem++ )
         {
            pItem = pGroup->pItems+uiItem;
            if( pItem->szName && ! leto_var_accessdeny(pUStru, pItem, LETO_VDENYRD ) )
            {
               if( bValue )
               {
                  hb_arrayNew( pSubarray, 2 );

                  hb_arraySetC( pSubarray, 1, pItem->szName );
                  hb_arraySet( pSubarray, 2, leto_var_ret( pItem ) );
                  hb_arrayAddForward( pArray, pSubarray );
               }
               else
               {
                  hb_arraySize( pArray, ++uiPos );
                  hb_arraySetC( pArray, uiPos, pItem->szName );
               }
            }
         }
         if( bValue )
            hb_itemRelease( pSubarray );
      }
   }
   else
   {

      for( uiItem=0; uiItem<uiVarGroupsAlloc; uiItem++ )
      {
         if( (pVarGroups+uiItem)->szName )
         {
            hb_arraySize( pArray, ++uiPos );
            hb_arraySetC( pArray, uiPos, (pVarGroups+uiItem)->szName );
         }
      }

   }

   hb_itemReturnForward( pArray );

}
