/*  $Id$  */

/*
 * Harbour Project source code:
 * Leto db server functions
 *
 * Copyright 2008 Alexander S. Kresin <alex / at / belacy.belgorod.su>
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
#include "hbvm.h"
#include "hbxvm.h"
#include "hbapicls.h"
#include "hbdate.h"
#include "srvleto.h"
#include "hbapierr.h"
#include "hbset.h"

#if defined(HB_OS_UNIX)
  #include <unistd.h>
  #include <sys/time.h>
  #include <sys/timeb.h>
#endif


// init users
#define INIT_USERS_ALLOC   500      // !!! realloc - no MT-safe
#define USERS_REALLOC       50
// init tables (global)
#define INIT_TABLES_ALLOC 5000      // !!! realloc - no MT-safe
#define TABLES_REALLOC     100

#ifndef HB_OS_PATH_DELIM_CHR
   #define HB_OS_PATH_DELIM_CHR OS_PATH_DELIMITER
#endif

#if !defined (SUCCESS)
#define SUCCESS            0
#define FAILURE            1
#endif

#define PDBFAREA( x ) pArea->area.x

#define LETO_MAX_TAGNAME 10
#define LETO_MAX_EXP 256

static DATABASE * s_pDB = NULL;

static AVAILAREAID s_AvailIDS = { NULL, 0, 0, 0 };

//volatile

static unsigned int uiInitTablesMax = INIT_TABLES_ALLOC;
static unsigned int uiInitUsersMax = INIT_USERS_ALLOC;

int iDebugMode = 0;

char szDirBase[_POSIX_PATH_MAX] = "";
char iDirBaseLen            = 0;


PTABLESTRU s_tables = NULL;
USHORT uiTablesAlloc = 0;                // Number of allocated table structures
static USHORT uiTablesMax   = 0;         // Higher index of table structure, which was busy
static USHORT uiTablesCurr  = 0;         // Current number of opened tables

static USHORT uiIndexMax   = 0;         // Higher index of index structure, which was busy
static USHORT uiIndexCurr  = 0;         // Current number of opened indexes

PUSERSTRU s_users = NULL;
static USHORT uiUsersAlloc = 0;         // Number of allocated user structures
USHORT uiUsersMax   = 0;                // Higher index of user structure, which was busy
USHORT uiUsersCurr  = 0;                // Current number of users

ULONG ulOperations = 0;
ULONG ulBytesRead  = 0;
ULONG ulBytesSent  = 0;
static ULONG ulTransAll   = 0;
static ULONG ulTransOK    = 0;

static long   lStartDate;
static double dStartsec;
double dMaxWait    = 0;
double dMaxDayWait = 0;
long   ulToday     = 0;
double dSumWait[6];
unsigned int uiSumWait[6];

const char * szOk   = "++++";
const char * szErr2 = "-002";
const char * szErr3 = "-003";
const char * szErr4 = "-004";
const char * szErr101 = "-101";
const char * szErrAcc = "-ACC";
const char * szNull = "(null)";

const char * szErrUnlocked = "-004:38-1022-0-5";
const char * szErrAppendLock = "-004:40-1024-0-4";

#define ELETO_UNLOCKED 201

static char * pDataPath = NULL;
static USHORT uiDriverDef = 0;
static BOOL   bShareTables = 0;
static BOOL   bNoSaveWA = 0;
static BOOL   bFileFunc = 0;
static BOOL   bAnyExt = 0;
static unsigned int uiUDF = 1;
static BOOL   bSetTrigger = 0;
BOOL   bPass4L = 0;
BOOL   bPass4M = 0;
BOOL   bPass4D = 0;
BOOL   bCryptTraf = 0;
char * pAccPath  = NULL;
char * pTrigger = NULL;
char * pPendTrigger = NULL;
ULONG  ulVarsMax = 10000;
USHORT uiVarLenMax = 10000;
USHORT uiVarsOwnMax = 50;
USHORT uiCacheRecords = 10;
static BOOL bOptimize = 0;
USHORT uiAutOrder = 0;
USHORT uiMemoType = 0;
BOOL bForceOpt = FALSE;
BOOL bLockConnect = FALSE;
static BOOL bLockLock = FALSE;
static int iUserLock = -1;

//PUSERSTRU pUStru_Curr = NULL;
//ULONG  ulAreaID_Curr  = 0;

static HB_CRITICAL_NEW( mutex_Open );
static HB_CRITICAL_NEW( mutex_AreaID );
static HB_CRITICAL_NEW( mutex_VARS );

extern int leto_GlobalExit;
extern const char * szRelease;

extern char * leto_memoread( const char * szFilename, ULONG *pulLen );
extern BOOL leto_fileread( const char * szFilename, const char * pBuffer, const ULONG ulStart, ULONG *ulLen );
extern BOOL leto_filesize( const char * szFilename, ULONG *pulLen );
extern char * leto_directory( const char * szDirSpec, const char * szAttributes, char * pBuffer, ULONG *pulLen );
extern BOOL leto_filewrite( const char * szFilename, const char * pBuffer, const ULONG ulStart, ULONG ulLen );
extern BOOL leto_memowrite( const char * szFilename, const char * pBuffer, ULONG ulLen );

extern void leto_acc_read( const char * szFilename );
extern void leto_acc_release( void );
extern char * leto_acc_find( const char * szUser, const char * szPass );
extern BOOL leto_acc_add( const char * szUser, const char * szPass, const char * szAccess );
extern void leto_acc_flush( const char * szFilename );
extern void leto_vars_release( void );
extern void leto_varsown_release( PUSERSTRU pUStru );
extern void leto_SendAnswer( PUSERSTRU pUStru, const char* szData, ULONG ulLen );
extern void leto_Admin( PUSERSTRU pUStru, char* szData );
//extern void leto_Intro( PUSERSTRU pUStru, char* szData );
//extern void leto_Mgmt( PUSERSTRU pUStru, const char* szData );
extern void leto_Set( PUSERSTRU pUStru, const char* szData );
extern void leto_Variables( PUSERSTRU pUStru, char* szData );

void letoListInit( PLETO_LIST pList, ULONG ulSize );
void letoListFree( PLETO_LIST pList );
void * letoAddToList( PLETO_LIST pList );
void * letoGetListItem( PLETO_LIST pList, USHORT uiNum );
BOOL letoDelFromList( PLETO_LIST pList, USHORT uiNum );
BOOL letoIsRecInList( PLETO_LIST pLockList, ULONG ulRecNo );
void letoAddRecToList( PLETO_LIST pLockList, ULONG ulRecNo );
BOOL letoDelRecFromList( PLETO_LIST pLockList, ULONG ulRecNo );
void letoClearList( PLETO_LIST pList );

static int leto_IsRecLocked( PAREASTRU pAStru, ULONG ulRecNo );
static void leto_SetScope( AREAP pArea, LETOTAG *pTag, BOOL bTop, BOOL bSet );


static HB_CRITICAL_NEW( s_fileMtx );

void leto_writelog( const char * sFile, int n, const char * s, ... )
{
   char sFileDef[_POSIX_PATH_MAX];

   hb_threadEnterCriticalSection( &s_fileMtx );

   if( !sFile )
   {
      memcpy( sFileDef, szDirBase, iDirBaseLen );
      strcpy( sFileDef + iDirBaseLen, "letodb.log" );
   }

   if( n )
   {
      HB_FHANDLE handle;
      if( hb_fsFile( (sFile)? sFile : sFileDef ) )
         handle = hb_fsOpen( (sFile)? sFile : sFileDef, FO_WRITE );
      else
         handle = hb_fsCreate( (sFile)? sFile : sFileDef, 0 );

      hb_fsSeek( handle, 0, SEEK_END );
      hb_fsWrite( handle, s, (n) ? n : (int) strlen(s) );
      hb_fsWrite( handle, "\n\r", 2 );
      hb_fsClose( handle );
   }
   else
   {
      FILE * hFile = hb_fopen( (sFile)? sFile : sFileDef, "a" );

      va_list ap;
      if( hFile )
      {
         va_start( ap, s );
         vfprintf( hFile, s, ap );
         va_end( ap );
         fclose( hFile );
      }
   }

   hb_threadLeaveCriticalSection( &s_fileMtx );
}

PUSERSTRU leto_FindUserStru( HB_THREAD_ID hThreadID )  // mt
{
   PUSERSTRU pUStru = s_users;
   int ui;

   if( hThreadID )
      for( ui=0; ui<uiUsersMax; ui++,pUStru++ )
         if( HB_THREAD_EQUAL( pUStru->hThreadID, hThreadID ) )
            return pUStru;

   return NULL;
}

HB_FUNC( LETO_CLIENTID )
{
   PUSERSTRU pUStru = leto_FindUserStru( HB_THREAD_SELF() );
   hb_retni( pUStru ? pUStru - s_users : -1 );
}

void leto_wUsLog( PUSERSTRU pUStru, const char* s, int n )
{
   HB_FHANDLE handle;
   char szName[_POSIX_PATH_MAX];

   if( ! s )
      return;

   if( !pUStru )
      pUStru = leto_FindUserStru( HB_THREAD_SELF() );

   if( pUStru )
   {
      memcpy( szName, szDirBase, iDirBaseLen );
      hb_snprintf( szName + iDirBaseLen, _POSIX_PATH_MAX-iDirBaseLen, "letodb_%d.log", pUStru->iUserStru );
      if( hb_fsFile( szName ) )
      {
         handle = hb_fsOpen( szName, FO_WRITE );
         hb_fsSeek( handle,0, SEEK_END );
      }
      else
      {
         handle = hb_fsCreate( szName, 0 );
         hb_fsWrite( handle, pUStru->szAddr, strlen( (const char *) pUStru->szAddr) );
         if( pUStru->szNetname )
         {
            hb_fsWrite( handle, " ", 1 );
            hb_fsWrite( handle, pUStru->szNetname, strlen( (const char *) pUStru->szNetname) );
         }
         if( pUStru->szExename )
         {
            hb_fsWrite( handle, " ", 1 );
            hb_fsWrite( handle, pUStru->szExename, strlen( (const char *) pUStru->szExename) );
         }
         hb_fsWrite( handle, "\r\n\r\n", 4 );
      }

      hb_fsWrite( handle, s, (n) ? n : (int) strlen(s) );
      hb_fsWrite( handle, "\r\n", 2 );

      hb_fsClose( handle );
   }
   else
      leto_writelog( NULL, n, s );
}

HB_FUNC( LETO_WUSLOG )
{
   leto_wUsLog( NULL, hb_parc(1), hb_parclen(1) );
}

HB_FUNC( SETHRBERROR )  // mt
{
   PUSERSTRU pUStru = leto_FindUserStru( HB_THREAD_SELF() );
   if( pUStru )
      pUStru->bHrbError = 1;
   else
      leto_writelog( NULL,0,"ERROR! SETHRBERROR - pUStru not found!!!!!!!!!!!!!!!!!!!!\r\n" );
}

char * leto_ErrorRT( HB_ERRCODE errGenCode, HB_ERRCODE errSubCode, HB_ERRCODE errOsCode, unsigned int uiFlags, char *szFilename )
{
   int uiLen = 50;
   char *szBuf;

   if( szFilename )
      uiLen += _POSIX_PATH_MAX;
   szBuf = hb_xgrab( uiLen );
   hb_snprintf( szBuf, uiLen, "-004:%d-%d-%d-%d", (int)errGenCode, (int)errSubCode, (int)errOsCode, (int)uiFlags );
   if( szFilename )
   {
      strcat( szBuf, "\t" );
      strcat( szBuf, szFilename );
   }

   return szBuf;
}

static char * leto_Driver( USHORT uiDriver )
{
#ifdef __BM
   return (uiDriver == LETO_NTX ? "BMDBFNTX" : "BMDBFCDX");
#else
   return (uiDriver == LETO_NTX ? "DBFNTX" : "DBFCDX");
#endif
}

HB_FUNC( LETO_DRIVER )
{
   hb_retc( leto_Driver( hb_parni( 1 ) ) );
}

void leto_initSet( void )
{
   PHB_ITEM pItem = hb_itemNew( NULL );

   hb_rddDefaultDrv( leto_Driver( LETO_CDX ) );
   hb_itemPutL( pItem, TRUE );
   hb_setSetItem( HB_SET_AUTOPEN, pItem );
   if( bOptimize )
      hb_setSetItem( HB_SET_HARDCOMMIT, pItem );
#ifdef __BM
   hb_setSetItem( HB_SET_OPTIMIZE, pItem );
   if( bForceOpt )
      hb_setSetItem( HB_SET_FORCEOPT, pItem );
#endif
   if( uiAutOrder )
   {
      hb_itemPutNI( pItem, uiAutOrder );
      hb_setSetItem( HB_SET_AUTORDER, pItem );
   }
   if( uiMemoType || pTrigger || pPendTrigger )
   {
      LPRDDNODE  pRDDNode;
      HB_USHORT  uiRddID;

      pRDDNode = hb_rddFindNode( leto_Driver( uiDriverDef ), &uiRddID );
      if( pRDDNode )
      {
         if( uiMemoType ) {
            hb_itemPutNI( pItem, uiMemoType );
            SELF_RDDINFO( pRDDNode, RDDI_MEMOTYPE, 0, pItem );
         }
         if( pTrigger ) {
            hb_itemPutC( pItem, pTrigger );
            SELF_RDDINFO( pRDDNode, RDDI_TRIGGER, 0, pItem );
         }
         if( pPendTrigger ) {
            hb_itemPutC( pItem, pPendTrigger );
            SELF_RDDINFO( pRDDNode, RDDI_PENDINGTRIGGER, 0, pItem );
         }
      }
   }
   hb_itemRelease( pItem );
}

HB_FUNC( LETO_INITSET )
{
   leto_initSet();
}

HB_FUNC( LETO_SETDIRBASE )
{
   iDirBaseLen = hb_parclen( 1 );
   strcpy( szDirBase, hb_parc(1) );
}

void leto_MakeAlias( ULONG ulAreaID, char * szAlias )  // mt
{
   hb_snprintf( szAlias, HB_RDD_MAX_ALIAS_LEN + 1, "LETO%lu", ulAreaID );
}

HB_FUNC( LETO_MAKEALIAS )  // mt
{
   ULONG ulAreaID = hb_parnl( 1 );
   char szAlias[HB_RDD_MAX_ALIAS_LEN + 1];

   leto_MakeAlias( ulAreaID, szAlias );
   hb_retc( szAlias );
}

AREAP leto_RequestArea( ULONG ulAreaID )  // mt
{
   char szAlias[HB_RDD_MAX_ALIAS_LEN + 1];
   int iArea;
   AREAP pArea;

   if( !ulAreaID )
   {
      leto_wUsLog(NULL,"ERROR! leto_RequestArea!\r\n", 0);
      return NULL;
   }

   leto_MakeAlias( ulAreaID, szAlias );

   if( bNoSaveWA )
   {
      hb_rddGetAliasNumber( szAlias, &iArea );
      if( iArea > 0 )
         pArea = hb_rddGetWorkAreaPointer( iArea );
      else
      {
         char sBuf[100+HB_RDD_MAX_ALIAS_LEN];
         hb_snprintf( sBuf, 100+HB_RDD_MAX_ALIAS_LEN, "ERROR! leto_RequestArea! hb_rddGetAliasNumber! %s\r\n", szAlias );
         leto_wUsLog( NULL, sBuf, 0 );
         return 0;
      }
   }
   else
   {
      pArea = hb_rddRequestArea( szAlias, NULL, TRUE, FALSE );
      if( ! pArea )
      {
         pArea = hb_rddRequestArea( szAlias, NULL, TRUE, TRUE );
         if( ! pArea )
         {
            char sBuf[100+8];
            hb_snprintf( sBuf, 100+8, "ERROR!!!!!!!!!!! hb_rddRequestArea! %lu\r\n", ulAreaID );
            leto_wUsLog( NULL, sBuf, 0 );
            return NULL;
         }
      }
   }
   hb_rddSelectWorkAreaNumber( pArea->uiArea );
   hb_cdpSelect( pArea->cdPage );

   return pArea;
}

BOOL leto_DetachArea( ULONG ulAreaID )  // mt
{
   AREAP pArea = NULL;
   BOOL bRetVal = FALSE;

   if( ulAreaID )
   {
      char szAlias[HB_RDD_MAX_ALIAS_LEN + 1];
      int iArea;

      leto_MakeAlias( ulAreaID, szAlias );
      if( hb_rddGetAliasNumber( szAlias, &iArea ) == HB_SUCCESS )
         pArea = hb_rddGetWorkAreaPointer( iArea );
      else
      {
         char sBuf[100+HB_RDD_MAX_ALIAS_LEN];
         hb_snprintf( sBuf, 100+HB_RDD_MAX_ALIAS_LEN, "ERROR! leto_DetachArea! hb_rddGetAliasNumber! %s\r\n", szAlias );
         leto_wUsLog( NULL, sBuf, 0 );
      }
   }
   else
      pArea = hb_rddGetCurrentWorkAreaPointer();

   if( pArea )
   {
      if( bNoSaveWA )
         bRetVal = TRUE;
      else
      {
         bRetVal = ( hb_rddDetachArea( pArea, NULL ) == HB_SUCCESS );
         if( !bRetVal )
         {
            if( ulAreaID )
            {
               char sBuf[100+8];
               hb_snprintf( sBuf, 100+8, "ERROR! leto_DetachArea! hb_rddDetachArea! %lu\r\n", ulAreaID );
               leto_wUsLog( NULL, sBuf, 0 );
            }
            else
               leto_wUsLog( NULL, "ERROR! leto_DetachArea! hb_rddDetachArea! current\r\n", 0 );
         }
      }
   }
   else
      leto_wUsLog(NULL,"ERROR! leto_DetachArea! can't select Area\r\n", 0);

   return bRetVal;
}

PAREASTRU leto_FindArea( PUSERSTRU pUStru, ULONG ulAreaID )  // mt
{
   PAREASTRU pAStru;
   PLETO_LIST_ITEM pListItem;

   if( !pUStru || !ulAreaID )
   {
      leto_wUsLog(pUStru,"ERROR! leto_FindArea!\r\n", 0);
      return NULL;
   }

   pListItem = pUStru->AreasList.pItem;
   while( pListItem )
   {
      pAStru = ( PAREASTRU ) ( pListItem + 1 );
      if( pAStru->ulAreaID == ulAreaID )
         return pAStru;
      pListItem = pListItem->pNext;
   }

   return NULL;
}

PAREASTRU leto_FindAlias( PUSERSTRU pUStru, char * szAlias )  // change!!!
{
   PAREASTRU pAStru;
   PLETO_LIST_ITEM pListItem;

   if( !pUStru || !szAlias || *szAlias == '\0' )
   {
      leto_wUsLog(pUStru,"ERROR! leto_FindAlias!\r\n", 0);
      return NULL;
   }

   pListItem = pUStru->AreasList.pItem;
   hb_strLower( szAlias, strlen(szAlias) );
   while( pListItem )
   {
      pAStru = ( PAREASTRU ) ( pListItem + 1 );
      if( pAStru->ulAreaID && !strcmp( pAStru->szAlias, szAlias ) )
         return pAStru;
      pListItem = pListItem->pNext;
   }

   return NULL;
}

BOOL leto_FreeArea( PUSERSTRU pUStru, BOOL bCurTable )  // mt
{
   BOOL lRet = TRUE;

   if( !pUStru )
   {
      leto_wUsLog(pUStru,"ERROR! leto_FreeArea!\r\n", 0);
      return FALSE;
   }

   if( bCurTable )
   {
      if( pUStru->ulCurAreaID && pUStru->pCurAStru && pUStru->pCurAStru->bNotDetach )
      {
         lRet = leto_DetachArea( pUStru->ulCurAreaID );
         if( lRet )
            pUStru->pCurAStru->bNotDetach = FALSE;
      }
   }
   else
   {
      PAREASTRU pAStru;
      PLETO_LIST_ITEM pListItem = pUStru->AreasList.pItem;
      while( pListItem )
      {
         pAStru = ( PAREASTRU ) ( pListItem + 1 );
         if( pAStru->ulAreaID && pAStru->bNotDetach )
         {
            if( leto_DetachArea( pAStru->ulAreaID ) )
               pAStru->bNotDetach = FALSE;
         }
         pListItem = pListItem->pNext;
      }

      pUStru->ulCurAreaID = 0;
      pUStru->pCurAStru   = NULL;
      lRet = TRUE;
   }

   return lRet;
}

AREAP leto_SelectArea( PUSERSTRU pUStru, ULONG ulAreaID )  // mt
{
   AREAP pArea = NULL;

   if( ulAreaID && ( ulAreaID != pUStru->ulCurAreaID || !pUStru->pCurAStru ) )
   {
//       if( pUStru->pCurAStru && pUStru->pCurAStru->bNotDetach )
//       {
//          leto_FreeArea( pUStru, TRUE );
//          pUStru->pCurAStru->bNotDetach  = FALSE;
//       }
      PAREASTRU pAStru;

      pUStru->ulCurAreaID = 0;
      pUStru->pCurAStru   = NULL;

      if( ( pAStru = leto_FindArea( pUStru, ulAreaID ) ) != NULL )
      {
         if( pAStru->bNotDetach )
         {
            char szAlias[HB_RDD_MAX_ALIAS_LEN + 1];
            int iArea;

            pUStru->ulCurAreaID = ulAreaID;
            pUStru->pCurAStru   = pAStru;

            leto_MakeAlias( ulAreaID, szAlias );
            hb_rddGetAliasNumber( szAlias, &iArea );
            if( iArea > 0 )
               pArea = hb_rddGetWorkAreaPointer( iArea );
         }
         else
         {
            if( ( pArea = leto_RequestArea( ulAreaID ) ) != NULL )
            {
               pUStru->ulCurAreaID = ulAreaID;
               pUStru->pCurAStru   = pAStru;
               pAStru->bNotDetach  = TRUE;
            }
         }
      }
      else
         leto_wUsLog(pUStru,"ERROR! leto_SelectArea! FindArea\r\n", 0);
   }
   else
   {
      if( !pUStru || !pUStru->ulCurAreaID || !pUStru->pCurAStru )
      {
         leto_wUsLog(pUStru,"ERROR! leto_SelectArea!!\r\n", 0);
         return NULL;
      }

      if( pUStru->pCurAStru->bNotDetach )
      {
         pArea = hb_rddGetCurrentWorkAreaPointer();
      }
      else
      {
         if( ( pArea = leto_RequestArea( pUStru->ulCurAreaID ) ) != NULL )
            pUStru->pCurAStru->bNotDetach  = TRUE;
      }
   }

   return pArea;
}

HB_FUNC ( LETO_SELECTAREA )
{
   int iUserStru  = hb_parni( 1 );
   ULONG ulAreaID = hb_parnl( 2 );

   if( iUserStru < 0 || iUserStru >= uiUsersAlloc || ulAreaID <= 0 )
   {
      leto_wUsLog(NULL,"ERROR! LETO_SELECTAREA!\r\n", 0);
      hb_retl( FALSE );
      return;
   }

   if( leto_SelectArea( s_users + iUserStru, ulAreaID ) )
      hb_retl( TRUE );
   else
      hb_retl( FALSE );
}

ULONG leto_CreateAreaID( void )  // mt
{
   ULONG ulRetVal;

   hb_threadEnterCriticalSection( &mutex_AreaID );

   if( s_AvailIDS.iCurIndex <=0 )
   {
      ++ s_AvailIDS.ulNextID;
      ulRetVal = s_AvailIDS.ulNextID;
   }
   else
   {
      -- s_AvailIDS.iCurIndex;
      ulRetVal = s_AvailIDS.pulAreaID[ s_AvailIDS.iCurIndex ];
   }

   hb_threadLeaveCriticalSection( &mutex_AreaID );

   return ulRetVal;
}

HB_FUNC( LETO_CREATEAREAID )  // mt
{
   hb_retnl( leto_CreateAreaID() );
}

void leto_DelAreaID( ULONG nAreaID )  // mt
{
   if( !nAreaID )
   {
      leto_wUsLog(NULL,"ERROR! leto_DelAreaID! param\r\n", 0);
      return;
   }

   hb_threadEnterCriticalSection( &mutex_AreaID );

   if( nAreaID > s_AvailIDS.ulNextID )
   {
      hb_threadLeaveCriticalSection( &mutex_AreaID );
      leto_wUsLog(NULL,"ERROR! leto_DelAreaID! nAreaID > s_AvailIDS.ulNextID\r\n", 0);
      return;
   }

   if( !s_AvailIDS.pulAreaID )
   {
      s_AvailIDS.pulAreaID = (ULONG*) hb_xgrab( sizeof(ULONG) * 100 );
      s_AvailIDS.iAllocIndex = 100;
      s_AvailIDS.iCurIndex   = 0;
   }
   else if( s_AvailIDS.iCurIndex >= s_AvailIDS.iAllocIndex )
   {
      s_AvailIDS.iAllocIndex += 100;
      s_AvailIDS.pulAreaID = (ULONG*) hb_xrealloc( s_AvailIDS.pulAreaID, sizeof(ULONG) * s_AvailIDS.iAllocIndex );
   }

   s_AvailIDS.pulAreaID[ s_AvailIDS.iCurIndex ] = nAreaID;
   ++s_AvailIDS.iCurIndex;

   hb_threadLeaveCriticalSection( &mutex_AreaID );
}

HB_FUNC( LETO_DELAREAID )  // mt
{
   leto_DelAreaID( hb_parnl(1) );
}

void leto_profile( int iMode, int i1, ULONG ** parr1, ULONG ** parr2 )
{
   static ULONG arr1[10];
   static ULONG arr2[10];
   static ULONG arr3[10];
   ULONG ul = leto_MilliSec();

   switch( iMode )
   {
      case 0:
         memset( arr1,0,sizeof(ULONG)*10 );
         memset( arr2,0,sizeof(ULONG)*10 );
         memset( arr3,0,sizeof(ULONG)*10 );
         break;
      case 1:
         arr3[i1] = ul;
         break;
      case 2:
         if( arr3[i1] != 0 && ul >= arr3[i1] )
         {
            arr1[i1] += ul - arr3[i1];
            arr2[i1] ++;
            arr3[i1] = 0;
         }
         break;
      case 3:
         *parr1 = arr1;
         *parr2 = arr2;
         break;
   }
}

long leto_Date( void )  // mt
{
   int iYear, iMonth, iDay;
   hb_dateToday( &iYear, &iMonth, &iDay );
   return hb_dateEncode( iYear, iMonth, iDay );
}

/*
HB_FUNC( LETO_READMEMAREA )  // mt
{
   char * pBuffer = (char*) hb_xgrab( hb_parni(2)+1 );
   if( !leto_ReadMemArea( pBuffer, hb_parni(1), hb_parni(2) ) )
   {
      hb_xfree( pBuffer );
      hb_ret();
   }
   else
      hb_retc_buffer( pBuffer );
}

HB_FUNC( LETO_WRITEMEMAREA )  // mt
{
   leto_WriteMemArea( hb_parc(1), hb_parni(2), hb_parni(3) );
}
*/

HB_FUNC( GETCMDITEM )  // mt
{
   PHB_ITEM pText = hb_param( 1, HB_IT_STRING );
   PHB_ITEM pStart = hb_param( 2, HB_IT_NUMERIC );

   if( pText && pStart )
   {
      ULONG ulSize = hb_itemGetCLen( pText );
      ULONG ulStart = pStart ? hb_itemGetNL( pStart ) : 1;
      ULONG ulEnd = ulSize, ulAt;
      const char *Text = hb_itemGetCPtr( pText );

      if ( ulStart > ulSize )
      {
         hb_retc( NULL );
         if( HB_ISBYREF( 3 ) ) hb_storni( 0, 3 );
      }
      else
      {
         ulAt = hb_strAt( ";", 1, Text + ulStart - 1, ulEnd - ulStart + 1 );
         if ( ulAt > 0)
            ulAt += ( ulStart - 1 );

         if( HB_ISBYREF( 3 ) )
            hb_storni( ulAt, 3 );

         if ( ulAt != 0)
         {
            ULONG ulLen = ulAt - ulStart;

            if( ulStart ) ulStart--;

            if( ulStart < ulSize )
            {
               if( ulLen > ulSize - ulStart )
               {
                  ulLen = ulSize - ulStart;
               }

               if( ulLen > 0 )
               {
                  if( ulLen == ulSize )
                     hb_itemReturn( pText );
                  else
                     hb_retclen( Text + ulStart, ulLen );
               }
               else
                  hb_retc( NULL );
            }
            else
               hb_retc( NULL );
         }
         else
            hb_retc( NULL );
      }
   }
   else
   {
      if( HB_ISBYREF( 3 ) )
         hb_storni( 0, 3 );
      hb_retc( NULL );
   }
}

static char leto_ExprGetType( PUSERSTRU pUStru, const char* pExpr, int iLen )
{
   PHB_ITEM pItem = hb_itemPutCL( NULL, pExpr, iLen );
   const char * szType;

   hb_xvmSeqBegin();
   szType = hb_macroGetType( pItem );
   hb_xvmSeqEnd();
   hb_itemClear( pItem );
   if( pUStru->bHrbError )
   {
      hb_itemRelease( pItem );
      return ' ';
   }
   else if( szType[ 0 ] == 'U' && szType[ 1 ] == 'I' )
   {
      PHB_MACRO pMacro;
      PHB_ITEM pValue;
      char * szExpr = hb_strndup( pExpr, iLen );

      hb_xvmSeqBegin();
      pMacro = hb_macroCompile( szExpr );
      if( pMacro )
      {
         pItem = hb_itemPutPtr( pItem, ( void * ) pMacro );
         pValue = hb_vmEvalBlockOrMacro( pItem );
      }
      else
         pValue = NULL;
      hb_xvmSeqEnd();
      hb_itemClear( pItem );
      hb_itemRelease( pItem );
      hb_xfree( szExpr );
      if( pUStru->bHrbError )
         return ' ';
      else
         return hb_itemTypeStr( pValue )[ 0 ];
   }
   else
   {
      hb_itemRelease( pItem );
      return szType[0];
   }
}

static BOOL leto_checkExpr( PUSERSTRU pUStru, const char* pExpr, int iLen )  // mt
{
   return leto_ExprGetType( pUStru, pExpr, iLen ) == 'L';
}

void leto_IndexInfo( char * szRet, ULONG ilLenRet, int iNumber, const char * szKey )  // mt
{
   int iLen;
   DBORDERINFO pOrderInfo;
   AREAP pArea = hb_rddGetCurrentWorkAreaPointer();

   if( pArea )
   {
      memset( &pOrderInfo, 0, sizeof( DBORDERINFO ) );
      pOrderInfo.itmOrder = hb_itemPutNI( NULL, iNumber );
      pOrderInfo.itmResult = hb_itemPutC( NULL, "" );
      SELF_ORDINFO( pArea, DBOI_BAGNAME, &pOrderInfo );
      strcpy(szRet, hb_itemGetCPtr( pOrderInfo.itmResult ) );
      iLen = strlen(szRet);
      szRet[ iLen ++ ] = ';';
      hb_itemClear( pOrderInfo.itmResult );

      hb_itemPutC( pOrderInfo.itmResult, "" );
      SELF_ORDINFO( pArea, DBOI_NAME, &pOrderInfo );
      strcpy(szRet + iLen, hb_itemGetCPtr( pOrderInfo.itmResult ) );
      iLen = strlen(szRet);
      szRet[ iLen ++ ] = ';';
      hb_itemClear( pOrderInfo.itmResult );

      strcpy(szRet + iLen, szKey );
      iLen = strlen(szRet);
      szRet[ iLen ++ ] = ';';

      hb_itemPutC( pOrderInfo.itmResult, "" );
      SELF_ORDINFO( pArea, DBOI_CONDITION, &pOrderInfo );
      strcpy(szRet + iLen, hb_itemGetCPtr( pOrderInfo.itmResult ) );
      iLen = strlen(szRet);
      szRet[ iLen ++ ] = ';';
      hb_itemClear( pOrderInfo.itmResult );

      hb_itemPutC( pOrderInfo.itmResult, "" );
      SELF_ORDINFO( pArea, DBOI_KEYTYPE, &pOrderInfo );
      strcpy(szRet + iLen, hb_itemGetCPtr( pOrderInfo.itmResult ) );
      iLen = strlen(szRet);
      szRet[ iLen ++ ] = ';';
      hb_itemClear( pOrderInfo.itmResult );

      hb_itemPutNI( pOrderInfo.itmResult, 0 );
      SELF_ORDINFO( pArea, DBOI_KEYSIZE, &pOrderInfo );
      hb_snprintf( szRet + iLen, ilLenRet-iLen, "%lu;", hb_itemGetNL(pOrderInfo.itmResult) );
      iLen += strlen( szRet + iLen );

      hb_itemPutL( pOrderInfo.itmResult, FALSE );
      SELF_ORDINFO( pArea, DBOI_ISDESC, &pOrderInfo );
      hb_snprintf( szRet + iLen, ilLenRet-iLen, "%s;", hb_itemGetL(pOrderInfo.itmResult) ? "T" : "F" );
      iLen += strlen( szRet + iLen );

      hb_itemPutL( pOrderInfo.itmResult, FALSE );
      SELF_ORDINFO( pArea, DBOI_UNIQUE, &pOrderInfo );
      hb_snprintf( szRet + iLen, ilLenRet-iLen, "%s;", hb_itemGetL(pOrderInfo.itmResult) ? "T" : "F" );
      iLen += strlen( szRet + iLen );

      hb_itemPutL( pOrderInfo.itmResult, FALSE );
      SELF_ORDINFO( pArea, DBOI_CUSTOM, &pOrderInfo );
      hb_snprintf( szRet + iLen, ilLenRet-iLen, "%s;", hb_itemGetL(pOrderInfo.itmResult) ? "T" : "F" );

      hb_itemRelease( pOrderInfo.itmOrder );
      hb_itemRelease( pOrderInfo.itmResult );

   }
   else
   {
      szRet[0] = 0;
   }
}

/*
HB_FUNC( LETO_INDEXINFO )  // mt
{
   char szRet[_POSIX_PATH_MAX + LETO_MAX_TAGNAME + LETO_MAX_EXP*2 + 30];
   leto_IndexInfo( szRet, _POSIX_PATH_MAX + LETO_MAX_TAGNAME + LETO_MAX_EXP*2 + 30, hb_parni(1), hb_parc(2) );
   hb_retc( szRet );
}
*/

#define MAXDEEP  5

BOOL leto_ParseFilter( PUSERSTRU pUStru, const char * pNew, ULONG ulFltLen )  // mt
{
   int iMode = 0;
   BOOL bLastCond=0;
   int iDeep = 0;
   int piDeep[MAXDEEP];
   const char *ppDeep[MAXDEEP];
   const char * ptr, * ptr1;
   char c, cQuo=' ';

   for( ptr=ptr1=pNew; ulFltLen; ptr++,ulFltLen-- )
   {
      c = *ptr;
      if( iMode == 0 )
      {
         if( c == '\"' || c == '\'' )
         {
            iMode = 1;        // mode 1 - a string
            cQuo = c;
         }
         else if( HB_ISFIRSTIDCHAR( c ) )
         {
            iMode = 2;        // mode 2 - an identifier
            ppDeep[iDeep] = ptr;
         }
         else if( HB_ISDIGIT( c ) )
         {
            iMode = 3;        // mode 3 - a number
         }
         else if( c == '(' )  // parenthesis, but not of a function
         {
            piDeep[iDeep] = 1;
            ppDeep[iDeep] = ptr+1;
            if( ++iDeep >= MAXDEEP )
            {
               return FALSE;
            }
            piDeep[iDeep] = 0;
            ppDeep[iDeep] = NULL;
         }
         else if( ( c == '>' ) && ( ptr > pNew ) && ( *(ptr-1) == '-' ) )
         {
            return FALSE;
         }
      }
      else if( iMode == 1 )   // We are inside a string
      {
         if( c == cQuo )
            iMode = 0;
      }
      else if( iMode == 2 )   // We are inside an identifier
      {
         if( c == '(' )
         {
            piDeep[iDeep] = 2;
            if( ++iDeep >= MAXDEEP )
            {
               return FALSE;
            }
            piDeep[iDeep] = 0;
            ppDeep[iDeep] = NULL;
            iMode = 0;
         }
         else if( ( c == '-' ) && ( ulFltLen > 1 ) && ( *(ptr+1) == '>' ) )     /* Check "FIELD->" */
         {
            if( iDeep >= 0 && ppDeep[iDeep] && ( ptr - ppDeep[iDeep] == 5 ) )
            {
               char pBuf[6];
               memcpy( pBuf, ppDeep[iDeep], 5 );
               pBuf[5] = '\0';
               hb_strLower( pBuf, 5 );
               if( strcmp( pBuf, "field" ) == 0 )
               {
                  -- ulFltLen;
                  ++ ptr;
               }
               else
                  return FALSE;
            }
            else
               return FALSE;
         }
         else if( !( HB_ISNEXTIDCHAR( c ) || c == ' ' ) )
         {
            iMode = 0;
         }
      }
      else if( iMode == 3 )   // We are inside a number
      {
         if( !( HB_ISDIGIT( c ) || ( c =='.' && ulFltLen > 1 && HB_ISDIGIT( *(ptr+1) ) ) ) )
            iMode = 0;
      }
      if( iMode != 1 )
      {
         if( c == ')' )
         {
            if( --iDeep < 0 )
            {
               return FALSE;
            }
            if( bLastCond && ptr1 > ppDeep[iDeep] )
            {
               if( !leto_checkExpr( pUStru, ptr1, ptr-ptr1 ) )
               {
                  return FALSE;
               }
               ptr1 = ptr + 1;
               bLastCond = 0;
            }
         }
         else if( iMode != 3 && c == '.'  && (
               ( ulFltLen >= 5 && *(ptr+4) == '.' && ( *(ptr+1)=='A' || *(ptr+1)=='a' ) && ( *(ptr+2)=='N' || *(ptr+2)=='n' ) && ( *(ptr+3)=='D' || *(ptr+3)=='d' ) )
               || ( ulFltLen >= 4 && *(ptr+3) == '.' && ( *(ptr+1)=='O' || *(ptr+1)=='o' ) && ( *(ptr+2)=='R' || *(ptr+2)=='r' ) ) ) )
         {
            // ptr stays at a beginning of .and. or .or.
            if( iDeep && ( ppDeep[iDeep-1] > ptr1 ) )
            {
               ptr1 = ppDeep[iDeep-1];
               bLastCond = 1;
            }
            if( bLastCond )
            {
               if( iDeep && piDeep[iDeep-1] == 2 )
                  while( *ptr1 != '(' ) ptr1 ++;
               if( *ptr1 == '(' )
                  ptr1 ++;
               if( ( ptr > (ptr1+1) ) && !leto_checkExpr( pUStru, ptr1, ptr-ptr1 ) )
               {
                  return FALSE;
               }
            }
            ptr += 3;
            ulFltLen -= 3;
            if( ulFltLen > 0 && *ptr != '.' )
            {
               ptr ++;
               ulFltLen --;
            }
            ptr1 = ptr + 1;
            bLastCond = 1;
         }
      }
   }
   ptr --;
   if( (ptr1 > pNew) && ((ptr - ptr1) > 2) && !leto_checkExpr( pUStru, ptr1, ptr-ptr1+1 ) )
   {
      return FALSE;
   }

   return TRUE;
}

HB_FUNC( LETO_SETAPPOPTIONS )  // mt
{
   USHORT uiLen;

   if( ! HB_ISNIL(1) )
   {
      uiLen = (USHORT) hb_parclen(1);
      pDataPath = (char*) hb_xgrab( uiLen+1 );
      memcpy( pDataPath, hb_parc(1), uiLen );
      pDataPath[uiLen] = '\0';
   }

   uiDriverDef = hb_parni(2);
   bFileFunc = hb_parl(3);
   bAnyExt   = hb_parl(4);
   bPass4L   = hb_parl(5);
   bPass4M   = hb_parl(6);
   bPass4D   = hb_parl(7);

   if( ! HB_ISNIL(8) )
   {
      uiLen = (USHORT) hb_parclen(8);
      pAccPath = (char*) hb_xgrab( uiLen + iDirBaseLen + 1 );
      if( iDirBaseLen )
         memcpy( pAccPath, szDirBase, iDirBaseLen );
      memcpy( pAccPath+iDirBaseLen, hb_parc(8), uiLen );
      pAccPath[ uiLen + iDirBaseLen ] = '\0';
      leto_acc_read( pAccPath );
   }
   bCryptTraf = hb_parl(9);
   bShareTables = hb_parl(10);
   bNoSaveWA = hb_parl(11);

   if( HB_ISNUM(12) )
      ulVarsMax = hb_parnl(12);
   if( HB_ISNUM(13) )
      uiVarLenMax = hb_parnl(13);
   if( HB_ISNUM(14) )
      uiCacheRecords = hb_parnl(14);
   if( HB_ISNUM(15) )
      uiInitTablesMax = hb_parnl(15);
   if( HB_ISNUM(16) )
      uiInitUsersMax = hb_parnl(16);
   if( HB_ISNUM(17) )
      iDebugMode = hb_parnl(17);
   if( HB_ISLOG(18) )
      bOptimize = hb_parl(18);
   if( HB_ISNUM(19) )
      uiAutOrder = hb_parni(19);
   if( HB_ISNUM(20) )
      uiMemoType = hb_parni(20);
   if( HB_ISLOG(21) )
      bForceOpt = hb_parl(21);
   if( ! HB_ISNIL(22) )
   {
      uiLen = (USHORT) hb_parclen(22);
      pTrigger = (char*) hb_xgrab( uiLen+1 );
      memcpy( pTrigger, hb_parc(22), uiLen );
      pTrigger[uiLen] = '\0';
   }
   if( ! HB_ISNIL(23) )
   {
      uiLen = (USHORT) hb_parclen(23);
      pPendTrigger = (char*) hb_xgrab( uiLen+1 );
      memcpy( pPendTrigger, hb_parc(23), uiLen );
      pPendTrigger[uiLen] = '\0';
   }
   if( HB_ISLOG(24) )
      bSetTrigger = hb_parl(24);
   if( HB_ISNUM(25) )
      uiUDF = hb_parni(25);

}

HB_FUNC( LETO_GETAPPOPTIONS )  // mt
{
   USHORT uiNum = hb_parni(1);

   switch( uiNum )
   {
      case 1:
         hb_retc( pDataPath );
         break;
      case 2:
         hb_retni( uiDriverDef );
         break;
      case 3:
         hb_retl( bFileFunc );
         break;
      case 4:
         hb_retl( bAnyExt );
         break;
      case 5:
         hb_retl( bPass4L );
         break;
      case 6:
         hb_retl( bPass4M );
         break;
      case 7:
         hb_retl( bPass4D );
         break;
      case 8:
         if( pAccPath )
            hb_retc( pAccPath );
         else
            hb_ret();
         break;
      case 9:
         hb_retl( bCryptTraf );
         break;
      case 10:
         hb_retl( bShareTables );
         break;
      case 11:
         hb_retl( bNoSaveWA );
         break;
      case 18:
         hb_retl( bOptimize );
         break;
      case 19:
         hb_retni( uiAutOrder );
         break;
      case 20:
         hb_retni( uiMemoType );
         break;
      case 21:
         hb_retl( bForceOpt );
         break;
      case 25:
         hb_retni( uiUDF );
         break;
   }
}

HB_FUNC( LETO_SETSHARED )  // mt
{
   int iTbl = hb_parni( 1 );
   PTABLESTRU pTStru;
   BOOL bRes;

   if( iTbl < 0 || iTbl >= uiTablesAlloc )
   {
      leto_wUsLog(NULL,"ERROR! LETO_SETSHARED!\r\n", 0);
      hb_retl( 0 );
      return;
   }

   pTStru = s_tables + iTbl;
   if( pTStru->ulAreaID <= 0 )
   {
      leto_wUsLog(NULL,"ERROR! LETO_SETSHARED!\r\n", 0);
      hb_retl( 0 );
      return;
   }

   bRes = pTStru->bShared;
   if( HB_ISLOG(2) )
      pTStru->bShared = hb_parl( 2 );

   hb_retl( bRes );
}

HB_FUNC( LETO_TABLENAME )  // mt
{
   int iUserStru = hb_parni( 1 );
   PUSERSTRU pUStru = s_users + iUserStru;
   PAREASTRU pAStru = pUStru->pCurAStru;

   if( iUserStru < 0 || !pAStru || !pAStru->pTStru->szTable )
   {
      leto_wUsLog(NULL,"ERROR! LETO_TABLENAME!\r\n", 0);
      hb_retc( "" );
      return;
   }

   hb_retc( (char*) pAStru->pTStru->szTable );
}

char * leto_RealAlias( PUSERSTRU pUStru, const char * szClientAlias )  //!!! change !!! client-part must be change!
{
   PAREASTRU pAStru = leto_FindAlias( pUStru, (char *) szClientAlias );
   if( pAStru )
   {
      AREAP pArea = (AREAP) leto_SelectArea( pUStru, pAStru->ulAreaID );
//      AREAP pArea = hb_rddGetWorkAreaPointer( pAStru->ulAreaID/512 );

      if( pArea )
      {
         char * szAlias = hb_xgrab( HB_RDD_MAX_ALIAS_LEN + 1 );

         SELF_ALIAS( pArea, ( char * ) szAlias );
         return szAlias;
      }

   }

   return "";
}

HB_FUNC( LETO_ALIAS )  //!!! change !!! client-part must be change!
{
   int iUserStru = hb_parni( 1 );
   if( HB_ISNUM(1) && HB_ISCHAR(2) && ( iUserStru < uiUsersAlloc ) )
   {
      hb_retc_buffer( leto_RealAlias( s_users + iUserStru, hb_parc(2) ) );
   }
   else
      hb_retc( "" );
}

HB_FUNC( LETO_AREAID )
{
   int iUserStru = hb_parni( 1 );
   if( HB_ISNUM(1) && ( iUserStru < uiUsersAlloc ) )
   {
      PUSERSTRU pUStru = s_users + iUserStru;
      hb_retnl( pUStru->pCurAStru ? (long)pUStru->pCurAStru->ulAreaID : -1 );
   }
   else
      hb_retnl( -1 );
}

static BOOL leto_CheckClientVer( PUSERSTRU pUStru, USHORT uiVer )
{
   return pUStru->uiMajorVer*100 + pUStru->uiMinorVer >= uiVer;
}

HB_FUNC( LETO_LASTUPDATE )
{
   int iUserStru = hb_parni( 1 );
   if( HB_ISNUM(1) && ( iUserStru < uiUsersAlloc ) )
   {
      PUSERSTRU pUStru = s_users + iUserStru;
      AREAP pArea = hb_rddGetCurrentWorkAreaPointer();
      if( leto_CheckClientVer( pUStru, 208 ) && pArea )
      {
         char szDate[10];
         PHB_ITEM pItem = hb_itemNew( NULL );
         SELF_INFO( pArea, DBI_LASTUPDATE, pItem );
         hb_itemGetDS( pItem, szDate );
         szDate[8] = ';';
         hb_itemRelease( pItem );
         hb_retclen( szDate, 9 );
      }
      else
         hb_retc( "" );
   }
   else
      hb_retc( "" );
}

HB_FUNC( LETOMEMOINFO )
{
   AREAP pArea = hb_rddGetCurrentWorkAreaPointer();
   if( pArea )
   {
      char szTemp[30];
      PHB_ITEM pItem = hb_itemNew( NULL );
      int iMemoType, iMemoVer;

      SELF_INFO( pArea, DBI_MEMOTYPE, pItem );
      iMemoType = hb_itemGetNI( pItem );
      SELF_INFO( pArea, DBI_MEMOVERSION, pItem );
      iMemoVer = hb_itemGetNI( pItem );
      SELF_INFO( pArea, DBI_MEMOEXT, pItem );

      sprintf( szTemp, "%s;%d;%d;", hb_itemGetCPtr( pItem ), iMemoType, iMemoVer );
      hb_itemRelease( pItem );
      hb_retc( szTemp );
   }
   else
      hb_retc( ";0;0;" );
}

/*
HB_FUNC( LETO_SETFILTER )
{
   ULONG ulAreaID = (ULONG) hb_parnl(1);
   int iUserStru = (HB_ISNIL(2))? 0 : hb_parni( 2 );
   PAREASTRU pAStru;
   PUSERSTRU pUStru = s_users + iUserStru;
   PHB_ITEM pItem = (HB_ISNIL(3))? NULL : hb_param( 3, HB_IT_BLOCK );

   if( ( iUserStru < uiUsersMax ) && ( pAStru = leto_FindArea( pUStru, ulAreaID ) ) != NULL )
   {
      if( pAStru->itmFltExpr )
      {
         hb_itemClear( pAStru->itmFltExpr );
         hb_itemRelease( pAStru->itmFltExpr );
      }
      pAStru->itmFltExpr = (pItem)? hb_itemNew( pItem ) : NULL;
      hb_retl( 1 );
   }
   else
      hb_retl( 0 );
}
*/

LETOTAG * leto_FindTag( PAREASTRU pAStru, const char * szOrder )  //mt
{
   LETOTAG *pTag, *pTagCurrent = NULL;

   if( !pAStru || !szOrder )
   {
      leto_wUsLog(NULL,"ERROR! leto_FindTag!\r\n", 0);
      return NULL;
   }

   pTag = pAStru->pTag;

   while( pTag )
   {
      if( ! strcmp( pTag->szTagName, szOrder) )
      {
         pTagCurrent = pTag;
         break;
      }
      else
         pTag = pTag->pNext;
   }

   return pTagCurrent;
}

BOOL leto_AddTag( PUSERSTRU pUStru, int nIndexStru, const char * szOrder )
{
   PAREASTRU pAStru = pUStru->pCurAStru;
   ULONG ulLen;
   LETOTAG * pTag, * pTagNext;

   ulLen = strlen( szOrder );
   if( ulLen > 11 )
      ulLen = 11;

   pTag = (LETOTAG*) hb_xgrab( sizeof(LETOTAG) );
   memset( pTag, 0, sizeof(LETOTAG) );
   pTag->pIStru = ( PINDEXSTRU ) letoGetListItem( &pAStru->pTStru->IndexList, nIndexStru );
   memcpy( pTag->szTagName, szOrder, ulLen );
   hb_strLower( pTag->szTagName, ulLen );

   if( leto_FindTag( pAStru, pTag->szTagName ) )
   {
      // exists !
      hb_xfree( pTag );
      return FALSE;
   }

   if( pAStru->pTag )
   {
      pTagNext = pAStru->pTag;
      while( pTagNext->pNext )
         pTagNext = pTagNext->pNext;
      pTagNext->pNext = pTag;
   }
   else
      pAStru->pTag = pTag;

   return TRUE;
}

HB_FUNC( LETO_ADDTAG )  //mt
{
   int iUserStru = hb_parni( 1 );
   int       nIndexStru = hb_parni(2);
   const char * szOrder = hb_parc(3);

   if( iUserStru < 0 || nIndexStru < 0 || !szOrder || !*szOrder )
   {
      leto_wUsLog(NULL,"ERROR! LETO_ADDTAG!\r\n", 0);
      hb_retl( 0 );
      return;
   }

   hb_retl( leto_AddTag( s_users + iUserStru, nIndexStru, szOrder ) );
}

HB_FUNC( LETO_N2B )  //mt
{
   unsigned long int n = (unsigned long int)hb_parnl(1);
   int i = 0;
   unsigned char s[8];

   do
   {
      s[i] = (unsigned char)(n & 0xff);
      n = n >> 8;
      i ++;
   }
   while( n );
   s[i] = '\0';
   hb_retclen( (char*)s,i );
}

HB_FUNC( LETO_B2N )  //mt
{
   ULONG n = 0;
   int i = 0;
   BYTE *s = (BYTE*) hb_parc(1);

   do
   {
      n += (unsigned long int)(s[i] << (8*i));
      i ++;
   }
   while( s[i] );
   hb_retnl( n );
}

int leto_GetParam( const char *szData, const char **pp2, const char **pp3, const char **pp4, const char **pp5 )  // mt
{
   char * ptr;
   int iRes = 0;

   if( ( ptr = strchr( szData, ';' ) ) != NULL )
   {
      iRes ++;
      *ptr = '\0';
      if( pp2 )
      {
         *pp2 = ++ptr;
         if( ( ptr = strchr( *pp2, ';' ) ) != NULL )
         {
            iRes ++;
            *ptr = '\0';
            if( pp3 )
            {
               *pp3 = ++ptr;
               if( ( ptr = strchr( *pp3, ';' ) ) != NULL )
               {
                  iRes ++;
                  *ptr = '\0';
                  if( pp4 )
                  {
                     *pp4 = ++ptr;
                     if( ( ptr = strchr( *pp4, ';' ) ) != NULL )
                     {
                        iRes ++;
                        *ptr = '\0';
                        if( pp5 )
                        {
                           *pp5 = ++ptr;
                           if( ( ptr = strchr( *pp5, ';' ) ) != NULL )
                           {
                              iRes ++;
                              *ptr = '\0';
                              ptr ++;
                              if( ( ptr = strchr( ptr, ';' ) ) != NULL )
                              {
                                 iRes ++;
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }

   return iRes;
}

static HB_USHORT leto_FieldCount( AREAP pArea )
{
   HB_USHORT uiFields = 0;
   SELF_FIELDCOUNT( pArea, &uiFields );
   return uiFields;
}

static HB_USHORT leto_RecordLen( AREAP pArea )
{
   PHB_ITEM pItem = hb_itemNew( NULL );
   HB_USHORT uiRecordLen;
   SELF_INFO( pArea, DBI_GETRECSIZE, pItem );
   uiRecordLen = hb_itemGetNI( pItem );
   hb_itemRelease( pItem );
   return uiRecordLen;
}

static char * leto_FieldOffset( AREAP pArea, char * pRecord, HB_USHORT uiIndex )
{
   return pRecord + ( (DBFAREAP) pArea )->pFieldOffset[uiIndex];
}

static ULONG leto_GetOrdInfoNL( AREAP pArea, USHORT uiCommand )
{
   DBORDERINFO pInfo;
   ULONG ulResult;
   memset( &pInfo, 0, sizeof( DBORDERINFO ) );
   pInfo.itmResult = hb_itemPutNL( NULL, 0 );
   SELF_ORDINFO( pArea, uiCommand, &pInfo );
   ulResult = hb_itemGetNL( pInfo.itmResult );
   hb_itemRelease( pInfo.itmResult );
   return ulResult;
}

#define SHIFT_FOR_LEN       3

/*
// 3 bytes - len
//   1 byte  - flags
//   ? bytes - RECNO + ';'
//     //for each field
//       //if eof()
//          1 byte = '\0'
//       //else
//          HB_FT_STRING:   if(FieldLen>255 ? 2 : 1) bytes + field
//          HB_FT_LOGICAL:  field
//          HB_FT_MEMO:     1 byte
//          binary fields:  field
//          OTHER:          1 byte + field
*/

//#define ZIP_RECORD
static ULONG leto_rec( PUSERSTRU pUStru, PAREASTRU pAStru, AREAP pArea, char * szData, ULONG ulLenData )  // mt
{
   char * pData;
   BOOL bRecLocked;
   ULONG ulRealLen;
   PHB_ITEM pItem;

   if( !pAStru || !pArea || !szData || ulLenData <= SHIFT_FOR_LEN + 1 )
   {
      leto_wUsLog(NULL,"ERROR! leto_rec!\r\n", 0);
      return 0;
   }

   if( bCryptTraf )
   {
      if( ulLenData > pUStru->ulBufCryptLen )
      {
         if( !pUStru->ulBufCryptLen )
            pUStru->pBufCrypt = (BYTE*) hb_xgrab( ulLenData );
         else
            pUStru->pBufCrypt = (BYTE*) hb_xrealloc( pUStru->pBufCrypt, ulLenData );
         pUStru->ulBufCryptLen = ulLenData;
      }
      pData = ( char * ) pUStru->pBufCrypt;
   }
   else
   {
      pData = szData + SHIFT_FOR_LEN;
      ulLenData -= SHIFT_FOR_LEN;
   }

   pItem = hb_itemNew( NULL );
   if( SELF_GETVALUE( pArea, 1, pItem ) != SUCCESS )
   {
      ulRealLen = 0;
      hb_itemClear( pItem );
   }
   else
   {

#ifndef ZIP_RECORD
      USHORT ui;
      USHORT uiLen;
      LPFIELD pField;
      char * ptr, * ptrEnd;
      HB_USHORT uiFieldCount = leto_FieldCount( pArea );
#else
      HB_USHORT uiRecordLen = leto_RecordLen( pArea );
#endif
      BYTE * pRecord;
      ULONG ulRecNo;
      BOOL fDeleted;
      ULONG ulRecCount;
      char * pData1 = pData;

      hb_itemClear( pItem );
      SELF_RECNO( pArea, &ulRecNo );
      SELF_DELETED( pArea, &fDeleted );
      SELF_RECCOUNT( pArea, &ulRecCount );
      bRecLocked = ( leto_IsRecLocked( pAStru, ulRecNo ) == 1 );
      *pData = 0x40 + ((pArea->fBof)? LETO_FLG_BOF:0) +
                      ((pArea->fEof)? LETO_FLG_EOF:0) +
                      ((bRecLocked)? LETO_FLG_LOCKED:0) +
                      ((fDeleted)? LETO_FLG_DEL:0) +
                      ((pArea->fFound)? LETO_FLG_FOUND:0);
      pData ++;
      hb_snprintf( (char*) pData, ulLenData-1, "%lu;", ulRecNo );
      pData += strlen( (char*) pData );

      SELF_GETREC( pArea, &pRecord );

#ifdef ZIP_RECORD

      memcpy( pData, pRecord, uiRecordLen );
      pData += uiRecordLen;

#else

      for( ui = 0; ui < uiFieldCount; ui++ )
      {
         pField = pArea->lpFields + ui;
         ptr = leto_FieldOffset( pArea, (char *) pRecord, ui );

         ptrEnd = ptr + pField->uiLen - 1;
         if( pArea->fEof )
         {
            *pData = '\0';
            pData ++;
         }
         else
         {
            switch( pField->uiType )
            {
               case HB_FT_STRING:
                  while( ptrEnd > ptr && *ptrEnd == ' ' ) ptrEnd --;
                  ulRealLen = ptrEnd - ptr + 1;
                  if( ulRealLen == 1 && *ptr == ' ' )
                     ulRealLen = 0;
                  // Trimmed field length
                  if( pField->uiLen < 256 )
                  {
                     pData[0] = (BYTE) ulRealLen & 0xFF;
                     uiLen = 1;
                  }
                  else
                  {
                     uiLen = leto_n2b( pData+1, ulRealLen );
                     pData[0] = (BYTE) uiLen & 0xFF;
                     uiLen ++;
                  }
                  pData += uiLen;
                  if( ulRealLen > 0 )
                     memcpy( pData, ptr, ulRealLen );
                  pData += ulRealLen;
                  break;

               case HB_FT_LONG:
               case HB_FT_FLOAT:
                  while( ptrEnd > ptr && *ptr == ' ' ) ptr ++;
                  ulRealLen = ptrEnd - ptr + 1;
                  while( ptrEnd > ptr && ( *ptrEnd == '0' || *ptrEnd == '.' ) ) ptrEnd --;
                  if( *ptrEnd == '0' || *ptrEnd == '.' )
                     *pData++ = '\0';
                  else
                  {
                     *pData++ = (BYTE) ulRealLen & 0xFF;
                     memcpy( pData, ptr, ulRealLen );
                     pData += ulRealLen;
                  }
                  break;

               case HB_FT_DATE:
               {
                  if( *ptr <= ' ' && pField->uiLen == 8 )
                     *pData++ = '\0';
                  else
                  {
                     memcpy( pData, ptr, pField->uiLen );
                     pData += pField->uiLen;
                  }
                  break;
               }
               case HB_FT_LOGICAL:
                  *pData++ = *ptr;
                  break;
               case HB_FT_MEMO:
               case HB_FT_BLOB:
               case HB_FT_PICTURE:
               case HB_FT_OLE:
               {
                  ULONG ulBlock = 0;

                  if( pField->uiLen == 4 )
                     ulBlock =  HB_GET_LE_UINT32( ptr );
                  else
                  {
                     BYTE bByte;
                     USHORT uiCount;

                     for( uiCount = 0; uiCount < 10; uiCount++ )
                     {
                        bByte = ptr[ uiCount ];
                        if( bByte >= '0' && bByte <= '9' )
                           ulBlock = ulBlock * 10 + ( bByte - '0' );
                     }
                  }
//                  SELF_GETVALUE( (AREAP)pArea, ui+1, pItem );
//                  if( hb_itemGetCLen( pItem ) )
                  if( ulBlock )
                     *pData++ = '!';
                  else
                     *pData++ = '\0';
//                  hb_itemClear( pItem );
                  break;
               }
               // binary fields
               case HB_FT_INTEGER:
               case HB_FT_CURRENCY:
               case HB_FT_DOUBLE:
               case HB_FT_DATETIME:
               case HB_FT_MODTIME:
               case HB_FT_DAYTIME:
               case HB_FT_ROWVER:
               case HB_FT_AUTOINC:
                  memcpy( pData, ptr, pField->uiLen );
                  pData += pField->uiLen;
                  break;

               case HB_FT_ANY:
                  SELF_GETVALUE( pArea, ui+1, pItem );
                  if( pField->uiLen == 3 || pField->uiLen == 4 )
                  {
                     memcpy( pData, ptr, pField->uiLen );
                     pData += pField->uiLen;
                  }
                  else if( HB_IS_LOGICAL( pItem ) )
                  {
                     *pData++ = 'L';
                     *pData++ = (hb_itemGetL( pItem ) ? 'T' : 'F' );
                  }
                  else if( HB_IS_DATE( pItem ) )
                  {
                     *pData++ = 'D';
                     hb_itemGetDS( pItem, (char *) pData);
                     pData += 8;
                  }
                  else if( HB_IS_STRING( pItem ) )
                  {
                     uiLen = (USHORT) hb_itemGetCLen( pItem );
                     if( uiLen <= pField->uiLen - 3 )
                     {
                        *pData++ = 'C';
                        *pData++ = (BYTE) uiLen & 0xFF;
                        *pData++ = (BYTE) (uiLen >> 8) & 0xFF;
                        memcpy( pData, hb_itemGetCPtr( pItem ), uiLen );
                        pData += uiLen;
                     }
                     else
                     {
                        *pData++ = '!';
                     }
                  }
                  else if( HB_IS_NUMERIC( pItem ) )
                  {
                     char * szString = hb_itemStr( pItem, NULL, NULL );
                     char * szTemp;
                     *pData++ = 'N';

                     if( szString )
                     {
                        szTemp = szString;
                        while( HB_ISSPACE( *szTemp ) )
                           szTemp ++;
                        uiLen = strlen( szTemp );
                        memcpy( pData, szTemp, uiLen );
                        pData += uiLen;
                        hb_xfree( szString );
                     }
                     *pData++ = '\0';
                  }
                  else
                  {
                     *pData++ = 'U';
                  }
                  hb_itemClear( pItem );
                  break;
            }
         }
      }

#endif
      hb_snprintf( (char*) pData, ulLenData - ( pData - pData1 ), ";%lu;", ulRecCount );
      pData += strlen( pData );

      if( leto_CheckClientVer( pUStru, 213 ) )
      {
         ULONG ulKeyNo    = pUStru->bBufKeyNo    ? leto_GetOrdInfoNL( pArea, DBOI_POSITION ) : 0;
         ULONG ulKeyCount = pUStru->bBufKeyCount ? leto_GetOrdInfoNL( pArea, DBOI_KEYCOUNT ) : 0;
         hb_snprintf( (char*) pData, ulLenData - ( pData - pData1 ), "%lu;%lu;", ulKeyNo, ulKeyCount );
         pData += strlen( pData );
      }

      if( bCryptTraf )
      {
         ulRealLen = pData - ( char * ) pUStru->pBufCrypt;
         leto_encrypt( ( const char * ) pUStru->pBufCrypt, ulRealLen, szData + SHIFT_FOR_LEN, &ulRealLen, LETO_PASSWORD );
      }
      else
         ulRealLen = pData - szData - SHIFT_FOR_LEN;

      HB_PUT_LE_UINT24( szData, ( UINT32 ) ulRealLen );
      ulRealLen += SHIFT_FOR_LEN;
      hb_timeStampGet( &pAStru->lReadDate, &pAStru->lReadTime );
   }
   hb_itemRelease( pItem );

   return ulRealLen;
}

static ULONG leto_recLen( AREAP pArea )
{
/*
   USHORT uiTags = 0;
   LETOTAG *pTag = pAStru->pTag;

   while( pTag )
   {
      uiTags ++;
      pTag = pTag->pNext;
   }
 + uiTags*24
*/
   return leto_RecordLen( pArea ) + leto_FieldCount( pArea ) * 3 + 24 + 12 + 24;
}

HB_FUNC( LETO_REC )  // mt
{
   int iUserStru = hb_parni( 1 );
   AREAP pArea = hb_rddGetCurrentWorkAreaPointer();
   char * szData;
   USHORT uiRealLen;
   PUSERSTRU pUStru = s_users + iUserStru;
   PAREASTRU pAStru = pUStru->pCurAStru;
   ULONG ulRecLen;

   if( iUserStru < 0 || !pUStru->pCurAStru )
   {
      leto_wUsLog(NULL,"ERROR! LETO_REC!\r\n", 0);
      hb_retc( "" );
      return;
   }

                           // 3(len) + 1(flags) + 11(recno;) + uiFieldCount * { 3(len) + pField->uiLen(field) }
   ulRecLen = leto_recLen( pArea );
   szData = (char*) malloc( ulRecLen );

   uiRealLen = (USHORT) leto_rec( pUStru, pAStru, pArea, szData, ulRecLen );

   hb_retclen( szData, uiRealLen );
   free( szData );
}

static void leto_UnlockAllRec( PAREASTRU pAStru )  // mt
{
   PTABLESTRU pTStru;
   AREAP pArea = NULL;

   if( !pAStru )
   {
      leto_wUsLog(NULL,"ERROR! leto_UnlockAllRec!\r\n", 0);
      return;
   }

   pTStru = pAStru->pTStru;

   if( !pTStru )
   {
      leto_wUsLog(NULL,"ERROR! leto_UnlockAllRec!!\r\n", 0);
      return;
   }

   if( pAStru->LocksList.pItem )
   {
      PHB_ITEM pItem = NULL;
      PLETO_LOCK_ITEM pLockA;

      if( bShareTables )
      {
         pArea = hb_rddGetCurrentWorkAreaPointer();
         pItem = hb_itemPutNL( NULL, 0 );
      }
      for( pLockA = ( PLETO_LOCK_ITEM ) pAStru->LocksList.pItem; pLockA; pLockA = pLockA->pNext )
      {
        if( bShareTables )
        {
           hb_itemPutNL( pItem, pLockA->ulRecNo );
           SELF_UNLOCK( pArea, pItem );
        }
        letoDelRecFromList( &pTStru->LocksList, pLockA->ulRecNo );
      }
      letoClearList( &pAStru->LocksList );

      if( bShareTables )
         hb_itemRelease( pItem );
   }
}

// PRG -> C  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
/*

#if defined( HB_OS_LINUX ) || defined( HB_OS_BSD )
   #define DEF_SEP      '/'
   #define DEF_CH_SEP   '\'
#else
   #define DEF_SEP      '\'
   #define DEF_CH_SEP   '/'
#endif

static int leto_LastSep( char * szFile)  // mt
{
   char * ptrEnd;
   int i = 0, iLast = -1;
   for( ptrEnd = szFile; *ptrEnd; ++ptrEnd,++i )
      if( *ptrEnd == HB_OS_PATH_DELIM_CHR )
         iLast = i;
}


void leto_OpenIndex( PUSERSTRU pUStru, const char* szData )  // mt
{
   if( !pUStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_OpenIndex!", 0);
      return;
   }

   char * pData = NULL, * cBagName, * szData1 = NULL;
   char * cCurBagName = NULL;
   int nParam = leto_GetParam( szData, &cBagName, NULL, NULL, NULL );

   if( nParam < 2 )
      pData = szErr2;
   else
   {
      AREAP pArea = hb_rddGetCurrentWorkAreaPointer();

      if( pArea )
      {
         char cPath[_POSIX_PATH_MAX];
         PAREASTRU pAStru = pUStru->pCurAStru;
         int nLen;

         leto_NativeSep( cBagName );
         strcpy( cPath, pBagName );
         hb_strLower( pPath, 5 );
         if( strcmp( pPath, "/mem:" ) == 0 )
         {
            cBagName ++;
            cPath[ 0 ] = '\0';
         }
         else
         {
            if( pDataPath )
               strcpy( cPath, pDataPath );
            else
               cPath[ 0 ] = '\0';
         }
         nLen = strlen( cPath );

         if( strchr( cBagName, DEF_SEP ) != NULL )
         {
            if( ( nLen > 0 ) && ( *cBagName != DEF_SEP ) )
            {
               cPath[nLen] = DEF_SEP;
               ++ nLen;
            }
         }
         else
         {
            int nSep;
            if( ( nSep = leto_LastSep( pAStru->pTStru->szTable ) ) >= 0 )
            {
               ++ nSep;
               memcpy( cPath+nLen, pAStru->pTStru->szTable, nSep );
               nLen += nSep;
            }
         }
         strcpy( cPath + nLen, cBagName );

         hb_xvmSeqBegin();

         DBORDERINFO pOrderInfo;
         HB_ERRCODE errCode;

         hb_rddSetNetErr( HB_FALSE );
         memset( &pOrderInfo, 0, sizeof( pOrderInfo ) );
         pOrderInfo.atomBagName = hb_itemPutC( NULL, cPath );
         pOrderInfo.itmResult   = hb_itemPutNI( NULL, 0 );

         errCode = SELF_ORDLSTADD( pArea, &pOrderInfo );
         if( errCode == HB_SUCCESS )
         {
            pOrderInfo.itmOrder = hb_itemPutNI( NULL, hb_setGetAutOrder() );
            errCode = SELF_ORDLSTFOCUS( pArea, &pOrderInfo );
            hb_itemRelease( pOrderInfo.itmOrder );
            pOrderInfo.itmOrder = NULL;
            if( errCode == HB_SUCCESS )
               errCode = SELF_GOTOP( pArea );
         }
         hb_itemRelease( pOrderInfo.atomBagName );
         hb_itemRelease( pOrderInfo.itmResult );

         if( errCode != HB_SUCCESS || pUStru->bHrbError )
         {
            szData1 = leto_ErrorRT( EG_CORRUPTION, EDBF_CORRUPT, 0, 0, cBagName );
            pData = szData1;
         }
         else
         {
            cCurBagName = leto_GetOrdBagName( pArea, 0 );
            char * szIndexInfo = leto_IndexesInfo( pUStru, cCurBagName );

            int nIndexStru = leto_InitIndex( pUStru, cNextBagName, strlen(cNextBagName), cPath, strlen(cPath) );

            // return list of INDEX
            int iIdxInfoLen = 0, iIdxInfoBlock = _POSIX_PATH_MAX + LETO_MAX_TAGNAME + LETO_MAX_EXP*2 + 30;
            int iIdxInfoStart = 0;
            char * pIdxInfo = NULL;
            int nOrder = 1, nCount = 0;
            char * szOrdKey;
            ULONG uiRealLen;

            // DO WHILE !Empty( cName := OrdKey( nOrder ) )
            while( 1 )
            {
               char * cNextBagName;
               szOrdKey = leto_GetOrdKey( pArea, nOrder );
               if( ! szOrdKey )
                  break;
               // IF cCurBagName == OrdBagName( nOrder )
               cNextBagName = leto_GetOrdBagName( pArea, nOrder );
               if( !strcmp( cCurBagName, cNextBagName ) )
               {
                  // nCount ++
                  // IF nCount == 1
                  //   nIndexStru := leto_InitIndex( nUserStru, OrdBagName( nOrder ), cBagPath )
                  ++ nCount;
                  if( nCount == 1 )
                     nIndexStru = leto_InitIndex( pUStru, cNextBagName, strlen(cNextBagName), cPath, strlen(cPath) );

                  // cIndex += Leto_IndexInfo( nOrder, cName )
                  if( pIdxInfo )
                     pIdxInfo = hb_xrealloc( pIdxInfo, iIdxInfoLen + iIdxInfoBlock );
                  else
                     pIdxInfo = hb_xgrab( iIdxInfoLen + iIdxInfoBlock );
                  leto_IndexInfo( pIdxInfo + iIdxInfoStart, iIdxInfoBlock, nOrder, szOrdKey );
                  iIdxInfoStart += strlen( pIdxInfo + iIdxInfoStart );
                  iIdxInfoLen += iIdxInfoBlock;

                  // leto_addTag( nUserStru, nIndexStru, Lower( OrdName(nOrder ) ) )
                  memset( &pOrderInfo, 0, sizeof( pOrderInfo ) );
                  pOrderInfo.itmOrder = hb_itemPutNI( NULL, iOrder );
                  pOrderInfo.itmResult = hb_itemPutC( NULL, NULL );
                  SELF_ORDINFO( pArea, DBOI_NAME, &pOrderInfo );
                  leto_AddTag( pUStru, nIndexStru, hb_itemGetCPtr( pOrderInfo.itmResult ) );
                  hb_itemRelease( pOrderInfo.itmResult );
                  hb_itemRelease( pOrderInfo.itmOrder );
               }
               hb_xfree( cNextBagName );
               hb_xfree( szOrdKey );
               ++ nOrder;
            }

            //         "+" + LTrim( Str( nCount ) ) + ";" + Leto_IndexInfo( nOrder, cName ) + leto_rec( nUserStru )
            uiRealLen = 1 + 5 + 1 + iIdxInfoStart + leto_recLen( pArea );
            szData1 = (char*) hb_xgrab( uiRealLen );

            // cReply := "+" + LTrim( Str( nCount ) ) + ";"
            hb_snprintf( szData1, 7, "+%d;", nCount );
            // cReply += cIndex
            pData = szData1 + strlen(szData1);
            memcpy( pData, pIdxInfo, iIdxInfoStart );
            // cReply += leto_rec( nUserStru )
            uiRealLen = leto_rec( pAStru, pArea, pData + iIdxInfoStart, uiRealLen );

            if( uiRealLen <= 0 )
               pData = szErr3;
            else
            {
               leto_SendAnswer( pUStru, szData1, pData - szData1 + iIdxInfoStart + uiRealLen );
               pData = NULL;
            }

            if( pIdxInfo )
               hb_xfree( pIdxInfo );
         }

         hb_xvmSeqEnd();
      }
   }

   if( pData )
      leto_SendAnswer( pUStru, pData, strlen(pData) );
   pUStru->bAnswerSent = 1;
   if( szData1 )
      hb_xfree( szData1 );
   if( cCurBagName )
      hb_xfree( cCurBagName );
}
*/

char * leto_GetOrdKey( AREAP pArea, int iOrder )
{
   DBORDERINFO pOrderInfo;
   char * szOrdKey = NULL;
   int iLen;

   memset( &pOrderInfo, 0, sizeof( pOrderInfo ) );
   pOrderInfo.itmOrder = hb_itemPutNI( NULL, iOrder );
   pOrderInfo.itmResult = hb_itemPutC( NULL, NULL );
   SELF_ORDINFO( pArea, DBOI_EXPRESSION, &pOrderInfo );
   if( pOrderInfo.itmResult && HB_IS_STRING( pOrderInfo.itmResult ) )
   {
      if( ( iLen = hb_itemGetCLen( pOrderInfo.itmResult ) ) > 0 )
      {
         szOrdKey = hb_xgrab( iLen + 1 );
         memcpy( szOrdKey, hb_itemGetCPtr(pOrderInfo.itmResult), iLen );
         szOrdKey[iLen] = '\0';
      }
   }
   hb_itemRelease( pOrderInfo.itmOrder );
   hb_itemRelease( pOrderInfo.itmResult );

   return szOrdKey;
}

char * leto_GetOrdBagName( AREAP pArea, int iOrder )
{
   DBORDERINFO pOrderInfo;
   char * szBagName = NULL;
   int iLen;

   memset( &pOrderInfo, 0, sizeof( pOrderInfo ) );
   if( iOrder > 0 )
      pOrderInfo.itmOrder = hb_itemPutNI( NULL, iOrder );
   pOrderInfo.itmResult = hb_itemPutC( NULL, NULL );
   SELF_ORDINFO( pArea, DBOI_BAGNAME, &pOrderInfo );
   if( pOrderInfo.itmResult && HB_IS_STRING( pOrderInfo.itmResult ) )
   {
      if( ( iLen = hb_itemGetCLen( pOrderInfo.itmResult ) ) > 0 )
      {
         szBagName = hb_xgrab( iLen + 1 );
         memcpy( szBagName, hb_itemGetCPtr( pOrderInfo.itmResult ), iLen );
         szBagName[iLen] = '\0';
      }
   }
   if( iOrder > 0 )
      hb_itemRelease( pOrderInfo.itmOrder );
   hb_itemRelease( pOrderInfo.itmResult );

   return szBagName;
}

int leto_InitIndex( PUSERSTRU pUStru, const char * ccBagName, const char * ccFullName )  // mt
{
   PAREASTRU pAStru;

   PTABLESTRU pTStru;
   PINDEXSTRU pIStru;
   char * szBagName;
   USHORT ui = 0;
   USHORT uiLen;
   BOOL bRes = 0;

   if( !pUStru )
   {
      leto_wUsLog(NULL,"ERROR! LETO_INITINDEX!\r\n", 0);
      return -1;
   }
   pAStru = pUStru->pCurAStru;
   if( !pAStru )
   {
      leto_wUsLog(NULL,"ERROR! LETO_INITINDEX!\r\n", 0);
      return -1;
   }

   pTStru = pAStru->pTStru;

   uiLen = strlen(ccBagName);
   szBagName = (char*) hb_xgrab( uiLen + 1 );
   memcpy( szBagName, ccBagName, uiLen );
   szBagName[uiLen] = '\0';
   hb_strLower( szBagName, (ULONG)uiLen );

   if( !pTStru || !uiLen )
   {
      leto_wUsLog(pUStru,"ERROR! LETO_INITINDEX!!\r\n", 0);
      return -1;
   }

   while( ui < pTStru->uiIndexCount && ( pIStru = ( PINDEXSTRU ) letoGetListItem( &pTStru->IndexList, ui ) ) != NULL )
   {
     if( pIStru->BagName && !strcmp( szBagName, pIStru->BagName ) )
     {
        LETOTAG * pTag = pAStru->pTag;
        while( pTag )
        {
           if( pTag->pIStru == pIStru )
           {
              bRes = 1;
              break;
           }
           pTag = pTag->pNext;
        }
        if( !bRes )
           pIStru->uiAreas ++;
        hb_xfree( szBagName );
        return ui;
     }
     ui ++;
   }

   pIStru = ( PINDEXSTRU ) letoAddToList( &pTStru->IndexList );
   pTStru->uiIndexCount ++;
   pIStru->uiAreas = 1;
   pIStru->BagName = szBagName;
   pIStru->bCompound = ( !pTStru->uiDriver && !leto_BagCheck( ( const char * ) pTStru->szTable, szBagName ) );
   if( ccFullName )
   {
      uiLen = strlen( ccFullName );
      szBagName = (char*) hb_xgrab( uiLen + 1 );
      memcpy( szBagName, ccFullName, uiLen );
      szBagName[uiLen] = '\0';
      pIStru->szFullName = szBagName;
   }
   uiIndexCurr ++;
   if( uiIndexCurr > uiIndexMax )
      uiIndexMax = uiIndexCurr;
   return ui;
}

HB_FUNC( LETO_INITINDEX )  // mt
{
   hb_retni( leto_InitIndex( s_users + hb_parni( 1 ), hb_parc(2), HB_ISNIL(3) ? hb_parc(3) : NULL ) );
}

char * leto_IndexesInfo( PUSERSTRU pUStru, const char * szBagName )
{
   AREAP pArea = hb_rddGetCurrentWorkAreaPointer();
   DBORDERINFO pOrderInfo;
   int nOrder = 1, nCount = 0, nIndexStru = 0;
   char * szOrdBagName = NULL, * szOrdKey, * szData1;
   // return list of INDEX
   int iIdxInfoLen = 0, iIdxInfoBlock = _POSIX_PATH_MAX + LETO_MAX_TAGNAME + LETO_MAX_EXP*2 + 30;
   int iIdxInfoStart = 0;
   ULONG uiRealLen;
   char * pIdxInfo = NULL;
   BOOL bIf;

   while( 1 )
   {
      szOrdKey = leto_GetOrdKey( pArea, nOrder );
      if( ! szOrdKey )
         break;
      if( szBagName == NULL )
         bIf = TRUE;
      else
      {
         szOrdBagName = leto_GetOrdBagName( pArea, nOrder );
         bIf = !strcmp( szBagName, szOrdBagName );
      }
      if( bIf )
      {
         nCount ++;
         if( nCount == 1)
         {
            if( szBagName )
               nIndexStru = leto_InitIndex( pUStru, szOrdBagName, szBagName );
            else
            {
               if( szOrdBagName )
                  hb_xfree( szOrdBagName );
               szOrdBagName = leto_GetOrdBagName( pArea, 1 );
               nIndexStru = leto_InitIndex( pUStru, szOrdBagName, NULL );
               hb_xfree( szOrdBagName );
               szOrdBagName = NULL;
            }
         }
         if( pIdxInfo )
            pIdxInfo = hb_xrealloc( pIdxInfo, iIdxInfoLen + iIdxInfoBlock );
         else
            pIdxInfo = hb_xgrab( iIdxInfoLen + iIdxInfoBlock );
         leto_IndexInfo( pIdxInfo + iIdxInfoStart, iIdxInfoBlock, nOrder, szOrdKey );
         iIdxInfoStart += strlen( pIdxInfo + iIdxInfoStart );
         iIdxInfoLen += iIdxInfoBlock;

         memset( &pOrderInfo, 0, sizeof( pOrderInfo ) );
         pOrderInfo.itmOrder = hb_itemPutNI( NULL, nOrder );
         pOrderInfo.itmResult = hb_itemPutC( NULL, NULL );
         SELF_ORDINFO( pArea, DBOI_NAME, &pOrderInfo );
         leto_AddTag( pUStru, nIndexStru, hb_itemGetCPtr( pOrderInfo.itmResult ) );
         hb_itemClear( pOrderInfo.itmResult );
         hb_itemRelease( pOrderInfo.itmResult );
         hb_itemRelease( pOrderInfo.itmOrder );
      }

      if( szOrdBagName )
         hb_xfree( szOrdBagName );
      hb_xfree( szOrdKey );
      nOrder ++;
   }

   uiRealLen = 1 + 5 + 1 + iIdxInfoStart;
   szData1 = (char*) hb_xgrab( uiRealLen );

   hb_snprintf( szData1, 7, "%d;", nCount );
   if( pIdxInfo )
   {
      char * pData = szData1 + strlen( szData1 );
      strcpy( pData, pIdxInfo );
      hb_xfree( pIdxInfo );
   }
   return szData1;
}

HB_FUNC( LETO_INDEXESINFO )
{
   hb_retc_buffer( leto_IndexesInfo( s_users + hb_parni( 1 ), hb_parc(2) ) );
}

void leto_CloseIndex( PINDEXSTRU pIStru )  // mt
{
   if( !pIStru )
   {
      leto_wUsLog(NULL,"ERROR! leto_CloseIndex!\r\n", 0);
      return;
   }

   if( pIStru->BagName )
   {
      hb_xfree( pIStru->BagName );
      pIStru->BagName = NULL;
   }
   if( pIStru->szFullName )
   {
      hb_xfree( pIStru->szFullName );
      pIStru->szFullName = NULL;
   }
   pIStru->uiAreas = 0;
   uiIndexCurr --;
}

int leto_FindTable( const char *szPar, int uiLen, ULONG * ulAreaID )  // mt
{
   char szFile[_POSIX_PATH_MAX + 1];
   PTABLESTRU pTStru;
   int ui;

   if( !szPar )
   {
      leto_wUsLog(NULL,"ERROR! leto_FindTable!\r\n", 0);
      return -1;
   }

   memcpy( szFile, szPar, uiLen );
   szFile[uiLen] = '\0';
   hb_strLower( (char*)szFile, uiLen );

   for( ui=0, pTStru=s_tables ; ui < uiTablesAlloc ; ++pTStru, ++ui )
   {
      if( pTStru->szTable && !strcmp( (char*)pTStru->szTable,szFile ) )
      {
// leto_wUsLog(NULL,"DEBUG! leto_FindTable! leto_RequestArea (FOUND):", 0);
// leto_wUsLog(NULL,szFile, 0);
// leto_wUsLog(NULL,(char*)pTStru->szTable, 0);
         *ulAreaID = pTStru->ulAreaID;
         return ui;
      }
   }
// leto_wUsLog(NULL,"DEBUG! leto_FindTable! leto_RequestArea (NOT FOUND):", 0);
// leto_wUsLog(NULL,szFile, 0);

   return -1;
}

HB_FUNC( LETO_FINDTABLE )  //mt
{
   ULONG ulAreaID = 0;
   int ui = -1;

   if( hb_parclen(1) <= 0 )
      leto_wUsLog(NULL,"ERROR! LETO_FINDTABLE!\r\n", 0);
   else
      ui = leto_FindTable( hb_parc(1), hb_parclen(1), &ulAreaID );

   hb_stornl( ulAreaID, 2 );
   hb_retni( ui );
}

void leto_CloseTable( int nTableStru )  //mt
{
   PTABLESTRU pTStru;
   PINDEXSTRU pIStru;
   USHORT ui;

   if( nTableStru < 0 || nTableStru >= uiTablesAlloc )
   {
      leto_wUsLog(NULL,"ERROR! leto_CloseTable!\r\n", 0);
      return;
   }

   pTStru = s_tables + nTableStru;

   leto_DelAreaID( pTStru->ulAreaID );

   if( pTStru->szTable )
   {
      hb_xfree( pTStru->szTable );
      pTStru->szTable = NULL;
   }
   letoListFree( &pTStru->LocksList );
   pTStru->bLocked = FALSE;
   ui = 0;
   if( pTStru->uiIndexCount )
   {
      while( ui < pTStru->uiIndexCount && ( pIStru = ( PINDEXSTRU ) letoGetListItem( &pTStru->IndexList, ui ) ) != NULL )
      {
         if( pIStru->BagName )
            leto_CloseIndex( pIStru );
         ui ++;
      }
      pTStru->uiIndexCount = 0;
   }
   letoListFree( &pTStru->IndexList );

   pTStru->ulAreaID = pTStru->uiAreas = 0;
   uiTablesCurr --;
}

int leto_InitTable( ULONG ulAreaID, const char * szName, USHORT uiDriver, BOOL bShared ) // hb_xrealloc not MT-safe
{
   PTABLESTRU pTStru = s_tables;
   USHORT ui = 0, uiRet;
   USHORT uiLen;

   if( !ulAreaID || !szName )
   {
      leto_wUsLog(NULL,"ERROR! leto_InitTable!\r\n", 0);
      return -1;
   }

   while( ui < uiTablesAlloc && pTStru->szTable )
   {
     pTStru ++;
     ui ++;
   }
   if( ui >= uiTablesAlloc )
   {
      leto_wUsLog(NULL,"WARNING! leto_InitTable! s_tables realloc\r\n", 0);
      s_tables = (TABLESTRU*) hb_xrealloc( s_tables, sizeof(TABLESTRU) * ( uiTablesAlloc+TABLES_REALLOC ) );
      memset( s_tables+uiTablesAlloc, 0, sizeof(TABLESTRU) * TABLES_REALLOC );
      pTStru = s_tables + uiTablesAlloc;
      uiTablesAlloc += TABLES_REALLOC;
   }
   pTStru->ulAreaID = ulAreaID;
   pTStru->uiAreas = 0;
   uiLen = (USHORT) strlen(szName);
   pTStru->szTable = (BYTE*) hb_xgrab( uiLen + 1 );
   memcpy( pTStru->szTable, szName, uiLen );
   pTStru->szTable[uiLen] = '\0';
   hb_strLower( (char*)pTStru->szTable, (ULONG)uiLen );
   pTStru->uiDriver = uiDriver;
   pTStru->bShared = bShared;
   letoListInit( &pTStru->LocksList, sizeof( ULONG ) );
   letoListInit( &pTStru->IndexList, sizeof( INDEXSTRU ) );
   pTStru->ulFlags = 0;

   uiRet = ui;
   if( ++ui > uiTablesMax )
      uiTablesMax = ui;
   uiTablesCurr ++;
   return uiRet;

}

HB_FUNC( LETO_INITTABLE )  //mt
{
   hb_retni( leto_InitTable( hb_parnl(1), hb_parc(2), hb_parni(3), hb_parl(4) ) );
}

/*
void leto_OpenTable( PUSERSTRU pUStru, const char* szData )
{
   const char * pData;
   USHORT ulLen = 4;
   char szFileName[_POSIX_PATH_MAX + 1];
   char szAlias[HB_RDD_MAX_ALIAS_LEN + 1];
   BOOL lShared, lReadonly;
   char * szCdPage;
   USHORT uiDriver;
   AREAP pArea;
   const char * pp1, * pp2, * pp3, *ptrend;
   int nParam = leto_GetParam( szData, &pp1, &pp2, &pp3, NULL );

   if( nParam < 3 )
      pData = szErr2;
   else
   {
      memcpy( szFileName, szData, pp1-szData-1 );
      szFileName[pp1-szData-1] = '\0';
      memcpy( szAlias, pp1, pp2-pp1-1 );
      szAlias[pp2-pp1-1] = '\0';
      lShared   = (pp2[0]=="T") ? TRUE : FALSE;
      lReadonly = (pp2[1]=="T") ? TRUE : FALSE;
      szCdPage = pp3;
      ptrend = strchr(szCdPage, ';');
      *ptrend = '\0';
//      PHB_CODEPAGE = hb_cdpFind(cdp)
      uiDriver = leto_getDriver( szFileName );

      if( szFileName[0] )
      {
         pArea  = pUStru->pArea;

         hb_xvmSeqBegin();
         hb_rddSetNetErr( FALSE );
         hb_rddOpenTable( szFileName, leto_Driver( uiDriver ), 0, szAlias,
                           bShareTables & lShared,
                           bShareTables & lReadonly,
                           szCdPage, 0, NULL, NULL );
         if( pUStru->bHrbError )
            !error;
         hb_xvmSeqEnd();
      }
      else
         pData = szErr2;

   leto_SendAnswer( pUStru, pData, ulLen );
   pUStru->bAnswerSent = 1;
}
*/

static void letoTagFree( LETOTAG * pTag )
{
   if( pTag->pTopScope )
      hb_itemRelease( pTag->pTopScope );
   if( pTag->pBottomScope )
      hb_itemRelease( pTag->pBottomScope );
   hb_xfree( pTag );
}

void leto_CloseArea( PAREASTRU pAStru )  // mt
{
   if( !pAStru )
   {
      leto_wUsLog(NULL,"ERROR! leto_CloseArea!\r\n", 0);
      return;
   }

   pAStru->pTStru->uiAreas --;
   if( !pAStru->pTStru->uiAreas )
   {
      /* If this was a last area for a table pAStru->pTStru, we need to close
         this table */
      if( pAStru->bNotDetach )
      {
         char szAlias[HB_RDD_MAX_ALIAS_LEN + 1];
         int iArea;

         leto_MakeAlias( pAStru->ulAreaID, szAlias );
         hb_rddGetAliasNumber( szAlias, &iArea );
         if( iArea > 0 )
         {
            hb_rddSelectWorkAreaNumber( iArea );
            hb_rddReleaseCurrentArea();
         }
         else
            leto_wUsLog(NULL,"ERROR! leto_CloseArea! hb_rddGetAliasNumber\r\n", 0);
      }
      else
      {
         if( leto_RequestArea( pAStru->ulAreaID ) )
            hb_rddReleaseCurrentArea();
      }
      leto_CloseTable( pAStru->pTStru - s_tables );
   }
   else
   {
      leto_UnlockAllRec( pAStru );
      if( pAStru->pTStru->bLocked && pAStru->bLocked )
      {
         pAStru->pTStru->bLocked = FALSE;
      }
      if( pAStru->bNotDetach )
         leto_DetachArea( pAStru->ulAreaID );
   }
   pAStru->ulAreaID   = 0;
   pAStru->pTStru     = NULL;
   pAStru->bLocked    = FALSE;
   pAStru->bNotDetach = FALSE;
   letoListFree( &pAStru->LocksList );
   if( pAStru->pTag )
   {
      LETOTAG * pTag = pAStru->pTag, * pTagNext;
      do
      {
         pTagNext = pTag->pNext;
         /* if( pTag->szTagName )
            hb_xfree( pTag->szTagName ); */
         letoTagFree( pTag );
         pTag = pTagNext;
      }
      while( pTag );
      pAStru->pTag = NULL;
   }
   if( pAStru->itmFltExpr )
   {
      hb_itemClear( pAStru->itmFltExpr );
      hb_itemRelease( pAStru->itmFltExpr );
      pAStru->itmFltExpr = NULL;
   }
#ifdef __BM
   if( pAStru->pBM )
   {
      hb_xfree( pAStru->pBM );
      pAStru->pBM = NULL;
   }
#endif
}

int leto_InitArea( int iUserStru, int iTableStru, ULONG ulAreaID, const char * szAlias, BOOL bRequest )  // mt
{
   PUSERSTRU pUStru;
   PAREASTRU pAStru, pAStruEmpty = NULL;
   PTABLESTRU pTStru;
   PLETO_LIST_ITEM pListItem;
   USHORT ui = 0;

   if( iUserStru < 0 || iUserStru >= uiUsersAlloc || iTableStru < 0 || iTableStru >= uiTablesAlloc || !ulAreaID || !szAlias )
   {
      leto_wUsLog(NULL,"ERROR! leto_InitArea!\r\n", 0);
      return -1;
   }

   pUStru  = s_users + iUserStru;

   pListItem = pUStru->AreasList.pItem;
   while( pListItem )
   {
      pAStru = ( PAREASTRU ) ( pListItem + 1 );
      if( !pAStru->ulAreaID )
      {
         pAStruEmpty = pAStru;
         break;
      }
      pListItem = pListItem->pNext;
   }
   if( pAStruEmpty )
      pAStru = pAStruEmpty;
   else
   {
      pAStru  = letoAddToList( &pUStru->AreasList );
      pUStru->uiAreasCount ++;
   }
   pTStru = s_tables + iTableStru;

   pTStru->uiAreas ++;

   pAStru->ulAreaID   = ulAreaID;
   pAStru->pTStru     = pTStru;
   pAStru->bNotDetach = ( bRequest ? FALSE : TRUE );
   pAStru->bUseBuffer = TRUE;
   strcpy( pAStru->szAlias, szAlias );
   hb_strLower( pAStru->szAlias, strlen(pAStru->szAlias) );
   letoListInit( &pAStru->LocksList, sizeof( ULONG ) );

   pUStru->ulCurAreaID = ulAreaID;
   pUStru->pCurAStru   = pAStru;

   if( bRequest )
   {
      if( !leto_SelectArea( pUStru, ulAreaID ) )
         return -1;
   }

   return ui;
}

HB_FUNC( LETO_INITAREA )  // mt
{
   hb_retni( leto_InitArea( hb_parni(1), hb_parni(2), hb_parnl(3), hb_parc(4), hb_parl(5) ) );
}

void leto_CloseUS( PUSERSTRU pUStru )  // mt
{
   PAREASTRU pAStru;
   PLETO_LIST_ITEM pListItem;

   if( !pUStru )
   {
      leto_wUsLog(pUStru,"ERROR! leto_CloseUS!\r\n", 0);
      return;
   }

   if( iDebugMode > 0 )
      leto_writelog( NULL,0,"DEBUG! close connect %s %s %s users=(%d : %d : %d), tables=(%d : %d)\r\n", 
            pUStru->szAddr, pUStru->szNetname, pUStru->szExename,
            pUStru->iUserStru, uiUsersCurr, uiUsersMax,
            uiTablesCurr, uiTablesMax );

   hb_threadEnterCriticalSection( &mutex_Open );

   if( pUStru->pBuffer )
   {
      hb_xfree( pUStru->pBuffer );
      pUStru->pBuffer = NULL;
   }
   if( pUStru->pSendBuffer )
   {
      hb_xfree( pUStru->pSendBuffer );
      pUStru->pSendBuffer = NULL;
   }
   if( pUStru->szAddr )
   {
      hb_xfree( pUStru->szAddr );
      pUStru->szAddr = NULL;
   }
   if( pUStru->szVersion )
   {
      hb_xfree( pUStru->szVersion );
      pUStru->szVersion = NULL;
   }
   if( pUStru->szNetname )
   {
      hb_xfree( pUStru->szNetname );
      pUStru->szNetname = NULL;
   }
   if( pUStru->szExename )
   {
      hb_xfree( pUStru->szExename );
      pUStru->szExename = NULL;
   }
   if( pUStru->szDateFormat )
   {
      hb_xfree( pUStru->szDateFormat );
      pUStru->szDateFormat = NULL;
   }

   pListItem = pUStru->AreasList.pItem;
   while( pListItem )
   {
      pAStru = ( PAREASTRU ) ( pListItem + 1 );
      if( pAStru->pTStru )
      {
         leto_CloseArea( pAStru );
      }
      pListItem = pListItem->pNext;
   }
   letoListFree( &pUStru->AreasList );
   pUStru->uiAreasCount = 0;

   if( pUStru->pBufCrypt )
   {
      hb_xfree( pUStru->pBufCrypt );
      pUStru->pBufCrypt = NULL;
      pUStru->ulBufCryptLen = 0;
   }
   if( pUStru == iUserLock + s_users && bLockLock )
   {
      bLockLock = FALSE;
   }

   leto_varsown_release( pUStru );

   memset( pUStru, 0, sizeof(USERSTRU) );

   uiUsersCurr --;

   hb_threadLeaveCriticalSection( &mutex_Open );
}

int leto_InitUS( HB_SOCKET hSocket )  // mt   hb_xrealloc - not MT-safe
{
   PUSERSTRU pUStru = s_users;
   int iUserStru = 0;

   while( iUserStru < uiUsersAlloc && pUStru->pBuffer )
   {
     pUStru ++;
     iUserStru ++;
   }
   if( iUserStru == uiUsersAlloc )
   {
      leto_wUsLog(NULL,"WARNING! leto_InitUS! s_users realloc\r\n", 0);
      s_users = (USERSTRU*) hb_xrealloc( s_users, sizeof(USERSTRU) * ( uiUsersAlloc+USERS_REALLOC ) );
      memset( s_users+uiUsersAlloc, 0, sizeof(USERSTRU) * USERS_REALLOC );
      pUStru = s_users + uiUsersAlloc;
      uiUsersAlloc += USERS_REALLOC;
   }
   pUStru->iUserStru   = iUserStru;
   pUStru->hSocket     = hSocket;
   pUStru->ulBufferLen = HB_SENDRECV_BUFFER_SIZE;
   pUStru->pBuffer = (BYTE*) hb_xgrab( pUStru->ulBufferLen );
   pUStru->ulSendBufferLen = HB_SENDRECV_BUFFER_SIZE;
   pUStru->pSendBuffer = (BYTE*) hb_xgrab( pUStru->ulSendBufferLen );

   letoListInit( &pUStru->AreasList, sizeof( AREASTRU ) );
   pUStru->uiAreasCount = 0;

   ++ uiUsersCurr;

   if( iUserStru+1 > uiUsersMax )
      uiUsersMax = iUserStru + 1;

   return iUserStru;
}

void leto_CloseAllSocket()
{
   PUSERSTRU pUStru = s_users;
   int iUserStru = 0;

   while( iUserStru < uiUsersAlloc )
   {
      if( pUStru->pBuffer && pUStru->hSocket != HB_NO_SOCKET )
         hb_socketClose( pUStru->hSocket );
      pUStru ++;
      iUserStru ++;
   }
}

HB_FUNC( LETO_ADDDATABASE )  // mt
{
   DATABASE * pDBNext, * pDB = (DATABASE*) hb_xgrab( sizeof(DATABASE) );
   USHORT uiLen = (USHORT) hb_parclen(1);

   memset( pDB, 0, sizeof(DATABASE) );
   pDB->szPath = (char*) hb_xgrab( uiLen+1 );
   memcpy( pDB->szPath, hb_parc(1), uiLen );
   pDB->szPath[uiLen] = '\0';
   hb_strLower( pDB->szPath, uiLen );

   pDB->uiDriver = hb_parni(2);

   if( !s_pDB )
      s_pDB = pDB;
   else
   {
      pDBNext = s_pDB;
      while( pDBNext->pNext )
         pDBNext = pDBNext->pNext;
      pDBNext->pNext = pDB;
   }

}

USHORT leto_getDriver( const char * szPath )  // mt
{
   unsigned int iLen, iLenPath;
   char * szLowerPath;
   USHORT uiRet = uiDriverDef;
   DATABASE * pDB = s_pDB;

   iLenPath = (USHORT) strlen(szPath);
   szLowerPath = (char*) hb_xgrab( iLenPath + 1 );
   memcpy( szLowerPath, szPath, iLenPath + 1 );
   hb_strLower( szLowerPath, (ULONG)iLenPath );

   while( pDB )
   {
      iLen = strlen( pDB->szPath );
      if( ( iLen <= iLenPath ) && ( !strncmp( pDB->szPath,szLowerPath,iLen ) ) )
      {
         uiRet = pDB->uiDriver;
         break;
      }
      pDB = pDB->pNext;
   }
   hb_xfree( szLowerPath );
   return uiRet;
}

HB_FUNC( LETO_GETDRIVER )  // mt
{
   USHORT uiDriver = leto_getDriver( hb_parc(1) );
   if( uiDriver )
      hb_retni( uiDriver );
   else
      hb_ret();
}

HB_FUNC( LETO_CREATEDATA )  // mt
{

   uiUsersAlloc = uiInitUsersMax;
   s_users = (USERSTRU*) hb_xgrab( sizeof(USERSTRU) * uiUsersAlloc );
   memset( s_users, 0, sizeof(USERSTRU) * uiUsersAlloc );

   uiTablesAlloc = uiInitTablesMax;
   s_tables = (TABLESTRU*) hb_xgrab( sizeof(TABLESTRU) * uiTablesAlloc );
   memset( s_tables, 0, sizeof(TABLESTRU) * uiTablesAlloc );

   lStartDate = leto_Date();
   dStartsec  = hb_dateSeconds();
   memset( dSumWait, 0, sizeof(dSumWait) );
   memset( uiSumWait, 0, sizeof(uiSumWait) );

   leto_writelog( NULL, 0, "%s%s ! INIT: DataPath=%s, ShareTables=%d, MaxUsers=%d, MaxTables=%d, CacheRecords=%d\r\n",
          szRelease, HB_LETO_VERSION_STRING, (pDataPath ? pDataPath : ""),
          bShareTables, uiInitUsersMax, uiTablesAlloc, uiCacheRecords );
}

HB_FUNC( LETO_RELEASEDATA )  // mt
{
   DATABASE * pDBNext, * pDB = s_pDB;
   USHORT ui = 0;

   PUSERSTRU pUStru = s_users;
   PTABLESTRU pTStru = s_tables;

   if( pUStru )
   {
      while( ui < uiUsersAlloc )
      {
         if( pUStru->pBuffer )
            leto_CloseUS( pUStru );
         ui ++;
         pUStru ++;
      }

      hb_xfree( s_users );
      s_users = NULL;
   }

   ui = 0;
   if( pTStru )
   {
      while( ui < uiTablesAlloc )
      {
         if( pTStru->szTable )
         {
            if ( leto_RequestArea( pTStru->ulAreaID ) )
               leto_CloseTable( ui );
         }
         ui ++;
         pTStru ++;
      }

      hb_xfree( s_tables );
      s_tables = NULL;
   }
   if( pDataPath )
   {
      hb_xfree( pDataPath );
      pDataPath = NULL;
   }
   if( pDB )
      while( pDB )
      {
         pDBNext = pDB->pNext;
         if( pDB->szPath )
            hb_xfree( pDB->szPath );
         hb_xfree( pDB );
         pDB = pDBNext;
      }
   leto_acc_flush( pAccPath );
   if( pAccPath )
   {
      hb_xfree( pAccPath );
      pAccPath = NULL;
   }
   leto_acc_release();
   leto_vars_release();

}

static void leto_GotoIf( AREAP pArea, ULONG ulRecNo )  // mt
{
   ULONG ulOldRecNo;

   if( !pArea )
   {
      leto_wUsLog(NULL,"ERROR! leto_GotoIf!\r\n", 0);
      return;
   }

   SELF_RECNO( pArea, &ulOldRecNo );
   if( ulOldRecNo != ulRecNo )
   {
      SELF_GOTO( pArea, ulRecNo );
   }
}

static void leto_NativeSep( char * szFile)  // mt
{
   char * ptrEnd;
#if defined( HB_OS_WIN_32 ) || defined( HB_OS_WIN )
   for( ptrEnd = szFile; *ptrEnd; ptrEnd++ )
      if( *ptrEnd == '/' )
         *ptrEnd = '\\';
#else
   for( ptrEnd = szFile; *ptrEnd; ptrEnd++ )
      if( *ptrEnd == '\\' )
         *ptrEnd = '/';
#endif
}

static void leto_DataPath( const char* szFilename, char* szBuffer )
{
   int iLen;
   const char * sSrc;

   szBuffer[0] = '\0';
   if( szFilename[0] )
   {
      if( pDataPath )
      {
         iLen = strlen( pDataPath );
         strcpy( szBuffer, pDataPath );
         if( iLen > 0 && szBuffer[iLen-1] != HB_OS_PATH_DELIM_CHR )
         {
            szBuffer[iLen++] = HB_OS_PATH_DELIM_CHR;
            szBuffer[iLen] = '\0';
         }
      }

      sSrc = szFilename;
      if( sSrc[1] == ':' && pDataPath )   // example: "c:\path"
         sSrc += 2;
      while( *sSrc == '\\' || *sSrc == '/' )
         ++ sSrc;

      strcat( szBuffer, sSrc );
      leto_NativeSep( szBuffer );
   }
}

ULONG leto_CryptText( PUSERSTRU pUStru, const char* pData, ULONG ulLen )
{
   ULONG ulBufLen = ulLen + 23;
   USHORT uiLen;

   if( ulBufLen > pUStru->ulBufCryptLen )
   {
      if( !pUStru->ulBufCryptLen )
         pUStru->pBufCrypt = (BYTE*) hb_xgrab( ulBufLen );
      else
         pUStru->pBufCrypt = (BYTE*) hb_xrealloc( pUStru->pBufCrypt, ulBufLen );
      pUStru->ulBufCryptLen = ulBufLen;
   }


   if( bCryptTraf && ulLen )
   {
      leto_encrypt( pData, ulLen, (char*) pUStru->pBufCrypt+4+2, &ulLen, LETO_PASSWORD );
      uiLen = leto_n2b( (char*) pUStru->pBufCrypt+1, ulLen + 1 );
      pUStru->pBufCrypt[0] = (BYTE) uiLen & 0xFF;
      if( uiLen < 4)
         memmove( pUStru->pBufCrypt+uiLen+2, pUStru->pBufCrypt+4+2, ulLen + 1 );
   }
   else
   {
      uiLen = leto_n2b( (char*) pUStru->pBufCrypt+1, ulLen + 1 );
      pUStru->pBufCrypt[0] = (BYTE) uiLen & 0xFF;
      memcpy( pUStru->pBufCrypt+uiLen+2, pData, ulLen );
   }

   ulLen += uiLen + 2;
   return ulLen;
}

static BOOL leto_WriteDeny( PUSERSTRU pUStru )
{
   return bPass4D && !( pUStru->szAccess[0] & 4 );
}

static void leto_Drop( PUSERSTRU pUStru, const char* szData )
{
   const char * pIFile;
   char szBuf[_POSIX_PATH_MAX + 1];
   int nParam = leto_GetParam( szData, &pIFile, NULL, NULL, NULL );

   if( nParam < 2 )
   {
      leto_SendAnswer( pUStru, szErr2, 4 );
   }
   else if( leto_WriteDeny( pUStru ) )
   {
      leto_SendAnswer( pUStru, szErrAcc, 4 );
   }
   else
   {
      LPRDDNODE  pRDDNode;
      HB_USHORT  uiRddID;
      const char * szDriver;
      PHB_ITEM pName1, pName2;

      szDriver = hb_rddDefaultDrv( NULL );
      pRDDNode = hb_rddFindNode( szDriver, &uiRddID );  /* find the RDDNODE */

      if( pRDDNode )
      {

         leto_DataPath( szData, szBuf );
         pName1 = hb_itemPutC( NULL, szBuf );

         leto_DataPath( pIFile, szBuf );
         pName2 = hb_itemPutC( NULL, szBuf );

         if( SELF_DROP( pRDDNode, pName1, pName2, 0 ) == HB_SUCCESS )
            leto_SendAnswer( pUStru, "+T;0;", 5 );
         else
            leto_SendAnswer( pUStru, "+F;0;", 5 );

         hb_itemRelease( pName1 );
         hb_itemRelease( pName2 );
      }
      else
         leto_SendAnswer( pUStru, szErr2, 4 );
   }
   pUStru->bAnswerSent = 1;
}

static void leto_Exists( PUSERSTRU pUStru, const char* szData )
{
   const char * pIFile;
   char szBuf[_POSIX_PATH_MAX + 1];
   int nParam = leto_GetParam( szData, &pIFile, NULL, NULL, NULL );

   if( nParam < 2 )
   {
      leto_SendAnswer( pUStru, szErr2, 4 );
   }
   else
   {
      LPRDDNODE  pRDDNode;
      HB_USHORT  uiRddID;
      const char * szDriver;
      PHB_ITEM pName1, pName2;

      szDriver = hb_rddDefaultDrv( NULL );
      pRDDNode = hb_rddFindNode( szDriver, &uiRddID );  /* find the RDDNODE */

      if( pRDDNode )
      {

         leto_DataPath( szData, szBuf );
         pName1 = hb_itemPutC( NULL, szBuf );

         leto_DataPath( pIFile, szBuf );
         pName2 = hb_itemPutC( NULL, szBuf );

         if( SELF_EXISTS( pRDDNode, pName1, pName2, 0 ) == HB_SUCCESS )
            leto_SendAnswer( pUStru, "+T;0;", 5 );
         else
            leto_SendAnswer( pUStru, "+F;0;", 5 );

         hb_itemRelease( pName1 );
         hb_itemRelease( pName2 );
      }
      else
         leto_SendAnswer( pUStru, szErr2, 4 );
   }
   pUStru->bAnswerSent = 1;
}

static void leto_Rename( PUSERSTRU pUStru, const char* szData )
{
   const char * pIFile, * pNewFile;
   char szBuf[_POSIX_PATH_MAX + 1];
   int nParam = leto_GetParam( szData, &pIFile, &pNewFile, NULL, NULL );

   if( nParam < 3 )
   {
      leto_SendAnswer( pUStru, szErr2, 4 );
   }
   else if( leto_WriteDeny( pUStru ) )
   {
      leto_SendAnswer( pUStru, szErrAcc, 4 );
   }
   else
   {
      LPRDDNODE  pRDDNode;
      HB_USHORT  uiRddID;
      const char * szDriver;
      PHB_ITEM pName1, pName2, pName3;

      szDriver = hb_rddDefaultDrv( NULL );
      pRDDNode = hb_rddFindNode( szDriver, &uiRddID );  /* find the RDDNODE */

      if( pRDDNode )
      {

         leto_DataPath( szData, szBuf );
         pName1 = hb_itemPutC( NULL, szBuf );

         leto_DataPath( pIFile, szBuf );
         pName2 = hb_itemPutC( NULL, szBuf );

         leto_DataPath( pNewFile, szBuf );
         pName3 = hb_itemPutC( NULL, szBuf );

         if( SELF_RENAME( pRDDNode, pName1, pName2, pName3, 0 ) == HB_SUCCESS )
            leto_SendAnswer( pUStru, "+T;0;", 5 );
         else
            leto_SendAnswer( pUStru, "+F;0;", 5 );

         hb_itemRelease( pName1 );
         hb_itemRelease( pName2 );
         hb_itemRelease( pName3 );
      }
      else
         leto_SendAnswer( pUStru, szErr2, 4 );
   }
   pUStru->bAnswerSent = 1;
}

static void hb_setSetDeleted( BOOL bDeleted )  // mt
{
   PHB_ITEM pItem = hb_itemNew( NULL );
   hb_itemPutL( pItem, bDeleted );
   hb_setSetItem( HB_SET_DELETED, pItem );
   hb_itemRelease( pItem );
}

static void hb_setSetDateFormat( char* szDateFormat )  // mt
{
   PHB_ITEM pItem = hb_itemNew( NULL );
   hb_itemPutC( pItem, szDateFormat );
   hb_setSetItem( HB_SET_DATEFORMAT, pItem );
   hb_itemRelease( pItem );
}

static void leto_SetUserEnv( PUSERSTRU pUStru )  // mt
{
   if( !pUStru )
   {
      leto_wUsLog(NULL,"ERROR! leto_SetUserEnv!\r\n", 0);
      return;
   }

   hb_setSetCentury( pUStru->bCentury );
   if( pUStru->cdpage )
   {
      hb_vmSetCDP( pUStru->cdpage );
   }
   if( pUStru->szDateFormat )
   {
      hb_setSetDateFormat( pUStru->szDateFormat );
   }
}

HB_FUNC( LETO_SETUSERENV )  // mt
{
   leto_SetUserEnv(s_users + hb_parni(1));
}

void leto_filef( PUSERSTRU pUStru, const char* szData )  // mt
{
   const char * pSrcFile, * pp2 = NULL, * pp3 = NULL;
   char szData1[16];
   char szFile[_POSIX_PATH_MAX + 1];
   int nParam = leto_GetParam( szData, &pSrcFile, &pp2, &pp3, NULL );

   if( !bFileFunc )
   {
      leto_SendAnswer( pUStru, "+F;100;", 7 );
   }
   else if( nParam < 2 || *(szData+2) != '\0' || !*pSrcFile ||
      ( strlen( pSrcFile ) >= 2 && pSrcFile[1] == ':' ) ||
      strstr( pSrcFile, ".." HB_OS_PATH_DELIM_CHR_STRING ) )
   {
      leto_SendAnswer( pUStru, szErr2, 4 );
   }
   else
   {
      ULONG ulLen = 0;
      char * pBuffer = NULL;
      BOOL bFreeBuf = FALSE;

      leto_DataPath( pSrcFile, szFile );
      leto_SetUserEnv( pUStru );

      if( *szData == '0' )
      {
         switch( *(szData+1) )
         {
            case '1':  // fexists
               hb_snprintf( szData1, 16, "+%s;", ( hb_spFile( szFile, NULL ) )? "T" : "F" );
               break;

            case '2':  // ferase
               if( leto_WriteDeny( pUStru ) )
               {
                  pBuffer = ( char * ) szErrAcc;
                  ulLen = 4;
               }
               else if( hb_fsDelete( szFile ) )
                  hb_snprintf( szData1, 16, "+T;0;" );
               else
                  hb_snprintf( szData1, 16, "+F;%d;",hb_fsError() );
               break;

            case '3':  // frename
            {
               if( nParam < 3 )
                  strcpy( szData1, szErr2 );
               else if( leto_WriteDeny( pUStru ) )
               {
                  pBuffer = ( char * ) szErrAcc;
                  ulLen = 4;
               }
               else
               {
                  char szDest[_POSIX_PATH_MAX + 1];

                  leto_DataPath( pp2, szDest );
                  if ( hb_fsRename( szFile, szDest ) )
                     hb_snprintf( szData1, 16, "+T;0;" );
                  else
                     hb_snprintf( szData1, 16, "+F;%d;",hb_fsError() );
               }
               break;
            }

            case '4':  // memoread
            {
               pBuffer = leto_memoread( szFile, &ulLen );
               if( pBuffer )
               {
                  ulLen = leto_CryptText( pUStru, pBuffer, ulLen );
                  hb_xfree( pBuffer );
                  pBuffer = (char*) pUStru->pBufCrypt;
               }
               else
                  strcpy( szData1, szErr2 );
               break;
            }

            case '5':  // MkDir
            {
               if( leto_WriteDeny( pUStru ) )
               {
                  pBuffer = ( char * ) szErrAcc;
                  ulLen = 4;
               }
               else if( hb_fsMkDir( szFile ) )
                  sprintf( szData1, "+T;0;" );
               else
                  sprintf( szData1, "+F;%d;",hb_fsError() );
               break;
            }

            case '6':  // direxists
               hb_snprintf( szData1, 16, "+%s;", ( hb_fsDirExists( szFile ) )? "T" : "F" );
               break;

            case '7':  // RmDir
            {
               if( leto_WriteDeny( pUStru ) )
               {
                  pBuffer = ( char * ) szErrAcc;
                  ulLen = 4;
               }
               else if( hb_fsRmDir( szFile ) )
                  sprintf( szData1, "+T;0;" );
               else
                  sprintf( szData1, "+F;%d;",hb_fsError() );
               break;
            }

            default:
               strcpy( szData1, szErr2 );
               break;
         }
      }
      else if( *szData == '1' )
      {
         switch( *(szData+1) )
         {
            case '0':  // FileRead
            {
               if( nParam < 4 )
                  strcpy( szData1, szErr2 );
               else
               {
                  ULONG ulStart;
                  sscanf( pp2, "%lu", &ulStart );
                  sscanf( pp3, "%lu", &ulLen );

                  pBuffer = ( char * ) hb_xgrab( 18 + ulLen );

                  bFreeBuf = TRUE;
                  if( pBuffer && leto_fileread( szFile, pBuffer+3+4, ulStart, &ulLen ) )
                  {
                     memcpy( pBuffer, "+T;", 3 );
                     pBuffer[ 3 + 4 + ulLen ] = '\r';
                     pBuffer[ 3 + 4 + ulLen + 1 ] = '\n';
                     HB_PUT_LE_UINT32( pBuffer + 3, ulLen );
                     ulLen += 3 + 4 + 2;
                  }
                  else
                  {
                     if( pBuffer )
                        hb_xfree( pBuffer );
                     pBuffer = NULL;
                     sprintf( szData1, "+F;%d;", hb_fsError() );
                  }
               }
               break;
            }

            case '1':  // FileSize
            {
               if( leto_filesize( szFile, &ulLen ) )
                  sprintf( szData1, "+T;%lu;", ulLen );
               else
                  sprintf( szData1, "+F;%d;", hb_fsError() );
               break;
            }

            case '2':  // Directory
            {
               if( nParam < 3 )
                  strcpy( szData1, szErr2 );
               else
               {
                  ulLen    = 3;
                  pBuffer  = ( char * ) hb_xgrab( ulLen + 1 );
                  strcpy( pBuffer, "+T;" );
                  bFreeBuf = TRUE;

                  if( ( pBuffer = leto_directory( szFile, pp2, pBuffer, &ulLen ) ) == NULL )
                  {
                     sprintf( szData1, "+F;%d;", hb_fsError() );
                     bFreeBuf = FALSE;
                  }
               }
               break;
            }

            case '3':  // memowrite
            {
               if( nParam < 4 )
                  strcpy( szData1, szErr2 );
               else if( leto_WriteDeny( pUStru ) )
               {
                  pBuffer = ( char * ) szErrAcc;
                  ulLen = 4;
               }
               else
               {
                  const char * pBuf = pp3 + strlen( pp3 ) + 1;
                  ulLen = HB_GET_LE_UINT32( pBuf );
                  if( leto_memowrite( szFile, pBuf + 4, ulLen ) )
                     sprintf( szData1, "+T;0;" );
                  else
                     sprintf( szData1, "+F;%d;",hb_fsError() );
               }
               break;
            }

            case '4':  // FileWrite
            {
               if( nParam < 4 )
                  strcpy( szData1, szErr2 );
               else if( leto_WriteDeny( pUStru ) )
               {
                  pBuffer = ( char * ) szErrAcc;
                  ulLen = 4;
               }
               else
               {
                  ULONG ulStart;
                  const char * pBuf;

                  sscanf( pp2, "%lu", &ulStart );
                  sscanf( pp3, "%lu", &ulLen );

                  pBuf = pp3 + strlen( pp3 ) + 1;
                  ulLen = HB_GET_LE_UINT32( pBuf );

                  if( leto_filewrite( szFile, pBuf+4, ulStart, ulLen ) )
                     sprintf( szData1, "+T;0;" );
                  else
                     sprintf( szData1, "+F;%d;",hb_fsError() );
               }
               break;
            }

            case '5':  // FileAttr
            {
               HB_FATTR ulAttr;
               HB_BOOL fResult;
               char sBuffer[16];
               const char * ptr;
               if( nParam == 3 && pp2 && pp2[0] != '-' )
               {
                  if( leto_WriteDeny( pUStru ) )
                  {
                     pBuffer = ( char * ) szErrAcc;
                     ulLen = 4;
                     break;
                  }
                  ulAttr = hb_fsAttrEncode( pp2 );
                  fResult = hb_fsSetAttr( szFile, ulAttr );
                  if( fResult )
                     ptr = pp2;
               }
               else
               {
                  fResult = hb_fsGetAttr( szFile, &ulAttr );
                  if( fResult )
                     ptr = hb_fsAttrDecode( ulAttr, sBuffer );
               }
               if( fResult )
                  sprintf( szData1, "+T;%s;", ptr );
               else
                  sprintf( szData1, "+F;%d;",hb_fsError() );
               break;
            }

            default:
               strcpy( szData1, szErr2 );
               break;
         }
      }
      else
         strcpy( szData1, szErr2 );

      if( pBuffer )
      {
         leto_SendAnswer( pUStru, pBuffer, ulLen );
         if( bFreeBuf )
            hb_xfree( pBuffer );
      }
      else
         leto_SendAnswer( pUStru, szData1, strlen(szData1) );
   }
   pUStru->bAnswerSent = 1;
}

static BOOL leto_IsServerLock( PUSERSTRU pUStru )
{
   return ( iUserLock != ( pUStru - s_users ) ) && bLockLock;
}

BOOL leto_ServerLock( PUSERSTRU pUStru, BOOL bLock )
{
   if( pUStru )
   {
      iUserLock = pUStru - s_users;
      bLockLock = bLock;
   }
   return bLockLock;
}

/*
   leto_IsRecLocked()
   Returns 0, if the record isn't locked, 1, if it is locked in given area
   and -1 if it is locked by other area.
 */
static int leto_IsRecLocked( PAREASTRU pAStru, ULONG ulRecNo )  // mt
{
   PTABLESTRU pTStru;

   if( !pAStru )
   {
      leto_wUsLog(NULL,"ERROR! leto_IsRecLocked!\r\n", 0);
      return -1;
   }

   pTStru = pAStru->pTStru;

   if( bNoSaveWA )
   {
      AREAP pArea;
      PHB_ITEM pInfo = hb_itemNew( NULL );
      PHB_ITEM pRecNo = hb_itemPutNL( NULL, ulRecNo );
      BOOL bLocked;

      pArea = hb_rddGetCurrentWorkAreaPointer();
      SELF_RECINFO( pArea, pRecNo, DBRI_LOCKED, pInfo );
      bLocked = hb_itemGetL( pInfo );
      hb_itemRelease( pRecNo );
      hb_itemRelease( pInfo );
      return bLocked;
   }
   else
   {
      if( letoIsRecInList( &pAStru->LocksList, ulRecNo ) )
         return 1;
      if( letoIsRecInList( &pTStru->LocksList, ulRecNo ) )
         return -1;
   }
   return 0;

}

void leto_IsRecLockedUS( PUSERSTRU pUStru, const char* szData )  // mt
{
   PAREASTRU pAStru;
   int nParam;

   if( !pUStru || !pUStru->pCurAStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_IsRecLockedUS!\r\n", 0);
      return;
   }

   pAStru = pUStru->pCurAStru;
   nParam = leto_GetParam( szData, NULL, NULL, NULL, NULL );

   if( nParam < 1 )
   {
      leto_SendAnswer( pUStru, szErr2, 4 );
   }
   else
   {
      ULONG ulRecNo;
      sscanf( szData, "%lu", &ulRecNo );
      if ( leto_IsRecLocked( pAStru, ulRecNo ) )
        leto_SendAnswer( pUStru, "+T;", 3 );
      else
        leto_SendAnswer( pUStru, "+F;", 3 );
   }
   pUStru->bAnswerSent = 1;
}

static BOOL leto_RecLock( PUSERSTRU pUStru, PAREASTRU pAStru, ULONG ulRecNo )  // mt
{
   BOOL bLocked;
   PTABLESTRU pTStru;

   if( !pAStru )
   {
      leto_wUsLog(NULL,"ERROR! leto_RecLock!\r\n", 0);
      return FALSE;
   }

   pTStru = pAStru->pTStru;

   if( leto_IsServerLock( pUStru ) )
   {
      return FALSE;
   }
   else if( bNoSaveWA )
   {
      // Simply lock the record with the standard RDD method
      AREAP pArea;
      DBLOCKINFO dbLockInfo;
      dbLockInfo.fResult = FALSE;
      dbLockInfo.itmRecID = hb_itemPutNL( NULL, ulRecNo );
      dbLockInfo.uiMethod = DBLM_MULTIPLE;

      pArea = hb_rddGetCurrentWorkAreaPointer();

      SELF_LOCK( pArea, &dbLockInfo );
      hb_itemRelease( dbLockInfo.itmRecID );

      return dbLockInfo.fResult;
   }
   else
   {
      /* Add a record number (ulRecNo) to the area's and table's
         locks lists (pAStru->pLocksPos) */
      /* Firstly, scanning the table's locks list */
      bLocked = letoIsRecInList( &pTStru->LocksList, ulRecNo );

      if( !bLocked )
      {
         /* if the record isn't locked, lock it! */
         if( bShareTables )
         {
            /* if we work in ShareTables mode, set the real lock with the standard RDD method */
            AREAP pArea;
            DBLOCKINFO dbLockInfo;
            dbLockInfo.fResult = FALSE;
            dbLockInfo.itmRecID = hb_itemPutNL( NULL, ulRecNo );
            dbLockInfo.uiMethod = DBLM_MULTIPLE;

            pArea = hb_rddGetCurrentWorkAreaPointer();

            SELF_LOCK( pArea, &dbLockInfo );
            hb_itemRelease( dbLockInfo.itmRecID );
            if( !dbLockInfo.fResult )
               return FALSE;
         }
         letoAddRecToList( &pTStru->LocksList, ulRecNo );
      }

      /* The record is locked already, we need to determine,
         is it locked by current user/area, or no */
      /* Secondly, scanning the area's locks list */
      if( letoIsRecInList( &pAStru->LocksList, ulRecNo ) )
         return TRUE;
      if( bLocked )
         /* The record is locked by another user/area, so
            we return an error */
         return FALSE;
      else
      {
         letoAddRecToList( &pAStru->LocksList, ulRecNo );
         return TRUE;
      }
   }
}

static void leto_RecUnlock( PAREASTRU pAStru, ULONG ulRecNo )  // mt
{
   PTABLESTRU pTStru = pAStru->pTStru;
   BOOL bLocked;
   if( bNoSaveWA )
   {
      // Simply unlock the record with the standard RDD method
      AREAP pArea;
      PHB_ITEM pItem = hb_itemPutNL( NULL, ulRecNo );

      pArea = hb_rddGetCurrentWorkAreaPointer();

      SELF_UNLOCK( pArea, pItem );
      hb_itemRelease( pItem );
   }
   else
   {
      /* Firstly, scanning the area's locks list */
      bLocked = letoDelRecFromList( &pAStru->LocksList, ulRecNo );
      if( bLocked )
         /* The record is locked, so we unlock it */
      {
         /* Secondly, scanning the table's locks list if the record was
            locked by the current user/area */
         letoDelRecFromList( &pTStru->LocksList, ulRecNo );
         if( bShareTables )
         {
            /* if we work in ShareTables mode, unlock with the standard RDD method */
            AREAP pArea;
            PHB_ITEM pItem = hb_itemPutNL( NULL, ulRecNo );

            pArea = hb_rddGetCurrentWorkAreaPointer();

            SELF_UNLOCK( pArea, pItem );
            hb_itemRelease( pItem );
         }
      }
   }
}

static BOOL leto_IsLocked( void )
{
   PTABLESTRU pTStru;
   int ui;
   BOOL bRet = FALSE;

   for( ui=0, pTStru=s_tables ; ui < uiTablesAlloc ; ++pTStru, ++ui )
   {
      if( pTStru->ulAreaID && ( pTStru->bLocked || ( pTStru->LocksList.pItem ) ) )
      {
         bRet = TRUE;
         break;
      }
   }
   return bRet;
}

static void leto_FlushAll( void )
{
   PTABLESTRU pTStru;
   int ui;
   DBFAREAP pArea;

   for( ui=0, pTStru=s_tables ; ui < uiTablesAlloc ; ++pTStru, ++ui )
   {
      pArea = ( pTStru->ulAreaID ? (DBFAREAP) leto_RequestArea( pTStru->ulAreaID ) : NULL );
      if( pArea )
      {
         if( pArea->fUpdateHeader || pArea->fDataFlush )
         {
            SELF_FLUSH( (AREAP) pArea );
         }
         else
         {
            hb_fileCommit( pArea->pDataFile );
            if( pArea->fHasMemo && pArea->pMemoFile )
               hb_fileCommit( pArea->pMemoFile );
         }
         leto_DetachArea( pTStru->ulAreaID );
      }
   }
}

static void hb_setHardCommit( BOOL bSet )
{
   PHB_ITEM pItem = hb_itemNew( NULL );
   hb_itemPutL( pItem, bSet );
   hb_setSetItem( HB_SET_HARDCOMMIT, pItem );
   hb_itemRelease( pItem );
}

void leto_TryLock( int iSecs )
{
   while( iSecs > 0 && leto_IsLocked() )
   {
      hb_idleSleep( 1.0 );
      iSecs --;
   }
   if( leto_IsLocked() )
      bLockLock = FALSE;
   else if( bOptimize )
   {
      hb_setHardCommit( TRUE );
      leto_FlushAll();
      hb_setHardCommit( FALSE );
   }
}

HB_FUNC( LETO_TABLELOCK )
{
   int iUserStru = hb_parni( 1 );
   BOOL bRet = FALSE;
   if( HB_ISNUM( 1 ) && ( iUserStru < uiUsersAlloc ) )
   {
      PUSERSTRU pUStru = s_users + iUserStru;
      if( pUStru->pCurAStru && ! leto_IsServerLock( pUStru ) )
      {
         PTABLESTRU pTStru = pUStru->pCurAStru->pTStru;
         ULONG ulFlag = 1 << (( HB_ISNUM( 2 ) ? hb_parni( 2 ) : 1 ) - 1 );
         ULONG ulSecs = ( HB_ISNUM( 2 ) ? hb_parni( 3 ) : 1 )*100;
         ULONG ulDeciSec = (ULONG) ( hb_dateSeconds() * 100 );
         while( pTStru->ulFlags & ulFlag )
         {
            if( ((ULONG)(hb_dateSeconds()*100) - ulDeciSec) > ulSecs )
               break;
         }
         if( ! (pTStru->ulFlags & ulFlag) )
         {
            bRet = TRUE;
            pTStru->ulFlags |= ulFlag;
         }
      }
   }
   hb_retl( bRet );
}

HB_FUNC( LETO_TABLEUNLOCK )
{
   int iUserStru = hb_parni( 1 );
   if( HB_ISNUM( 1 ) && ( iUserStru < uiUsersAlloc ) )
   {
      PUSERSTRU pUStru = s_users + iUserStru;
      if( pUStru->pCurAStru )
      {
         PTABLESTRU pTStru = pUStru->pCurAStru->pTStru;
         ULONG ulFlag = 1 << (( HB_ISNUM( 2 ) ? hb_parni( 2 ) : 1 ) - 1 );
         pTStru->ulFlags &= ~ulFlag;
      }
   }
   hb_ret();
}

HB_FUNC( LETO_RECLOCK )
{
   int iUserStru = hb_parni( 1 );
   if( HB_ISNUM( 1 ) && ( iUserStru < uiUsersAlloc ) )
   {
      PUSERSTRU pUStru = s_users + iUserStru;
      if( pUStru->pCurAStru )
      {
         PAREASTRU pAStru = pUStru->pCurAStru;
         ULONG ulRecNo;

         if( HB_ISNUM(2) )
            ulRecNo = hb_parnl( 2 );
         else
         {
            AREAP pArea = hb_rddGetCurrentWorkAreaPointer();
            SELF_RECNO( pArea, &ulRecNo );
         }
         hb_retl( leto_RecLock( pUStru, pAStru, ulRecNo ) );
      }
      else
         hb_retl( FALSE );
   }
   else
      hb_retl( FALSE );
}

HB_FUNC( LETO_RECLOCKLIST )
{
   int iUserStru = hb_parni( 1 );
   if( HB_ISNUM( 1 ) && ( iUserStru < uiUsersAlloc ) && HB_ISARRAY(2) )
   {
      PUSERSTRU pUStru = s_users + iUserStru;
      if( pUStru->pCurAStru )
      {
         PAREASTRU pAStru = pUStru->pCurAStru;
         PHB_ITEM pArray = hb_param( 2, HB_IT_ARRAY );
         HB_SIZE nSize = hb_arrayLen( pArray ), n, n2;
         BOOL bLocked = TRUE;

         for( n = 1; n <= nSize; ++n )
         {
            if( !leto_RecLock( pUStru, pAStru, hb_arrayGetNL( pArray, n ) ) )
            {
               for( n2 = 1; n2 < n; ++n2 )
                  leto_RecUnlock( pAStru, hb_arrayGetNL( pArray, n2 ) );
               bLocked = FALSE;
               break;
            }
         }

         hb_retl( bLocked );
      }
      else
         hb_retl( FALSE );
   }
   else
      hb_retl( FALSE );
}

static BOOL leto_Updated( PAREASTRU pAStru )
{
   PTABLESTRU pTStru = pAStru->pTStru;
   return pTStru->lWriteDate == pAStru->lReadDate ? pTStru->lWriteTime > pAStru->lReadTime : pTStru->lWriteDate > pAStru->lReadDate;
}

static void leto_SendRecWithOk( PUSERSTRU pUStru, PAREASTRU pAStru, ULONG ulRecNo )
{
   AREAP pArea = hb_rddGetCurrentWorkAreaPointer();
   char * szData;
   ULONG ulLen;

   leto_GotoIf( pArea, ulRecNo );
   ulLen = leto_recLen( pArea );
   szData = (char *) hb_xgrab( ulLen + 6 );

   memcpy( szData, szOk, 4);
   szData[4] = ';';
   ulLen = (USHORT) leto_rec( pUStru, pAStru, pArea, szData + 5, ulLen ) + 5;

   leto_SendAnswer( pUStru, szData, ulLen );
   hb_xfree( szData );
}

void leto_Lock( PUSERSTRU pUStru, const char* szData )  // mt
{
   const char * pRecNo;
   PAREASTRU pAStru;
   int nParam;

   if( !pUStru || !pUStru->pCurAStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Lock!\r\n", 0);
      return;
   }

   pAStru = pUStru->pCurAStru;
   nParam = leto_GetParam( szData, &pRecNo, NULL, NULL, NULL );

   if( nParam < 1 || *szData != '0' || *(szData+2) != '\0' || ( nParam < 2 && *(szData+1) == '1' ) )
      leto_SendAnswer( pUStru, szErr2, 4 );
   else
   {
      if( pAStru->pTStru->bLocked && !pAStru->bLocked )
         leto_SendAnswer( pUStru, szErr4, 4 );
      else
      {
         switch( *(szData+1) )
         {
            ULONG ulRecNo;
            case '1':
            {
               sscanf( pRecNo, "%lu", &ulRecNo );
               if ( leto_RecLock( pUStru, pAStru, ulRecNo ) )
               {
                  if( leto_CheckClientVer( pUStru, 213 ) && leto_Updated( pAStru ) )
                     leto_SendRecWithOk( pUStru, pAStru, ulRecNo );
                  else
                     leto_SendAnswer( pUStru, szOk, 4 );
               }
               else
                  leto_SendAnswer( pUStru, szErr4, 4 );
               break;
            }
            case '2':
            {
               PTABLESTRU pTStru = pAStru->pTStru;
               if( leto_IsServerLock( pUStru ) )
               {
                  leto_SendAnswer( pUStru, szErr2, 4 );
                  break;
               }
               else if( bNoSaveWA || ( !pAStru->bLocked && bShareTables ) )
               {
                  /* Lock the file with the standard RDD method */
                  AREAP pArea;
                  DBLOCKINFO dbLockInfo;
                  dbLockInfo.fResult = FALSE;
                  dbLockInfo.itmRecID = NULL;
                  dbLockInfo.uiMethod = DBLM_FILE;

                  pArea = hb_rddGetCurrentWorkAreaPointer();

                  SELF_LOCK( pArea, &dbLockInfo );
                  if( !dbLockInfo.fResult )
                  {
                     leto_SendAnswer( pUStru, szErr2, 4 );
                     break;
                  }
               }
               pTStru->bLocked = pAStru->bLocked = TRUE;
               if( leto_CheckClientVer( pUStru, 213 ) && leto_Updated( pAStru ) )
               {
                  sscanf( pRecNo, "%lu", &ulRecNo );
                  leto_SendRecWithOk( pUStru, pAStru, ulRecNo );
               }
               else
                  leto_SendAnswer( pUStru, szOk, 4 );
               break;
            }
            default:
            {
               leto_SendAnswer( pUStru, szErr2, 4 );
            }
         }
      }
   }
   pUStru->bAnswerSent = 1;
}

static BOOL leto_UnlockTable( PAREASTRU pAStru )
{
   leto_UnlockAllRec( pAStru );
   if( bNoSaveWA || ( pAStru->bLocked && bShareTables ) )
   {
      AREAP pArea;
      pArea = hb_rddGetCurrentWorkAreaPointer();
      SELF_UNLOCK( pArea, NULL );
   }
   if( pAStru->bLocked || !pAStru->pTStru->bLocked )
   {
      pAStru->pTStru->bLocked = pAStru->bLocked = FALSE;
      return TRUE;
   }
   else
      return FALSE;
}

HB_FUNC( LETO_RECUNLOCK )
{
   int iUserStru = hb_parni( 1 );
   if( HB_ISNUM( 1 ) && ( iUserStru < uiUsersAlloc ) )
   {
      PUSERSTRU pUStru = s_users + iUserStru;
      if( pUStru->pCurAStru )
      {
         PAREASTRU pAStru = pUStru->pCurAStru;
         ULONG ulRecNo;

         if( HB_ISNUM(2) )
            ulRecNo = hb_parnl( 2 );
         else
         {
            AREAP pArea = hb_rddGetCurrentWorkAreaPointer();
            SELF_RECNO( pArea, &ulRecNo );
         }
         leto_RecUnlock( pAStru, ulRecNo );
      }
   }
   hb_ret( );
}

void leto_Unlock( PUSERSTRU pUStru, const char* szData )  // mt
{
   PAREASTRU pAStru;
   const char * pRecNo;
   int nParam;

   if( !pUStru || !pUStru->pCurAStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Unlock!\r\n", 0);
      return;
   }

   pAStru = pUStru->pCurAStru;
   nParam = leto_GetParam( szData, &pRecNo, NULL, NULL, NULL );

   if( nParam < 1 || *szData != '0' || *(szData+2) != '\0' || ( nParam < 2 && *(szData+1) == '1' ) )
      leto_SendAnswer( pUStru, szErr2, 4 );
   else
   {
      switch( *(szData+1) )
      {
         case '1':
         /* Delete a record number (ulRecNo) from the area's and table's
            locks lists (pAStru->pLocksPos) */
         {
            ULONG ulRecNo;

            sscanf( pRecNo, "%lu", &ulRecNo );

            leto_RecUnlock( pAStru, ulRecNo );
            leto_SendAnswer( pUStru, szOk, 4 );
            break;
         }
         case '2':
         {
            /* Unlock table */
            if( leto_UnlockTable( pAStru ) )
            {
               leto_SendAnswer( pUStru, szOk, 4 );
            }
            else
            {
               leto_SendAnswer( pUStru, szErr4, 4 );
            }
            break;
         }
         default:
         {
            leto_SendAnswer( pUStru, szErr2, 4 );
         }
      }
   }
   pUStru->bAnswerSent = 1;
}

static int UpdateRec( PUSERSTRU pUStru, const char* szData, BOOL bAppend, ULONG * pRecNo, TRANSACTSTRU * pTA, BOOL bCommit, AREAP pArea )  // mt
{
   PAREASTRU pAStru;
   ULONG ulRecNo;
   int   iUpd, iRes = 0;
   BOOL  bDelete = FALSE;
   BOOL  bRecall = FALSE;
   const char * pUpd, * pDelRecall, * ptr;
   int nParam;

   if( !pUStru || !pUStru->pCurAStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! UpdateRec!\r\n", 0);
      return 2;
   }

   pAStru = pUStru->pCurAStru;
   nParam = leto_GetParam( szData, &pUpd, &pDelRecall, NULL, NULL );

   if( nParam < 3 )
      return 2;
   else
   {
      sscanf( szData, "%lu", &ulRecNo );        // Get a rec number or bUnlockAll(for append)
      sscanf( pUpd, "%d", &iUpd );              // Get a number of updated fields
      if( *pDelRecall == '1' )
         bDelete = TRUE;
      else if( *pDelRecall == '2' )
         bRecall = TRUE;
      ptr = pDelRecall + strlen(pDelRecall) + 1;

      if( !pArea )
         pArea = hb_rddGetCurrentWorkAreaPointer();

      if( pTA )
         pTA->pArea = pArea;
      else
         hb_xvmSeqBegin();
      if( bAppend )
      {
         if( ( !pAStru->bLocked && pAStru->pTStru->bLocked ) || leto_IsServerLock( pUStru ) )
         {
            if( !pTA )
               hb_xvmSeqEnd();
            return ELETO_UNLOCKED;
         }
         if( pTA )
            pTA->bAppend = 1;
         else
         {
            if( ulRecNo )
            {
               leto_UnlockAllRec( pAStru );
            }

            hb_rddSetNetErr( FALSE );
            SELF_APPEND( pArea, FALSE );
            SELF_RECNO( pArea, &ulRecNo );
            if( pRecNo )
               *pRecNo = ulRecNo;
            if( pUStru->bHrbError )
               iRes = 101;
            else if( ! pAStru->bLocked )
               leto_RecLock( pUStru, pAStru, ulRecNo );
         }
      }
      else
      {
         if( ( !pTA || ulRecNo ) && pAStru->pTStru->bShared && !pAStru->bLocked && leto_IsRecLocked( pAStru, ulRecNo ) != 1 )
         {
            /*  The table is opened in shared mode, but the record isn't locked */
            if( !pTA )
               hb_xvmSeqEnd();
            return ELETO_UNLOCKED;
         }
         else if( !pTA )
            leto_GotoIf( pArea, ulRecNo );
         else
            pTA->ulRecNo = ulRecNo;
      }
      if( bDelete || bRecall )
      {
         if( pTA )
            pTA->uiFlag = ( (bDelete)? 1 : 0 ) | ( (bRecall)? 2 : 0 );
         else
         {
            BOOL b;
            SELF_DELETED( pArea, &b );
            if( bDelete && !b )
               SELF_DELETE( pArea );
            else if( bRecall && b )
               SELF_RECALL( pArea );
            if( pUStru->bHrbError )
               iRes = 102;
         }
      }
      if( !iRes && iUpd )
      {
         int i;
         LPFIELD pField;
         USHORT uiField, uiRealLen, uiLenLen;
         int n255;
         PHB_ITEM pItem = hb_itemNew( NULL );
         HB_USHORT uiFieldCount = leto_FieldCount( pArea );

         if( pTA )
         {
            pTA->uiItems  = iUpd;
            pTA->puiIndex = (USHORT*) malloc( sizeof(USHORT)*iUpd );
            pTA->pItems   = (PHB_ITEM*) malloc( sizeof(PHB_ITEM)*iUpd );
         }

         n255 = ( uiFieldCount > 255 ? 2 : 1 );

         for( i=0; i<iUpd; i++ )
         {
            /*  Firstly, calculate the updated field number ( uiField ) */
            uiField = (USHORT) leto_b2n( ptr, n255 );
            ptr += n255;

            if( !uiField || uiField > uiFieldCount )
            {
               iRes = 3;
               break;
            }
            pField = pArea->lpFields + uiField - 1;
            switch( pField->uiType )
            {
               case HB_FT_STRING:
                  uiLenLen = ((int)*ptr) & 0xFF;
                  ptr ++;
                  if( pField->uiLen > 255 )
                  {
                     if( uiLenLen > 6 )
                     {
                        iRes = 3;
                        break;
                     }
                     uiRealLen = (USHORT) leto_b2n( ptr, uiLenLen );
                     ptr += uiLenLen;
                  }
                  else
                     uiRealLen = uiLenLen;
                  if( uiRealLen > pField->uiLen )
                  {
                     iRes = 3;
                     break;
                  }
                  hb_itemPutCL( pItem, (char*)ptr, uiRealLen );
                  ptr += uiRealLen;
                  break;

               case HB_FT_LONG:
               case HB_FT_FLOAT:
               {
                  HB_MAXINT lVal;
                  double dVal;
                  BOOL fDbl;

                  uiRealLen = ((int)*ptr) & 0xFF;
                  if( uiRealLen > pField->uiLen )
                  {
                     iRes = 3;
                     break;
                  }
                  ptr ++;
                  fDbl = hb_strnToNum( (const char *) ptr,
                                       uiRealLen, &lVal, &dVal );
                  if( pField->uiDec )
                     hb_itemPutNDLen( pItem, fDbl ? dVal : ( double ) lVal,
                                      ( int ) ( pField->uiLen - pField->uiDec - 1 ),
                                      ( int ) pField->uiDec );
                  else if( fDbl )
                     hb_itemPutNDLen( pItem, dVal, ( int ) pField->uiLen, 0 );
                  else
                     hb_itemPutNIntLen( pItem, lVal, ( int ) pField->uiLen );
                  ptr += uiRealLen;
                  break;
               }
               case HB_FT_INTEGER:
               {
                  switch( pField->uiLen )
                  {
                     case 2:
                        hb_itemPutNILen( pItem, ( int ) HB_GET_LE_INT16( ptr ), 6 );
                        break;
                     case 4:
                        hb_itemPutNIntLen( pItem, ( HB_LONG ) HB_GET_LE_INT32( ptr ), 10 );
                        break;
                     case 8:
#ifndef HB_LONG_LONG_OFF
                        hb_itemPutNIntLen( pItem, ( HB_LONG ) HB_GET_LE_INT64( ptr ), 20 );
#else
                        hb_itemPutNLen( pItem, ( double ) HB_GET_LE_INT64( ptr ), 20, 0 );
#endif
                     break;
                  }
                  ptr += pField->uiLen;
                  break;
               }

               case HB_FT_DOUBLE:
               {
                  hb_itemPutNDLen( pItem, HB_GET_LE_DOUBLE( ptr ),
                     20 - ( pField->uiDec > 0 ? ( pField->uiDec + 1 ) : 0 ),
                     ( int ) pField->uiDec );
                  ptr += pField->uiLen;
                  break;
               }

               case HB_FT_CURRENCY:
               {
/* to do */
                  ptr += pField->uiLen;
                  break;
               }
               case HB_FT_DATETIME:
               case HB_FT_MODTIME:
               case HB_FT_DAYTIME:
               {
                  hb_itemPutTDT( pItem, HB_GET_LE_INT32( ptr ), HB_GET_LE_INT32( ptr + 4 ) );
                  ptr += pField->uiLen;
                  break;
               }

               case HB_FT_DATE:
                  if( pField->uiLen == 8 )
                  {
                     char szBuffer[ 9 ];
                     memcpy( szBuffer, ptr, 8 );
                     szBuffer[ 8 ] = 0;
                     hb_itemPutDS( pItem, szBuffer );
                     ptr += 8;
                  }
                  else if( pField->uiLen == 4 )
                  {
                     hb_itemPutDL( pItem, HB_GET_LE_UINT32( ptr ) );
                     ptr += 4;
                  }
                  else  /*  pField->uiLen == 3 */
                  {
                     hb_itemPutDL( pItem, HB_GET_LE_UINT24( ptr ) );
                     ptr += 3;
                  }
                  break;

               case HB_FT_LOGICAL:
                  hb_itemPutL( pItem, ( *ptr == 'T' ) );
                  ptr ++;
                  break;

               case HB_FT_ANY:
               {
                  if( pField->uiLen == 3 )
                  {
                     hb_itemPutDL( pItem, HB_GET_LE_UINT24( ptr ) );
                     ptr += 3;
                  }
                  else if( pField->uiLen == 4 )
                  {
                     hb_itemPutNL( pItem, HB_GET_LE_UINT32( ptr ) );
                     ptr += 4;
                  }
                  else
                  {
                     switch( *ptr++ )
                     {
                        case 'D':
                        {
                           char szBuffer[ 9 ];
                           memcpy( szBuffer, ptr, 8 );
                           szBuffer[ 8 ] = 0;
                           hb_itemPutDS( pItem, szBuffer );
                           ptr += 8;
                           break;
                        }
                        case 'L':
                        {
                           hb_itemPutL( pItem, (*ptr == 'T') );
                           ptr += 8;
                           break;
                        }
                        case 'N':
                        {
                           /* to do */
                           break;
                        }
                        case 'C':
                        {
                           USHORT uiLen = (USHORT) leto_b2n( ptr, 2 );
                           ptr += 2;
                           hb_itemPutCL( pItem, (char*)ptr, uiLen );
                           ptr += uiLen;
                           break;
                        }
                     }
                  }
                  break;
               }
            }
            if( iRes )
               break;
            if( pTA )
            {
               pTA->puiIndex[i] = uiField;
               pTA->pItems[i]   = hb_itemNew( pItem );
               hb_itemClear( pItem );
            }
            else
            {
               SELF_PUTVALUE( pArea, uiField, pItem );
               hb_itemClear( pItem );
               if( pUStru->bHrbError )
               {
                  iRes = 102;
                  break;
               }
            }
            hb_timeStampGet( &pAStru->pTStru->lWriteDate, &pAStru->pTStru->lWriteTime );
            hb_timeStampGet( &pAStru->lReadDate, &pAStru->lReadTime );
         }
         hb_itemRelease( pItem );
      }
      if( !pTA )
         hb_xvmSeqEnd();

      if(! iRes && bCommit )
      {
         hb_xvmSeqBegin();
         SELF_FLUSH( pArea );
         hb_xvmSeqEnd();
         pAStru->bUseBuffer = FALSE;
         if( pUStru->bHrbError )
            iRes = 101;
         else
         {
            leto_UnlockTable( pAStru );
         }
      }

      return iRes;
   }
}

void leto_UpdateRec( PUSERSTRU pUStru, const char* szData, BOOL bAppend, BOOL bCommit )  // mt
{
   const char * pData;
   char szData1[24];
   ULONG ulRecNo = 0;
   int iRes;

   if( !pUStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_UpdateRec!\r\n", 0);
      return;
   }

   if( leto_WriteDeny( pUStru ) )
      pData = szErrAcc;
   else
   {
      iRes = UpdateRec( pUStru, szData, bAppend, &ulRecNo, NULL, bCommit, NULL );
      if( !iRes )
      {
         if( bAppend )
         {
            hb_snprintf( szData1, 24, "+%lu;", ulRecNo );
            pData = szData1;
         }
         else
            pData = szOk;
      }
      else if( iRes == 2 )
         pData = szErr2;
      else if( iRes == 3 )
         pData = szErr3;
      else if( iRes == 4 )
         pData = szErr4;
      else if( iRes == ELETO_UNLOCKED )
      {
         if( bAppend )
            pData = szErrAppendLock;
         else
            pData = szErrUnlocked;
      }
      else if( iRes > 100 )
      {
         hb_snprintf( szData1, 24, "-%d;", iRes );
         pData = szData1;
      }
      else  // ???
         pData = szErr2;
   }

   leto_SendAnswer( pUStru, pData, strlen(pData) );
   pUStru->bAnswerSent = 1;
}

static int leto_SetFocus( AREAP pArea, const char * szOrder )  // mt
{
   DBORDERINFO pInfo;
   int iOrder;

   if( !pArea || !szOrder )
   {
      leto_wUsLog(NULL,"ERROR! leto_SetFocus!\r\n", 0);
      return 0;
   }

   memset( &pInfo, 0, sizeof( DBORDERINFO ) );
   pInfo.itmOrder = NULL;
   pInfo.atomBagName = NULL;
   pInfo.itmResult = hb_itemPutC( NULL, "" );
   SELF_ORDLSTFOCUS( pArea, &pInfo );

   if( strcmp( (char*)szOrder, hb_itemGetCPtr( pInfo.itmResult ) ) )
   {
      hb_itemRelease( pInfo.itmResult );

      pInfo.itmOrder = hb_itemPutC( NULL,(char*)szOrder );
      pInfo.itmResult = hb_itemPutC( NULL, "" );
      SELF_ORDLSTFOCUS( pArea, &pInfo );
      hb_itemRelease( pInfo.itmOrder );
   }
   hb_itemRelease( pInfo.itmResult );

   pInfo.itmOrder = NULL;
   pInfo.itmResult = hb_itemPutNI( NULL, 0 );
   SELF_ORDINFO( pArea, DBOI_NUMBER, &pInfo );
   iOrder = hb_itemGetNI( pInfo.itmResult );
   hb_itemRelease( pInfo.itmResult );

   return iOrder;
}

static PHB_ITEM leto_KeyToItem( AREAP pArea, const char *ptr, int iKeyLen, char *pOrder )  // mt
{
   BYTE KeyType;
   PHB_ITEM pKey = NULL;
   DBORDERINFO pInfo;

   if( !pArea || !ptr )
   {
      leto_wUsLog(NULL,"ERROR! leto_KeyToItem!\r\n", 0);
      return 0;
   }

   memset( &pInfo, 0, sizeof( DBORDERINFO ) );
   if( pOrder )
      pInfo.itmOrder = hb_itemPutC( NULL, pOrder );
   pInfo.itmResult = hb_itemPutC( NULL, "" );
   SELF_ORDINFO( pArea, DBOI_KEYTYPE, &pInfo );
   KeyType = *( hb_itemGetCPtr( pInfo.itmResult ) );
   if( pOrder && pInfo.itmOrder )
   {
      hb_itemClear( pInfo.itmOrder );
      hb_itemRelease( pInfo.itmOrder );
   }
   hb_itemClear( pInfo.itmResult );
   hb_itemRelease( pInfo.itmResult );

   switch( KeyType )
   {
      case 'N':
      {
         int iWidth, iDec;
         BOOL fDbl;
         HB_MAXINT lValue;
         double dValue;

         fDbl = hb_valStrnToNum( ptr, iKeyLen, &lValue, &dValue , &iDec, &iWidth );
         if ( !fDbl )
            pKey = hb_itemPutNIntLen( NULL, lValue, iWidth );
         else
            pKey = hb_itemPutNLen( NULL, dValue, iWidth, iDec );
         break;
      }
      case 'D':
      {
         char szBuffer[ 9 ];
         memcpy( szBuffer, ptr, 8 );
         szBuffer[ 8 ] = 0;
         pKey = hb_itemPutDS( NULL, szBuffer );
         break;
      }
      case 'L':
         pKey = hb_itemPutL( NULL, *ptr=='T' );
         break;
      case 'C':
         pKey = hb_itemPutCL( NULL, ptr, (ULONG)iKeyLen );
         break;
      default:
         break;
   }

   return pKey;
}

static ULONG leto_rec_prefix( char * szData, ULONG ulLen )
{
   ulLen ++;    // Length is incremented because of adding '+'
   szData[0] = '\3';
   HB_PUT_LE_UINT24( szData + 1, ulLen );
   szData[4] = '+';
   ulLen += 4;
   return ulLen;
}

static char * leto_recWithAlloc( AREAP pArea, PUSERSTRU pUStru, PAREASTRU pAStru, ULONG *pulLen )  // mt
{
   char * szData;
   ULONG ulRecLen;

   if( !pArea || !pAStru || !pArea )
   {
      leto_wUsLog(NULL,"ERROR! leto_recWithAlloc!\r\n", 0);
      return NULL;
   }

   ulRecLen = leto_recLen( pArea );
   szData = (char*) malloc( ulRecLen );
   if( szData )
   {
      *pulLen = leto_rec( pUStru, pAStru, pArea, szData + 5, ulRecLen - 5 );
      if( *pulLen )
      {
         *pulLen = leto_rec_prefix( szData, *pulLen );
      }
      else
      {
         free( szData );
         szData = NULL;
      }
   }
   return szData;
}

#ifdef __BM
static void leto_BMRestore( AREAP pArea, PAREASTRU pAStru )
{
   pArea->dbfi.lpvCargo = pAStru->pBM;
   pArea->dbfi.fFilter = TRUE;
   if( pAStru->pBM )
      pArea->dbfi.fOptimized = TRUE;
}
#endif

static void leto_SetFilter( PUSERSTRU pUStru, PAREASTRU pAStru, AREAP pArea )  // mt
{
   if( !pUStru || !pAStru || !pArea )
   {
      leto_wUsLog(pUStru,"ERROR! leto_SetFilter!\r\n", 0);
      return;
   }

   if( pAStru->itmFltExpr )
   {
      leto_SetUserEnv( pUStru );
      pArea->dbfi.itmCobExpr = pAStru->itmFltExpr;
      pArea->dbfi.fFilter = TRUE;
   }
#ifdef __BM
   if( pAStru->pBM )
      leto_BMRestore( pArea, pAStru );
#endif
}

static void leto_ClearFilter( AREAP pArea )
{
   pArea->dbfi.itmCobExpr = NULL;
   pArea->dbfi.fFilter = FALSE;
#ifdef __BM
   pArea->dbfi.lpvCargo = NULL;
#endif
}

int leto_SetAreaEnv( PUSERSTRU pUStru, PAREASTRU pAStru, AREAP pArea,
   BOOL bDeleted, const char *szOrder, LETOTAG ** ppTag, BOOL bSet)
{
   int iOrder = leto_SetFocus( pArea, szOrder );

   hb_setSetDeleted( bDeleted );
   if( !bNoSaveWA && (bSet || iOrder) )
   {
      if( ppTag && *szOrder )
      {
         *ppTag = leto_FindTag( pAStru, szOrder );
         leto_SetScope( pArea, *ppTag, TRUE, TRUE );
         leto_SetScope( pArea, *ppTag, FALSE, TRUE );
      }

      leto_SetFilter( pUStru, pAStru, pArea );
   }
   return iOrder;
}

void leto_ClearAreaEnv( AREAP pArea, LETOTAG * pTag )
{
   if( !bNoSaveWA )
   {
      leto_ClearFilter( pArea );

      if( pTag )
      {
         leto_SetScope( pArea, pTag, TRUE, FALSE );
         leto_SetScope( pArea, pTag, FALSE, FALSE );
      }
   }
}

HB_FUNC( LETO_SETAREAENV )
{
   int iUserStru = hb_parni( 1 );
   PUSERSTRU pUStru = s_users + iUserStru;
   PAREASTRU pAStru = pUStru->pCurAStru;
   if( pAStru )
   {
      AREAP pArea = hb_rddGetCurrentWorkAreaPointer();
      BOOL bDeleted = TRUE;
      LETOTAG * pTag = NULL;
      const char *szOrder = (HB_ISCHAR( 2 ) ? hb_parc(2) : NULL);
      leto_SetAreaEnv( pUStru, pAStru, pArea, bDeleted, szOrder, &pTag, TRUE );
   }
   hb_ret( );
}

HB_FUNC( LETO_CLEARAREAENV )
{
   int iUserStru = hb_parni( 1 );
   PUSERSTRU pUStru = s_users + iUserStru;
   PAREASTRU pAStru = pUStru->pCurAStru;
   AREAP pArea = hb_rddGetCurrentWorkAreaPointer();
   const char *szOrder = (HB_ISCHAR( 2 ) ? hb_parc(2) : NULL);
   LETOTAG * pTag = NULL;
   if( szOrder )
      pTag = leto_FindTag( pAStru, szOrder );
   leto_ClearAreaEnv( pArea, pTag );
   hb_ret( );
}

void leto_Seek( PUSERSTRU pUStru, const char* szData )  // mt
{
   AREAP pArea;
   PAREASTRU pAStru;
   char * szData1 = NULL;
   const char * pData;
   const char * pp1, * pOrdKey;
   ULONG ulLen = 4;
   char  szOrder[16];
   BOOL  bSoftSeek, bFindLast, bDeleted;
   int   iKeyLen;
   int   nParam;

   if( !pUStru || !pUStru->pCurAStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Seek!\r\n", 0);
      return;
   }

   pAStru = pUStru->pCurAStru;
   nParam = leto_GetParam( szData, &pp1, /*pOrdKey*/NULL, NULL, NULL );

   if( nParam < 2 )
      pData = szErr2;
   else
   {
      LETOTAG * pTag = NULL;

      strncpy( szOrder, szData, 15 );
      bDeleted  = ( *pp1 & 0x01 )? 1 : 0;
      bSoftSeek = ( *pp1 & 0x10 )? 1 : 0;
      bFindLast = ( *pp1 & 0x20 )? 1 : 0;

      pOrdKey = pp1 + strlen(pp1) + 1;       // binary data
      iKeyLen = (((int)*pOrdKey) & 0xFF);
      ++ pOrdKey;

      pArea = hb_rddGetCurrentWorkAreaPointer();

      hb_xvmSeqBegin();
      if( leto_SetAreaEnv( pUStru, pAStru, pArea, bDeleted, szOrder, &pTag, FALSE ) )
      {
         PHB_ITEM pKey = leto_KeyToItem( pArea, pOrdKey, iKeyLen, NULL );

         if( pKey )
         {
            SELF_SEEK( pArea, bSoftSeek, pKey, bFindLast );

            hb_itemRelease( pKey );
            if( pUStru->bHrbError )
               pData = szErr101;
            else
            {
               szData1 = leto_recWithAlloc( pArea, pUStru, pAStru, &ulLen );
               if( szData1 )
                  pData = szData1;
               else
                  pData = szErr2;
            }
         }
         else
            pData = szErr4;

         leto_ClearAreaEnv( pArea, pTag );
      }
      else
         pData = szErr4;
      hb_xvmSeqEnd();
   }

   leto_SendAnswer( pUStru, pData, ulLen );
   pUStru->bAnswerSent = 1;
   if( szData1 )
      free( szData1 );

}

void leto_Scope( PUSERSTRU pUStru, const char* szData )  // mt
{
   AREAP pArea;
   PAREASTRU pAStru;
   int iCommand;
   char  szOrder[16];
   int   iKeyLen;
   const char * pOrder, * szKey, * pData;
   int nParam;

   if( !pUStru || !pUStru->pCurAStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Scope!\r\n", 0);
      return;
   }

   pAStru = pUStru->pCurAStru;
   nParam = leto_GetParam( szData, &pOrder, /*szKey*/NULL, NULL, NULL );

   if( nParam < 2 )
      pData = szErr2;
   else
   {
      sscanf( szData, "%d", &iCommand );
      strcpy( szOrder, pOrder );

      szKey = pOrder + strlen(pOrder) + 1;       // binary data
      iKeyLen = (((int)*szKey) & 0xFF);
      ++ szKey;

      pArea = hb_rddGetCurrentWorkAreaPointer();

      if( szOrder[0] )
      {
         LETOTAG *pTag = leto_FindTag( pAStru, szOrder );

         if( pTag )
         {
            PHB_ITEM pKey = leto_KeyToItem( pArea, szKey, iKeyLen, szOrder );

            if( pKey )
            {
               if( iCommand == DBOI_SCOPETOP || iCommand == DBOI_SCOPESET )
               {
                  if( !pTag->pTopScope )
                     pTag->pTopScope = hb_itemNew( NULL );
                  hb_itemCopy( pTag->pTopScope, pKey );
                  if( bNoSaveWA )
                  {
                     leto_SetFocus( pArea, szOrder );
                     leto_SetScope( pArea, pTag, TRUE, TRUE );
                  }
               }
               if( iCommand == DBOI_SCOPEBOTTOM || iCommand == DBOI_SCOPESET )
               {
                  if( !pTag->pBottomScope )
                     pTag->pBottomScope = hb_itemNew( NULL );
                  hb_itemCopy( pTag->pBottomScope, pKey );
                  if( bNoSaveWA )
                  {
                     leto_SetFocus( pArea, szOrder );
                     leto_SetScope( pArea, pTag, FALSE, TRUE );
                  }
               }
               hb_itemRelease( pKey );
            }
            if( (iCommand == DBOI_SCOPETOPCLEAR || iCommand == DBOI_SCOPECLEAR) && pTag->pTopScope )
            {
               hb_itemClear(pTag->pTopScope);
               if( bNoSaveWA )
               {
                  leto_SetFocus( pArea, szOrder );
                  leto_SetScope( pArea, pTag, TRUE, FALSE );
               }

            }
            if( (iCommand == DBOI_SCOPEBOTTOMCLEAR || iCommand == DBOI_SCOPECLEAR) && pTag->pBottomScope )
            {
               hb_itemClear(pTag->pBottomScope);
               if( bNoSaveWA )
               {
                  leto_SetFocus( pArea, szOrder );
                  leto_SetScope( pArea, pTag, FALSE, FALSE );
               }
            }
            pData = szOk;
         }
         else
         {
            pData = szErr3;
         }
      }
      else
         pData = szErr2;
   }

   leto_SendAnswer( pUStru, pData, 4 );
   pUStru->bAnswerSent = 1;

}

static void leto_ScopeCommand( AREAP pArea, USHORT uiCommand, PHB_ITEM pKey )  // mt
{
   DBORDERINFO pInfo;

   if( !pArea )
   {
      leto_wUsLog(NULL,"ERROR! leto_ScopeCommand!\r\n", 0);
      return;
   }

   memset( &pInfo, 0, sizeof( DBORDERINFO ) );
   pInfo.itmNewVal = pKey;
   pInfo.itmOrder = NULL;
   pInfo.atomBagName = NULL;
   pInfo.itmResult = hb_itemNew( NULL );
   SELF_ORDINFO( pArea, uiCommand, &pInfo );
   hb_itemRelease( pInfo.itmResult );
}

static void leto_SetScope( AREAP pArea, LETOTAG *pTag, BOOL bTop, BOOL bSet)  // mt
{
   if( !pArea )
   {
      leto_wUsLog(NULL,"ERROR! leto_SetScope!\r\n", 0);
      return;
   }

   if( pTag )
   {
      PHB_ITEM pKey = (bTop ? pTag->pTopScope : pTag->pBottomScope);
      if( pKey )
      {
         USHORT uiCommand;

         if( bSet )
         {
            uiCommand = (bTop ? DBOI_SCOPETOP : DBOI_SCOPEBOTTOM);
         }
         else
         {
            uiCommand = (bTop ? DBOI_SCOPETOPCLEAR : DBOI_SCOPEBOTTOMCLEAR);
         }

         leto_ScopeCommand( pArea, uiCommand, (bSet ? pKey : NULL ) );
      }
   }
}

void leto_Skip( PUSERSTRU pUStru, const char* szData )  // mt
{
   AREAP pArea;
   PAREASTRU pAStru;
   char * szData1 = NULL;
   const char * pData = NULL;
   const char * pRecNo, * pOrder, * pFlags, *pSkipBuf;
   char  szOrder[16];
   ULONG ulRecNo;
   ULONG ulLen = 4, ulLenAll, ulLen2;
   LONG lSkip;
   BOOL bDeleted;
   BOOL bFlag;
   int nParam;
   ULONG ulMemSize;

   if( !pUStru || !pUStru->pCurAStru < 0 || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Skip!\r\n", 0);
      return;
   }

   pAStru = pUStru->pCurAStru;
   nParam = leto_GetParam( szData, &pRecNo, &pOrder, &pFlags, &pSkipBuf );

   if( nParam < 4 )
      pData = szErr2;
   else
   {
      int uiSkipBuf;
      LETOTAG * pTag = NULL;

      sscanf( szData, "%ld", &lSkip );
      sscanf( pRecNo, "%lu", &ulRecNo );
      strcpy( szOrder, pOrder );
      bDeleted  = ( *pFlags & 0x01 )? 1 : 0;

      pArea = hb_rddGetCurrentWorkAreaPointer();
      if( pSkipBuf && pSkipBuf[0] )
      {
         sscanf( pSkipBuf, "%d", &uiSkipBuf );
         pAStru->uiSkipBuf = uiSkipBuf;
      }
      else
         uiSkipBuf = pAStru->uiSkipBuf ? pAStru->uiSkipBuf : uiCacheRecords;

      hb_xvmSeqBegin();
      if( ! bShareTables )
      {
         if( pAStru->bUseBuffer && ( ( lSkip == 1 ) || ( lSkip == -1 ) ) )
         {
            ULONG ulRecCount;
            SELF_RECCOUNT( pArea, &ulRecCount );
            if( ( ULONG ) uiSkipBuf > ulRecCount )
            {
               uiSkipBuf = ( ulRecCount ? ulRecCount : 1 );
            }
         }
         else
            uiSkipBuf = 1;
      }

      leto_SetAreaEnv( pUStru, pAStru, pArea, bDeleted, szOrder, ( lSkip ? &pTag : NULL ), TRUE );
      leto_GotoIf( pArea, ulRecNo );
      SELF_SKIP( pArea, lSkip );

      if( !pUStru->bHrbError )
      {
         ulMemSize = leto_recLen( pArea ) * uiSkipBuf;
         szData1 = (char*) malloc( ulMemSize );
         ulLenAll = leto_rec( pUStru, pAStru, pArea, szData1 + 5, ulMemSize - 5 );
         if( ! ulLenAll )
         {
            pData = szErr2;
         }
         else if( !pAStru->bUseBuffer )
         {
            pAStru->bUseBuffer = TRUE;
         }
         else if( ( lSkip == 1 ) || ( lSkip == -1 ) )
         {
            USHORT i = 1;

            while( i < uiSkipBuf && !pUStru->bHrbError )
            {
               if( lSkip == 1 )
                  SELF_EOF( pArea, &bFlag );
               else
                  SELF_BOF( pArea, &bFlag );
               if( bFlag )
               {
                  break;
               }
               else
               {
                  SELF_SKIP( pArea, lSkip );

                  if( !pUStru->bHrbError )
                  {
                     if( lSkip == -1 )
                     {
                        SELF_BOF( pArea, &bFlag );
                        if( bFlag )
                           break;
                     }
                     i ++;
                     ulLen2 = leto_rec( pUStru, pAStru, pArea, szData1 + 5 + ulLenAll, ulMemSize - 5 - ulLenAll );
                     if( ! ulLen2 )
                     {
                        pData = szErr2;
                        break;
                     }
                     ulLenAll += ulLen2;
                  }
                  else
                  {
                     pData = szErr101;
                     break;
                  }
               }
            }
         }

         if( pUStru->bHrbError )
         {
            pData = szErr101;
         }
         else if( pData == NULL )
         {
            ulLen = leto_rec_prefix( szData1, ulLenAll );
            pData = szData1;
         }
      }
      else
         pData = szErr101;

      leto_ClearAreaEnv( pArea, pTag );

      hb_xvmSeqEnd();
   }

   leto_SendAnswer( pUStru, pData, ulLen );
   pUStru->bAnswerSent = 1;
   if( szData1 )
      free( szData1 );

}

static char * letodbEval( PUSERSTRU pUStru, AREAP pArea, ULONG *pulLen )
{
   ULONG ulRecLen = leto_recLen( pArea );
   ULONG ulMemSize = ulRecLen*10 + 5;
   ULONG ulLenAll = 0, ulLen2;
   char * szData1 = hb_xgrab( ulMemSize );
   BOOL bError = FALSE, bFlag;
   PAREASTRU pAStru = pUStru->pCurAStru;

   hb_xvmSeqBegin();
   for( ;; )
   {
      if( ulLenAll + 5 + ulRecLen > ulMemSize )
      {
         ulMemSize += ulRecLen*10;
         szData1 = hb_xrealloc( szData1, ulMemSize );
      }
      ulLen2 = leto_rec( pUStru, pAStru, pArea, szData1 + 5 + ulLenAll, ulMemSize - 5 - ulLenAll );
      if( pUStru->bHrbError || !ulLen2 )
      {
         bError = TRUE;
         break;
      }
      else
         ulLenAll += ulLen2;
      SELF_EOF( pArea, &bFlag );
      if( bFlag )
      {
         break;
      }
      SELF_SKIP( pArea, 1 );
   }
   hb_xvmSeqEnd();

   if( !bError )
      *pulLen = leto_rec_prefix( szData1, ulLenAll );
   else
   {
      hb_xfree( szData1 );
      szData1 = NULL;
   }
   return szData1;
}

HB_FUNC( LETO_DBEVAL )
{
   int iUserStru = hb_parni( 1 );
   AREAP pArea = hb_rddGetCurrentWorkAreaPointer();
   ULONG ulLen;
   char * szData;

   if( pArea && ( iUserStru < uiUsersAlloc ) )
      szData = letodbEval( s_users + iUserStru, pArea, &ulLen );
   else
      szData = NULL;

   if( szData )
      hb_retclen_buffer( szData, ulLen );
   else
      hb_ret();
}

void leto_Goto( PUSERSTRU pUStru, const char * szData )  // mt
{
   AREAP pArea;
   PAREASTRU pAStru;
   char * szData1 = NULL;
   const char * pData;
   const char * pOrder, * pFlags;
   char  szOrder[16];
   ULONG ulLen = 4;
   LONG lRecNo;
   BOOL bDeleted;
   int nParam;

   if( !pUStru || !pUStru->pCurAStru < 0 || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Goto!\r\n", 0);
      return;
   }

   pAStru = pUStru->pCurAStru;
   nParam = leto_GetParam( szData, &pOrder, &pFlags, NULL, NULL );

   if( nParam < 3 )
      pData = szErr2;
   else
   {
      LETOTAG * pTag = NULL;
      sscanf( szData, "%ld", &lRecNo );
      strcpy( szOrder, pOrder );
      bDeleted = ( *pFlags & 0x01 )? 1 : 0;

      hb_xvmSeqBegin();

      pArea = hb_rddGetCurrentWorkAreaPointer();

      if( lRecNo == -3 )
      {
         ULONG ulRecCount;
         SELF_RECCOUNT( pArea, &ulRecCount );
         lRecNo = ulRecCount + 10;
      }

      if( lRecNo >= 0 )
      {
//         leto_GotoIf( pArea, lRecNo );  // after GOTOP and skip(-1) - BOF flag not reset (if RECNO not changed)
         SELF_GOTO( pArea, (ULONG) lRecNo );
      }
      else if( (lRecNo == -1) || (lRecNo == -2) )
      {
         BOOL bTop = lRecNo==-1;

         leto_SetAreaEnv( pUStru, pAStru, pArea, bDeleted, szOrder, &pTag, TRUE );

         if( bTop )
         {
            SELF_GOTOP( pArea );
         }
         else
         {
            SELF_GOBOTTOM( pArea );
         }
      }
      if( pUStru->bHrbError )
         pData = szErr101;
      else
      {
         szData1 = leto_recWithAlloc( pArea, pUStru, pAStru, &ulLen );
         if( szData1 )
            pData = szData1;
         else
            pData = szErr2;
      }
      if( (lRecNo == -1) || (lRecNo == -2) )
         leto_ClearAreaEnv( pArea, pTag );
      hb_xvmSeqEnd();
   }

   leto_SendAnswer( pUStru, pData, ulLen );
   pUStru->bAnswerSent = 1;
   if( szData1 )
      free( szData1 );
}

int leto_Memo( PUSERSTRU pUStru, const char* szData, TRANSACTSTRU * pTA, AREAP pArea )  // mt
{
   PAREASTRU pAStru;
   const char * pNumRec, * pField, * pMemo = NULL;
   const char * pData = NULL;
   BOOL bPut = FALSE;
   BOOL bAdd = FALSE;
   ULONG ulNumrec;
   USHORT uiField = 0;
   ULONG ulLen = 4;
   int iRes = 0;
   ULONG ulField;
   int nParam;

   if( !pUStru || !pUStru->pCurAStru < 0 || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Memo!\r\n", 0);
      return 2;
   }

   pAStru = pUStru->pCurAStru;
   nParam = leto_GetParam( szData, &pNumRec, &pField, /*pMemo*/NULL, NULL );

   if( nParam < 3 )
   {
      pData = szErr2;
      iRes = 2;
   }
   else
   {
      if( !strcmp( szData,"put" ) )
         bPut = TRUE;
      else if( !strcmp( szData,"add" ) )
      {
         bPut = TRUE;
         bAdd = TRUE;
      }
      else if( strcmp( szData,"get" ) )
      {
         pData = szErr2;
         iRes = 2;
      }

      if( !pData )
      {
         sscanf( pNumRec, "%lu", &ulNumrec );
         sscanf( pField, "%ld", &ulField );
         uiField = (USHORT)ulField;
         pMemo = pField + strlen(pField) + 1;      // binary
         if( ( !ulNumrec && !pTA ) || !uiField )
         {
            pData = szErr2;
            iRes = 2;
         }
      }

      if( !pData )
      {

         if( !pArea )
            pArea = hb_rddGetCurrentWorkAreaPointer();

         if( !pTA )
            hb_xvmSeqBegin();

         if( ( !pTA || ulNumrec ) &&
               bPut && pAStru->pTStru->bShared && !pAStru->bLocked &&
               leto_IsRecLocked( pAStru, ulNumrec ) != 1 )
         {
            /*  The table is opened in shared mode, but the record isn't locked */
            pData = szErr4;
            iRes = 4;
         }
         else
         {
            if( !pTA )
            {
               leto_GotoIf( pArea, ulNumrec );
            }
            else
            {
               pTA->bAppend = ( bAdd )? 1 : 0;
               pTA->ulRecNo = ulNumrec;
            }
            if( !pUStru->bHrbError )
            {
               PHB_ITEM pItem = hb_itemNew( NULL );

               if( bPut )
               {
                  USHORT uiLenLen;
                  ULONG ulMemoLen;

                  if( ( uiLenLen = (((int)*pMemo) & 0xFF) ) >= 10 )
                  {
                     pData = szErr2;
                     iRes = 2;
                  }
                  else
                  {
                     ulMemoLen = leto_b2n( ++pMemo, uiLenLen );
                     pMemo += uiLenLen;
                     hb_itemPutCL( pItem, (char*)pMemo, ulMemoLen );

                     if( pTA )
                     {
                        pTA->pArea = pArea;
                        pTA->uiFlag = 4;
                        pTA->uiItems  = 1;
                        pTA->puiIndex = (USHORT*) malloc( sizeof(USHORT) );
                        pTA->pItems   = (PHB_ITEM*) malloc( sizeof(PHB_ITEM) );
                        pTA->puiIndex[0] = uiField;
                        pTA->pItems[0]   = hb_itemNew( pItem );
                        hb_itemClear( pItem );
                     }
                     else
                     {
                        SELF_PUTVALUE( pArea, uiField, pItem );
                        hb_itemClear( pItem );
                        if( pUStru->bHrbError )
                           pData = szErr101;
                        else
                           pData = szOk;
                     }
                  }
                  hb_timeStampGet( &pAStru->pTStru->lWriteDate, &pAStru->pTStru->lWriteTime );
                  hb_timeStampGet( &pAStru->lReadDate, &pAStru->lReadTime );
               }
               else
               {
                  SELF_GETVALUE( pArea, uiField, pItem );
                  if( pUStru->bHrbError )
                  {
                     pData = szErr101;
                  }
                  else
                  {
                     ulLen = leto_CryptText( pUStru, hb_itemGetCPtr(pItem), hb_itemGetCLen( pItem ) );
                     pData = (char*) pUStru->pBufCrypt;
                  }
                  hb_itemClear( pItem );
                  hb_timeStampGet( &pAStru->lReadDate, &pAStru->lReadTime );
               }
               hb_itemRelease( pItem );
            }
            else
               pData = szErr101;

         }
         if( !pTA )
            hb_xvmSeqEnd();
      }
   }
   if( !pTA )
   {
      leto_SendAnswer( pUStru, pData, ulLen );
      pUStru->bAnswerSent = 1;
   }
   return iRes;

}

void leto_Ordfunc( PUSERSTRU pUStru, const char* szData )  // mt
{
   AREAP pArea;
   PAREASTRU pAStru;
   char szData1[32];
   const char * pData;
   char * szData2 = NULL;
   ULONG ulLen = 4;
   const char * pTagName, * pNumber = NULL, * pDopFlags = NULL, * pFlags = NULL;
   int nParam;

   if( !pUStru || !pUStru->pCurAStru < 0 || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Ordfunc!\r\n", 0);
      return;
   }

   pAStru = pUStru->pCurAStru;
   nParam = leto_GetParam( szData, &pTagName, &pNumber, &pFlags, &pDopFlags );

   if( nParam < 1 || !(*szData == '0' || *szData == '1') )
      pData = szErr2;
   else
   {
      pArea = hb_rddGetCurrentWorkAreaPointer();

      hb_xvmSeqBegin();
      if( *szData == '1' && *(szData+1) == '0' )
      {
         /* ordListDelete */
         BOOL bDelete = TRUE;

/*
         LETOTAG * pTag = pAStru->pTag;
         while( pTag )
         {
            if( ( !strcmp( pTag->pIStru->BagName, pTagName ) ) && ( pTag->pIStru->uiAreas > 1 ) )
            {
               bDelete = FALSE;
               break;
            }
            pTag = pTag->pNext;
         }
*/

         if( bDelete )
         {
            DBORDERINFO pOrderInfo;
            LETOTAG * pTag, * pTagNext;
            memset( &pOrderInfo, 0, sizeof( DBORDERINFO ) );
            pOrderInfo.atomBagName = hb_itemPutC( NULL, pTagName );
            /* SELF_ORDLSTDELETE( pArea, &pOrderInfo ); */
            pTag = pAStru->pTag;
            while( pTag )
            {
               if( !strcmp( pTag->pIStru->BagName, pTagName ) )
               {
                  pTag->pIStru->uiAreas --;
                  pTagNext = pTag->pNext;
                  if( pTag == pAStru->pTag )
                     pAStru->pTag = pTagNext;
                  letoTagFree( pTag );
                  pTag = pTagNext;
               }
               else
                  pTag = pTag->pNext;
            }
            pData = szOk;
         }
         else
            pData = szErr101;
      }
      else if( *(szData+1) == '3' )
      {
         /* Reindex */
         SELF_ORDLSTREBUILD( pArea );
         if( pUStru->bHrbError )
            pData = szErr101;
         else
            pData = szOk;
      }
      else if( *(szData+1) == '4' )
      {
         /* ordListClear */
         PTABLESTRU pTStru;
         PINDEXSTRU pIStru;

         LETOTAG * pTag = pAStru->pTag, * pTag1, * pTagPrev;
         USHORT ui = 0;
         BOOL   bClear = 0;

         while( pTag )
         {
            pIStru = pTag->pIStru;

            // Just check, if it is another tag of an already checked bag
            pTag1 = pAStru->pTag;
            while( pTag1 != pTag && pTag1->pIStru != pIStru )
               pTag1 = pTag1->pNext;

            if( pTag1 != pTag )
            {  // if a bag is checked already
               if( ! *pTag1->szTagName )
                  *pTag->szTagName = '\0';
            }
            else if( !pIStru->bCompound )
            {  // it is a new bag and it isn't compound index
               *pTag->szTagName = '\0';
               pIStru->uiAreas --;
            }
            pTag = pTag->pNext;
         }

         // Now release and remove all tags of noncompound index
         pTag = pAStru->pTag;
         pTagPrev = NULL;
         while( pTag )
         {
            if( ! *pTag->szTagName )
            {
               pTag1 = pTag;
               if( pTag == pAStru->pTag )
                  pTag = pAStru->pTag = pTag->pNext;
               else
               {
                  pTag = pTag->pNext;
                  if( pTagPrev )
                     pTagPrev->pNext = pTag;
               }
               letoTagFree( pTag1 );
            }
            else
            {
               pTagPrev = pTag;
               pTag = pTagPrev->pNext;
            }
         }

         // And, finally, scan the open indexes of a table
         pTStru = pAStru->pTStru;
         while( ui < pTStru->uiIndexCount && ( pIStru = ( PINDEXSTRU ) letoGetListItem( &pTStru->IndexList, ui ) ) != NULL )
         {
           if( pIStru->BagName && !pIStru->bCompound && !pIStru->uiAreas )
           {
              bClear = 1;
              leto_CloseIndex( pIStru );
              /* to prevent nIndexStru changing
              letoDelFromList( &pTStru->IndexList, ui );
              pTStru->uiIndexCount --;
              */
           }
           else
              ui ++;
         }
         if( bClear )
         {
            SELF_ORDLSTCLEAR( pArea );

            ui = 0;
            while( ui < pTStru->uiIndexCount && ( pIStru = ( PINDEXSTRU ) letoGetListItem( &pTStru->IndexList, ui ) ) != NULL )
            {
              if( pIStru->BagName && !pIStru->bCompound )
              {
                 DBORDERINFO pOrderInfo;
                 memset( &pOrderInfo, 0, sizeof( DBORDERINFO ) );
                 pOrderInfo.atomBagName = hb_itemPutC( NULL, pIStru->szFullName );
                 pOrderInfo.itmResult = hb_itemNew( NULL );
                 SELF_ORDLSTADD( pArea, &pOrderInfo );
                 hb_itemRelease( pOrderInfo.itmResult );
                 hb_itemRelease( pOrderInfo.atomBagName );
              }
              ui ++;
            }
         }

         pData = szOk;
      }
      else
      {
         /* Check, if the order name transferred */
         if( nParam < 2 )
            pData = szErr2;
         else
         {
            BOOL bDeleted = FALSE;
            LETOTAG * pTag = NULL;

            if( nParam >= 4)
               bDeleted = ( *pFlags & 0x01 )? 1 : 0;

            if( leto_SetAreaEnv( pUStru, pAStru, pArea, bDeleted, pTagName, &pTag, FALSE ) )
            {
               switch( *(szData+1) )
               {
                  case '1':  /* ordKeyCount */
                  {
                     ULONG ulKeyCount = leto_GetOrdInfoNL( pArea, DBOI_KEYCOUNT );
                     if( pUStru->bHrbError )
                        pData = szErr101;
                     else
                     {
                        hb_snprintf( szData1, 32, "+%lu;", ulKeyCount );
                        pData = szData1;
                        ulLen = strlen( pData );
                     }
                     break;
                  }
                  case '2':  /* ordKeyNo */
                  case '8':  /* DBOI_KEYNORAW */
                  {
                     /* Check, if the current record number is transferred */
                     if( nParam < 3 )
                        pData = szErr2;
                     else
                     {
                        ULONG ulRecNo, ulKeyNo;

                        sscanf( pNumber, "%lu", &ulRecNo );
                        leto_GotoIf( pArea, ulRecNo );
                        ulKeyNo = leto_GetOrdInfoNL( pArea, ( *(szData+1) == '2' ? DBOI_POSITION : DBOI_KEYNORAW ) );

                        if( pUStru->bHrbError )
                           pData = szErr101;
                        else
                        {
                           hb_snprintf( szData1, 32, "+%lu;", ulKeyNo );
                           pData = szData1;
                           ulLen = strlen( pData );
                        }
                     }
                     break;
                  }
                  case '5':  /* ordKeyGoto */
                  case '9':  /* DBOI_KEYNORAW */
                  {
                     DBORDERINFO pInfo;
                     ULONG ulRecNo;

                     /* Check, if the current record number is transferred */
                     if( nParam < 3 )
                        pData = szErr2;
                     else
                     {
                        sscanf( pNumber, "%lu", &ulRecNo );
                        memset( &pInfo, 0, sizeof( DBORDERINFO ) );
                        pInfo.itmNewVal = hb_itemPutNL( NULL, ulRecNo );
                        pInfo.itmResult = hb_itemPutL( NULL, FALSE );
                        SELF_ORDINFO( pArea, ( *(szData+1) == '5' ? DBOI_POSITION : DBOI_KEYNORAW ), &pInfo );
                        if( pUStru->bHrbError || !hb_itemGetL( pInfo.itmResult ) )
                           pData = szErr101;
                        else
                        {
                           szData2 = leto_recWithAlloc( pArea, pUStru, pAStru, &ulLen );
                           if( szData2 )
                              pData = szData2;
                           else
                              pData = szErr2;
                        }
                        hb_itemRelease( pInfo.itmResult );
                     }
                     break;
                  }
                  case '6':  /* ordSkipUnique */
                  {
                     DBORDERINFO pInfo;
                     ULONG ulDirection, ulRecNo;

                     /* Check, if the direction is transferred */
                     if( nParam < 5 )
                        pData = szErr2;
                     else
                     {
                        sscanf( pNumber, "%lu", &ulRecNo );
                        sscanf( pDopFlags, "%lu", &ulDirection );

                        memset( &pInfo, 0, sizeof( pInfo ) );
                        pInfo.itmNewVal = hb_itemPutNL( NULL, ulDirection );
                        pInfo.itmResult = hb_itemPutL( NULL, FALSE );
                        leto_GotoIf( pArea, ulRecNo );
                        SELF_ORDINFO( pArea, DBOI_SKIPUNIQUE, &pInfo );
                        if( pUStru->bHrbError || !hb_itemGetL( pInfo.itmResult ) )
                           pData = szErr101;
                        else
                        {
                           szData2 = leto_recWithAlloc( pArea, pUStru, pAStru, &ulLen );
                           if( szData2 )
                              pData = szData2;
                           else
                              pData = szErr2;
                        }
                        hb_itemRelease( pInfo.itmResult );
                     }
                     break;
                  }
                  case '7':  /* ordSkipWild, ordSkipRegex */
                  {
                     if( nParam < 6 )
                        pData = szErr2;
                     else
                     {
                        DBORDERINFO pInfo;
                        int iCommand;
                        ULONG ulBufLen, ulRecNo;
                        sscanf( pNumber, "%lu", &ulRecNo );
                        sscanf( pDopFlags, "%d", &iCommand );
                        pDopFlags += strlen(pDopFlags) + 1;
                        sscanf( pDopFlags, "%lu;", &ulBufLen );
                        pDopFlags = strchr( pDopFlags, ';');
                        if( pDopFlags )
                        {
                           memset( &pInfo, 0, sizeof( pInfo ) );
                           pInfo.itmNewVal = hb_itemPutCL( NULL, pDopFlags + 1, ulBufLen );
                           pInfo.itmResult = hb_itemPutL( NULL, FALSE );
                           leto_GotoIf( pArea, ulRecNo );
                           SELF_ORDINFO( pArea, iCommand, &pInfo );
                           if( pUStru->bHrbError )
                              pData = szErr101;
                           else
                           {
                              szData2 = leto_recWithAlloc( pArea, pUStru, pAStru, &ulLen );
                              if( szData2 )
                                 pData = szData2;
                              else
                                 pData = szErr2;
                           }
                           hb_itemRelease( pInfo.itmResult );
                        }
                        else
                           pData = szErr2;
                     }
                     break;
                  }
                  default:
                     pData = szErr2;
                     break;
               }

               leto_ClearAreaEnv( pArea, pTag );

            }
            else
               pData = szErr4;
         }
      }
      hb_xvmSeqEnd();
   }

   leto_SendAnswer( pUStru, pData, ulLen );
   pUStru->bAnswerSent = 1;
   if( szData2 )
      free( szData2 );
}

static void leto_BufCheck( char **ppData, char ** pptr, ULONG * pulMemSize, ULONG ulMax, ULONG ulAdd )
{
   ULONG ulPos;
   if( *pulMemSize - (*pptr - *ppData) < ulMax )
   {
      ulPos = *pptr - *ppData;
      *pulMemSize += ulAdd;
      *ppData = (char*) hb_xrealloc( *ppData, *pulMemSize );
      *pptr = *ppData + ulPos;
   }
}

void leto_Mgmt( PUSERSTRU pUStru, const char* szData )  // mt
{
   char * ptr = NULL;
   char * pData;
   BOOL bShow;
   const char * pp1 = NULL;
   int nParam;

   if( !pUStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Mgmt!\r\n", 0);
      return;
   }

   nParam = leto_GetParam( szData, &pp1, NULL, NULL, NULL );

   if( ( bPass4M && !( pUStru->szAccess[0] & 2 ) ) || nParam < 1 || *szData != '0' )
   {
      leto_SendAnswer( pUStru, szErrAcc, 4 );
   }
   else
   {
      double dCurrent = hb_dateSeconds();
      switch( *(szData+1) )
      {
         case '0':   /* LETO_MGGETINFO */
         {
            char s[_POSIX_PATH_MAX+90];
            USHORT ui;
            double dWait = 0;
            long ulWait = 0;

            for( ui=0; ui<6; ui++ )
            {
               dWait  += dSumWait[ui];
               ulWait += uiSumWait[ui];

            }
            hb_snprintf( s, _POSIX_PATH_MAX+90, "+%d;%d;%d;%d;%lu;%lu;%lu;%lu;%u;%u;%s;%7.3f;%7.3f;%lu;%lu;%" HB_PFS "u;%" HB_PFS "u;",
               uiUsersCurr,uiUsersMax,uiTablesCurr,uiTablesMax,
               (leto_Date()-lStartDate)*86400+(long)(dCurrent-dStartsec),
               ulOperations,ulBytesSent,ulBytesRead,uiIndexCurr,uiIndexMax,
               (pDataPath ? pDataPath : ""),dMaxDayWait,(ulWait)? dWait/ulWait : 0,
               ulTransAll,ulTransOK, hb_xquery( 1001 ), hb_xquery( 1002 ) );
            leto_SendAnswer( pUStru, s, strlen(s) );
            break;
         }
         case '1':   /* LETO_MGGETUSERS */
         {
            PUSERSTRU pUStru1;
            int iTable = -1;
            USHORT ui, uiUsers;

            if( nParam == 2 )
            {
               sscanf( pp1,"%d", &iTable );
               if( iTable < 0 || iTable >= uiTablesMax )
                  iTable = -1;
            }
            pData = ( char * ) hb_xgrab( 7 + uiUsersCurr*128 );
            ptr = pData;
            *ptr++ = '+'; *ptr++ = '0'; *ptr++ = '0'; *ptr++ = '0'; *ptr++ = ';';

            ui = 0;
            uiUsers = 0;
            pUStru1 = s_users;
            while( ui < uiUsersMax && uiUsers <= 999 )
            {
               if( pUStru1->pBuffer )
               {
                  if( iTable != -1 )
                  {
                     PAREASTRU pAStru;
                     PLETO_LIST_ITEM pListItem = pUStru1->AreasList.pItem;
                     bShow = FALSE;
                     while( pListItem )
                     {
                        pAStru = ( PAREASTRU ) ( pListItem + 1 );
                        if( pAStru->pTStru && pAStru->pTStru->ulAreaID &&
                            ( pAStru->pTStru->ulAreaID == s_tables[iTable].ulAreaID ) )
                        {
                           bShow = TRUE;
                           break;
                        }
                        pListItem = pListItem->pNext;
                     }
                  }
                  else
                     bShow = TRUE;
                  if( bShow )
                  {
                     hb_snprintf( ptr, uiUsersCurr*128-(ptr-pData), "%d;%s;%s;%s;%lu;", ui,
                                       ( pUStru1->szAddr ? (char*)pUStru1->szAddr : szNull ),
                                       ( pUStru1->szNetname ? (char*)pUStru1->szNetname : szNull ),
                                       ( pUStru1->szExename ? (char*)pUStru1->szExename : szNull ),
                                       (long)(dCurrent-pUStru1->dLastAct) );
                     ptr += strlen(ptr);
                     uiUsers ++;
                  }
               }
               ui ++;
               pUStru1 ++;
            }
            *(pData+3) = ( uiUsers % 10 ) + 0x30;
            *(pData+2) = ( ( uiUsers / 10 ) % 10 ) + 0x30;
            *(pData+1) = ( ( uiUsers / 100 ) % 10 ) + 0x30;
            *ptr++ = '\r';
            *ptr++ = '\n';
            *ptr = '\0';
            leto_SendAnswer( pUStru, pData, ptr - pData );
            hb_xfree( pData );
            break;
         }
         case '2':   /* LETO_MGGETTABLES */
         {
            if( uiTablesCurr )
            {
               int iUser = -1;
               USHORT ui = 0;
               USHORT uiTables = 0;
               ULONG ulMemSize, ulPos;

               if( nParam == 2 )
               {
                  sscanf( (char*)pp1, "%d", &iUser );
                  if( iUser < 0 || iUser >= uiUsersMax )
                     iUser = -1;
               }
               ulMemSize = ( uiTablesCurr + 20 ) * 128 + 11;
               pData = (char*) hb_xgrab( ulMemSize );
               ptr = pData;
               *ptr++ = '+'; *ptr++ = '0'; *ptr++ = '0'; *ptr++ = '0'; *ptr++ = ';';

               if( iUser != -1 )
               {
                  PAREASTRU pAStru;
                  PLETO_LIST_ITEM pListItem = s_users[iUser].AreasList.pItem;
                  while( pListItem )
                  {
                     pAStru = ( PAREASTRU ) ( pListItem + 1 );
                     if( pAStru->pTStru && pAStru->pTStru->szTable )
                     {
                        if( ulMemSize - (ptr-pData) < 4*128 )
                        {
                           ulPos = ptr - pData;
                           ulMemSize += 20 * 128;
                           pData = (char*) hb_xrealloc( pData, ulMemSize );
                           ptr = pData + ulPos;
                        }
                        hb_snprintf( ptr, ulMemSize-(ptr-pData), "%d;%s;", ui, pAStru->pTStru->szTable );
                        ptr += strlen( ptr );
                        uiTables ++;
                     }
                     pListItem = pListItem->pNext;
                  }
               }
               else
               {
                  PTABLESTRU pTStru1 = s_tables;
                  while( ui < uiTablesMax && uiTables < 999 )
                  {
                     if( pTStru1->szTable )
                     {
                        if( ulMemSize - (ptr-pData) < 4*128 )
                        {
                           ulPos = ptr - pData;
                           ulMemSize += 20 * 128;
                           pData = (char*) hb_xrealloc( pData, ulMemSize );
                           ptr = pData + ulPos;
                        }
                        hb_snprintf( ptr, ulMemSize-(ptr-pData), "%d;%s;", ui, (char*)pTStru1->szTable );
                        ptr += strlen( ptr );
                        uiTables ++;
                     }
                     ui ++;
                     pTStru1 ++;
                  }
               }
               *(pData+3) = ( uiTables % 10 ) + 0x30;
               *(pData+2) = ( ( uiTables / 10 ) % 10 ) + 0x30;
               *(pData+1) = ( ( uiTables / 100 ) % 10 ) + 0x30;
               *ptr++ = '\r';
               *ptr++ = '\n';
               *ptr = '\0';
               leto_SendAnswer( pUStru, pData, ptr - pData );
               hb_xfree( pData );
            }
            else
               leto_SendAnswer( pUStru, "+000;", 5 );
            break;
         }
         case '3':   /* LETO_MGGETTIME */
         {
            char s[30];
            hb_snprintf( s, 30, "+%lu;%9.3f;", leto_Date(), dCurrent );
            leto_SendAnswer( pUStru, s, strlen(s) );
            break;
         }
         case '4':   /* LETO_MGGETLOCKS */
         {
            USHORT uiCount = 0;
            ULONG ulMemSize;
            if( uiTablesCurr )
            {
               int iUser = -1;
               USHORT ui = 0;
               PLETO_LOCK_ITEM pLockA;

               if( nParam == 2 )
               {
                  sscanf( (char*)pp1, "%d", &iUser );
                  if( iUser < 0 || iUser >= uiUsersMax )
                     iUser = -1;
               }
               pData = NULL;

               if( iUser != -1 )
               {
                  PAREASTRU pAStru;
                  PLETO_LIST_ITEM pListItem = s_users[iUser].AreasList.pItem;
                  while( pListItem )
                  {
                     pAStru = ( PAREASTRU ) ( pListItem + 1 );
                     if( pAStru->pTStru && pAStru->pTStru->szTable && pAStru->pTStru->LocksList.pItem )
                     {
                        if( !pData )
                        {
                           ulMemSize = strlen( (const char *) pAStru->pTStru->szTable ) + 128;
                           pData = hb_xgrab( ulMemSize );
                           ptr = pData;
                           *ptr++ = '+'; *ptr++ = '0'; *ptr++ = '0'; *ptr++ = '0'; *ptr++ = ';';
                        }
                        else
                           leto_BufCheck( &pData, &ptr, &ulMemSize, strlen( (const char *) pAStru->pTStru->szTable ), strlen( (const char *) pAStru->pTStru->szTable ) + 128 );
                        hb_snprintf( ptr, ulMemSize-(ptr-pData), "%s;", (char*)pAStru->pTStru->szTable );
                        ptr += strlen( ptr );
                        
                        for( pLockA = ( PLETO_LOCK_ITEM ) pAStru->LocksList.pItem; pLockA; pLockA = pLockA->pNext )
                        {
                           leto_BufCheck( &pData, &ptr, &ulMemSize, 12, 20 * 128 );
                           if( *(ptr-1) != ';' )
                              *ptr++ = ',';
                           hb_snprintf( ptr, ulMemSize-(ptr-pData), "%lu", pLockA->ulRecNo );
                           ptr += strlen( ptr );
                        }
                        *ptr++ = ';';
                        uiCount ++;
                     }
                     pListItem = pListItem->pNext;
                  }
               }
               else
               {
                  PTABLESTRU pTStru1 = s_tables;
                  while( ui < uiTablesMax && uiCount < 999 )
                  {
                     if( pTStru1->szTable && pTStru1->LocksList.pItem )
                     {
                        if( !pData )
                        {
                           ulMemSize = strlen( (const char *) pTStru1->szTable ) + 128;
                           pData = hb_xgrab( ulMemSize );
                           ptr = pData;
                           *ptr++ = '+'; *ptr++ = '0'; *ptr++ = '0'; *ptr++ = '0'; *ptr++ = ';';
                        }
                        else
                           leto_BufCheck( &pData, &ptr, &ulMemSize, strlen( (const char *) pTStru1->szTable ), strlen( (const char *) pTStru1->szTable ) + 128 );
                        hb_snprintf( ptr, ulMemSize-(ptr-pData), "%s;", (char*)pTStru1->szTable );
                        ptr += strlen( ptr );
                        
                        for( pLockA = ( PLETO_LOCK_ITEM ) pTStru1->LocksList.pItem; pLockA; pLockA = pLockA->pNext )
                        {
                           leto_BufCheck( &pData, &ptr, &ulMemSize, 12, 20 * 128 );
                           if( *(ptr-1) != ';' )
                              *ptr++ = ',';
                           hb_snprintf( ptr, ulMemSize-(ptr-pData), "%lu", pLockA->ulRecNo );
                           ptr += strlen( ptr );
                        }
                        *ptr++ = ';';
                        uiCount ++;
                     }
                     ui ++;
                     pTStru1 ++;
                  }
               }
            }
            if( uiCount )
            {
               *(pData+3) = ( uiCount % 10 ) + 0x30;
               *(pData+2) = ( ( uiCount / 10 ) % 10 ) + 0x30;
               *(pData+1) = ( ( uiCount / 100 ) % 10 ) + 0x30;
               leto_SendAnswer( pUStru, pData, ptr - pData );
               hb_xfree( pData );
            }
            else
               leto_SendAnswer( pUStru, "+000;", 5 );
            break;
         }
         case '9':   /* LETO_MGKILL */
         {
            int iUser;

            if( nParam == 2 && *pp1 )
            {
               if( strchr( pp1, '.' ) )
               {
                  /* IP-address */
                  PUSERSTRU pUStru1 = s_users;
                  USHORT ui = 0;
                  while( ui < uiUsersMax )
                  {
                     if( pUStru != pUStru1 && pUStru1->szAddr && !strcmp( (char*)pUStru1->szAddr, pp1 ) )
                     {
                       if( pUStru1->hSocket != HB_NO_SOCKET )
                          hb_socketClose( pUStru1->hSocket );
                     }
                     ui ++;
                     pUStru1 ++;
                  }
                  leto_SendAnswer( pUStru, szOk, 4 );
                  break;
               }
               else
               {
                  /* user-ID */
                  sscanf( pp1, "%d", &iUser );
                  if( iUser >= 0 && iUser < uiUsersMax )
                  {
                     PUSERSTRU pUStru1 = s_users + iUser;
                     if( pUStru != pUStru1 && pUStru1->hSocket != HB_NO_SOCKET )
                     {
                        hb_socketClose( pUStru1->hSocket );
                        leto_SendAnswer( pUStru, szOk, 4 );
                        break;
                     }
                  }
               }
            }
            leto_SendAnswer( pUStru, szErrAcc, 4 );
            break;
         }
         default:
         {
            leto_SendAnswer( pUStru, szErr2, 4 );
         }
      }

   }
   pUStru->bAnswerSent = 1;
}

void leto_Intro( PUSERSTRU pUStru, const char* szData )  // mt
{
   const char * pData = NULL;
   const char * pp1, * pp2, * pp3, * pp4;
   char * ptr;
   int nParam;

   if( !pUStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Intro!\r\n", 0);
      return;
   }

   nParam = leto_GetParam( szData, &pp1, &pp2, &pp3, &pp4 );

   if( nParam < 3 )
      pData = szErr2;
   else
   {
      if( pUStru->szVersion )
         hb_xfree( pUStru->szVersion );
      pUStru->szVersion = (char*) hb_xgrab( strlen(szData)+1 );
      strcpy( (char*) pUStru->szVersion, szData );
      sscanf( pUStru->szVersion, "%d.%d", &pUStru->uiMajorVer, &pUStru->uiMinorVer);

      if( pUStru->szNetname )
         hb_xfree( pUStru->szNetname );
      pUStru->szNetname = (BYTE*) hb_xgrab( strlen(pp1)+1 );
      strcpy( (char*)pUStru->szNetname, pp1 );

      if( pUStru->szExename )
         hb_xfree( pUStru->szExename );
      pUStru->szExename = (BYTE*) hb_xgrab( strlen(pp2)+1 );
      strcpy( (char*)pUStru->szExename, pp2 );

      /* pp3 - UserName, ptr - password */
      if( bPass4L || bPass4M || bPass4D )
      {
         BOOL bPassOk = 0;

         if( nParam >= 5 )
         {
            ULONG ulLen = strlen(pp3);

            if( ulLen > 0 && ulLen < 18 )
            {
               char szBuf[36], szUser[19], szPass[24];

               strcpy( szUser, pp3 );

               ulLen = strlen(pp4);
               if( ulLen > 4 && ulLen < 50 )
               {
                  char szKey[LETO_MAX_KEYLENGTH+1];
/*
                  USHORT uiKeyLen = strlen(LETO_PASSWORD);

                  if( uiKeyLen > LETO_MAX_KEYLENGTH )
                     uiKeyLen = LETO_MAX_KEYLENGTH;
*/
                  strcpy( szKey, LETO_PASSWORD );
                  szKey[0] = pUStru->szDopcode[0];
                  szKey[1] = pUStru->szDopcode[1];
                  leto_hexchar2byte( pp4, ulLen, szBuf );
                  ulLen = ulLen / 2;
                  leto_decrypt( szBuf, ulLen, szPass, &ulLen, szKey );
                  szPass[ulLen] = '\0';
               }

               ptr = leto_acc_find( szUser, szPass );
               if( ptr )
                  bPassOk = 1;
            }
         }
         if( bPassOk )
         {
            pUStru->szAccess[0] = *ptr;
            pUStru->szAccess[1] = *(ptr+1);
         }
         else if( bPass4L )
         {
            leto_CloseUS( pUStru );
            pUStru->bAnswerSent = 1;
            return;
         }
         else
            pData = szErrAcc;
      }
      else
         pUStru->szAccess[0] = pUStru->szAccess[1] = 0xff;

      if( nParam > 5 )
      {
         pp1 = pp4 + strlen(pp4) + 1;
         ptr = strchr( pp1, ';' );
         if( ptr != NULL )
         {
            *ptr = '\0';
            pUStru->cdpage = hb_cdpFind( pp1 );
            pp1 = ptr + 1;
            if( ( ptr = strchr( pp1, ';' ) ) != NULL )
            {
               if( pUStru->szDateFormat )
                  hb_xfree( pUStru->szDateFormat );
               pUStru->szDateFormat = (char*) hb_xgrab( ptr - pp1 + 1 );
               memcpy( pUStru->szDateFormat, pp1, ptr - pp1 );
               pUStru->szDateFormat[ptr-pp1] = '\0';
               if( *(ptr + 1) == ';')
                  pUStru->bCentury = (*(ptr + 2) == 'T' ? TRUE : FALSE);
            }
         }
      }
      if( !pData )
      {
         char * szVersion = hb_verHarbour();
         char pBuf[121];
         hb_snprintf(pBuf, 121, "+%c%c%c;%s;%d;%d;%d", ( pUStru->szAccess[0] & 1 )? 'Y' : 'N',
               ( pUStru->szAccess[0] & 2 )? 'Y' : 'N',
               ( pUStru->szAccess[0] & 4 )? 'Y' : 'N',
               szVersion, uiDriverDef, uiMemoType, ( int ) ( pUStru - s_users ) );
         hb_xfree( szVersion );
         pData = pBuf;
      }
   }
   leto_SendAnswer( pUStru, pData, strlen( pData ) );
   pUStru->bAnswerSent = 1;

}

void leto_CloseAll4Us( PUSERSTRU pUStru )  // mt
{
   PAREASTRU pAStru;
   PLETO_LIST_ITEM pListItem;

   if( !pUStru )
   {
      leto_wUsLog(pUStru,"ERROR! leto_CloseAll4Us!\r\n", 0);
      return;
   }

   pListItem = pUStru->AreasList.pItem;
   while( pListItem )
   {
      pAStru = ( PAREASTRU ) ( pListItem + 1 );
      if( pAStru->pTStru )
      {
         leto_CloseArea( pAStru );
      }
      pListItem = pListItem->pNext;
   }
   leto_SendAnswer( pUStru, szOk, 4 );
   pUStru->bAnswerSent = 1;

}

void leto_CloseT( PUSERSTRU pUStru, const char* szData )  //mt
{
   int nParam;
   const char * pData;

   if( !pUStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_CloseT!\r\n", 0);
      return;
   }

   nParam = leto_GetParam( szData, NULL, NULL, NULL, NULL );

   if( nParam < 1 )
      pData = szErr2;
   else
   {
      PAREASTRU pAStru;
      ULONG ulAreaID=0;

      sscanf( szData, "%lu", &ulAreaID );
      if( ulAreaID <= 0 )
         pData = szErr2;
      else
      {
         pAStru = leto_FindArea( pUStru, ulAreaID );
         if( pAStru )
         {
            if( bShareTables )
            {
               leto_SelectArea( pUStru, pAStru->ulAreaID );
            }
            leto_CloseArea( pAStru );
            pData = szOk;
         }
         else
            pData = szErr4;
      }
   }

   leto_SendAnswer( pUStru, pData, 4 );
   pUStru->bAnswerSent = 1;
}

HB_FUNC( LETOCLOSEAREA )
{
   int iUserStru = hb_parni( 1 );
   if( HB_ISNUM(1) && ( iUserStru < uiUsersAlloc ) )
   {
      PUSERSTRU pUStru = s_users + iUserStru;
      char szData[ 24 ];
      sprintf( szData, "%lu;\r\n", pUStru->pCurAStru->ulAreaID );
      leto_CloseT( pUStru, szData );
   }
   hb_ret( );
}

void leto_Pack( PUSERSTRU pUStru, const char* szData )  // mt
{
   AREAP pArea;
   const char * pData;

   if( !pUStru )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Pack!\r\n", 0);
      return;
   }
   else if( leto_WriteDeny( pUStru ) )
   {
      leto_SendAnswer( pUStru, szErrAcc, 4 );
      pUStru->bAnswerSent = 1;
      return;
   }

   HB_SYMBOL_UNUSED( szData );
   pArea = hb_rddGetCurrentWorkAreaPointer();

   if( pArea->valResult )
      hb_itemClear( pArea->valResult );
   else
      pArea->valResult = hb_itemNew( NULL );
   hb_xvmSeqBegin();
   SELF_PACK( pArea );
   hb_xvmSeqEnd();
   if( pUStru->bHrbError )
      pData = szErr101;
   else
      pData = szOk;

   leto_SendAnswer( pUStru, pData, 4 );
   pUStru->bAnswerSent = 1;
}

void leto_Zap( PUSERSTRU pUStru, const char* szData )  // mt
{
   AREAP pArea;
   const char * pData;

   if( !pUStru )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Zap!\r\n", 0);
      return;
   }
   else if( leto_WriteDeny( pUStru ) )
   {
      leto_SendAnswer( pUStru, szErrAcc, 4 );
      pUStru->bAnswerSent = 1;
      return;
   }

   HB_SYMBOL_UNUSED( szData );
   pArea = hb_rddGetCurrentWorkAreaPointer();

   hb_xvmSeqBegin();
   SELF_ZAP( pArea );
   hb_xvmSeqEnd();
   if( pUStru->bHrbError )
      pData = szErr101;
   else
      pData = szOk;

   leto_SendAnswer( pUStru, pData, 4 );
   pUStru->bAnswerSent = 1;
}

void leto_Flush( PUSERSTRU pUStru, const char* szData )  // mt
{
   AREAP pArea;
   const char * pData;

   if( !pUStru )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Flush!\r\n", 0);
      return;
   }

   HB_SYMBOL_UNUSED( szData );
   pArea = hb_rddGetCurrentWorkAreaPointer();

   hb_xvmSeqBegin();
   SELF_FLUSH( pArea );
   hb_xvmSeqEnd();
   pUStru->pCurAStru->bUseBuffer = FALSE;
   if( pUStru->bHrbError )
      pData = szErr101;
   else
      pData = szOk;

   leto_SendAnswer( pUStru, pData, 4 );
   pUStru->bAnswerSent = 1;
}

void leto_Reccount( PUSERSTRU pUStru, const char* szData )  // mt
{
   AREAP pArea;
   ULONG ulRecCount = 0;
   const char * pData;
   char s[16];

   if( !pUStru )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Reccount!\r\n", 0);
      return;
   }

   HB_SYMBOL_UNUSED( szData );
   pArea = hb_rddGetCurrentWorkAreaPointer();

   SELF_RECCOUNT( pArea, &ulRecCount );
   hb_snprintf( s, 16, "+%lu;", ulRecCount );
   pData = s;

   leto_SendAnswer( pUStru, pData, strlen(pData) );
   pUStru->bAnswerSent = 1;
}

void leto_Set( PUSERSTRU pUStru, const char* szData )  // mt
{
   const char * pAreaID, * pSkipBuf = NULL;
   int nParam;

   if( !pUStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Set!\r\n", 0);
      return;
   }

   nParam = leto_GetParam( szData, &pAreaID, &pSkipBuf, NULL, NULL );

   if( nParam < 1 || *szData != '0' )
      leto_SendAnswer( pUStru, szErr2, 4 );
   else
   {
      switch( *(szData+1) )
      {
         case '1':
            break;
         case '2':
         {
            if( nParam >= 3 )
            {
               ULONG ulAreaID;
               int   iSkipBuf;
               PAREASTRU pAStru;

               sscanf( pAreaID, "%lu", &ulAreaID );
               sscanf( pSkipBuf, "%d", &iSkipBuf );
               if( ( pAStru = leto_FindArea( pUStru, ulAreaID ) ) != NULL )
               {
                  pAStru->uiSkipBuf = (USHORT) iSkipBuf;
               }
            }
            break;
         }
         case '3':
         case '4':
         {
            if( pAreaID )
            {
               BOOL bSet = ( *pAreaID == 'T' );
               if( *(szData+1) == '3' )
                  pUStru->bBufKeyNo = bSet;
               else
                  pUStru->bBufKeyCount = bSet;
            }
            break;
         }
      }
      leto_SendAnswer( pUStru, szOk, 4 );
   }
   pUStru->bAnswerSent = 1;
}

void leto_Transaction( PUSERSTRU pUStru, const char* szData, ULONG ulTaLen )
{
   AREAP pArea = NULL;
   USHORT uiLenLen;
   int iRecNumber, i = 0, i1;
   ULONG ulLen, ulRecNo, ulAreaID, ulAreaIDOld = 0;
   const char * ptr = szData;
   const char * pData = NULL;
   char * ptrPar;
   TRANSACTSTRU * pTA;
   int iRes = 0;
   BOOL bUnlockAll;
   char szData1[16];

   ULONG * pulAppends = NULL;
   int iAppended = 0;
   char * pResult = NULL;
   ULONG ulResLen = 4;

   if( !pUStru )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Transaction!\r\n", 0);
      return;
   }

   if( leto_WriteDeny( pUStru ) )
   {
      leto_SendAnswer( pUStru, szErrAcc, 4 );
      pUStru->bAnswerSent = 1;
      return;
   }

   ulTransAll ++;
   iRecNumber = leto_b2n( ptr, 4 );
   ptr += 4;
   bUnlockAll = ( *ptr & 1 );
   ptr ++;

   if( iRecNumber )
   {
      pTA = (TRANSACTSTRU *) malloc( sizeof(TRANSACTSTRU) * iRecNumber );
      memset( pTA,0,sizeof(TRANSACTSTRU)*iRecNumber );

      while( !iRes && i < iRecNumber && ((ULONG)( ptr - szData )) < ulTaLen )
      {
         if( ( uiLenLen = (((int)*ptr) & 0xFF) ) < 10 )
         {
            ulLen = leto_b2n( ptr+1, uiLenLen );
            ptr += uiLenLen + 1;

            if( iDebugMode >= 10 )
               leto_wUsLog( pUStru, ptr, ulLen );

            if( ( ptrPar = strchr( ptr, ';' ) ) != NULL )
            {
               ++ ptrPar;
               sscanf( ptrPar, "%lu;", &ulAreaID );
               if( ulAreaID && ( ptrPar = strchr( ptrPar, ';' ) ) != NULL && ( ulAreaIDOld == ulAreaID || ( pArea = leto_SelectArea( pUStru, ulAreaID ) ) != NULL ) )
               {
                  ulAreaIDOld = ulAreaID;
                  ++ ptrPar;
                  if( !strncmp( ptr, "upd;", 4 ) )
                     iRes = UpdateRec( pUStru, ptrPar, FALSE, NULL, &pTA[i], FALSE, pArea );
                  else if( !strncmp( ptr, "add;", 4 ) )
                     iRes = UpdateRec( pUStru, ptrPar, TRUE, NULL, &pTA[i], FALSE, pArea );
                  else if( !strncmp( ptr, "memo;", 5 ) )
                     iRes = leto_Memo( pUStru, ptrPar, &pTA[i], pArea );
                  else
                  {
                     leto_wUsLog(pUStru,"ERROR! leto_Transaction! bad command!\r\n", 0);
                     iRes = 2;
                  }
               }
               else
               {
                  leto_wUsLog(pUStru,"ERROR! leto_Transaction! bad nAreaID!\r\n", 0);
                  iRes = 2;
               }
            }
            else
            {
               leto_wUsLog(pUStru,"ERROR! leto_Transaction! missing nAreaID!\r\n", 0);
               iRes = 2;
            }
            ptr += ulLen;
         }
         else
         {
            leto_wUsLog(pUStru,"ERROR! leto_Transaction! bad message length!\r\n", 0);
            iRes  = 2;
            pData = szErr2;
            break;
         }
         i ++;
      }

      if( !iRes )
      {

         hb_xvmSeqBegin();
         for( i=0; i < iRecNumber && !pUStru->bHrbError; i++ )
         {
            pArea = pTA[i].pArea;

            if( pTA[i].bAppend )
            {
               hb_rddSetNetErr( FALSE );
               SELF_APPEND( pArea, TRUE );
               SELF_RECNO( pArea, &pTA[i].ulRecNo );

               for( i1 = 0; i1 < iAppended; i1 ++ )
                  if( pulAppends[i1*2] == (ULONG)pArea->uiArea )
                     break;
               if( i1 == iAppended )
               {
                  iAppended ++;
                  if( pulAppends )
                     pulAppends = hb_xrealloc( pulAppends, sizeof( ULONG ) * iAppended * 2 );
                  else
                     pulAppends = hb_xgrab( sizeof( ULONG ) * iAppended * 2 );
                  pulAppends[ i1*2 ] = pArea->uiArea;
               }
               pulAppends[ i1*2 + 1 ] = pTA[i].ulRecNo;

            }
            else
            {
               ulRecNo = pTA[i].ulRecNo;
               if( !ulRecNo )
               {
                  for( i1=i-1; i1>=0; i1-- )
                     if( pArea == pTA[i1].pArea && pTA[i1].bAppend )
                     {
                        ulRecNo = pTA[i1].ulRecNo;
                        break;
                     }
               }
               leto_GotoIf( pArea, ulRecNo );
            }

            if( pTA[i].uiFlag & 1 )
               SELF_DELETE( pArea );
            else if( pTA[i].uiFlag & 2 )
               SELF_RECALL( pArea );

            for( i1=0; i1 < pTA[i].uiItems; i1++ )
               if( pTA[i].pItems[i1] )
                  SELF_PUTVALUE( pArea, pTA[i].puiIndex[i1], pTA[i].pItems[i1] );

         }

         if( bOptimize )
            hb_setHardCommit( TRUE );
         for( i=0; i < iRecNumber && !pUStru->bHrbError; i++ )
         {
            pArea = pTA[i].pArea;
            if( ( (DBFAREAP) pArea )->fUpdateHeader || ( (DBFAREAP) pArea )->fDataFlush ||
               ( ( (DBFAREAP) pArea )->fHasMemo && ( (DBFAREAP) pArea )->pMemoFile && ( (DBFAREAP) pArea )->fMemoFlush ) )
            {
               SELF_FLUSH( pArea );
            }
         }
         if( bOptimize )
            hb_setHardCommit( FALSE );

         hb_xvmSeqEnd();
         if( pUStru->bHrbError )
            pData = szErr101;
         else
         {
            if( pulAppends )
            {
               pResult = hb_xgrab( iAppended*2*22 + 15 );
               memcpy( pResult, szOk, 4);
               ptrPar = pResult + 4;
               sprintf( ptrPar, ";%d;", iAppended );
               ptrPar += strlen( ptrPar );
               for( i1 = 0; i1 < iAppended; i1 ++ )
               {
                  sprintf( ptrPar, "%lu,%lu;", pulAppends[ i1*2 ], pulAppends[ i1*2 + 1 ] );
                  ptrPar += strlen(ptrPar);
               }
               pData = pResult;
               ulResLen = strlen( pResult );
            }
            else
               pData = szOk;
            ulTransOK ++;
         }
      }
      else if( iRes == 2 )
         pData = szErr2;
      else if( iRes == 3 )
         pData = szErr3;
      else if( iRes == 4 )
         pData = szErr4;
      else if( iRes > 100 )
      {
         hb_snprintf( szData1, 16, "-%d;", iRes );
         pData = szData1;
      }
      if( pulAppends )
         hb_xfree( pulAppends );

      for( i=0; i < iRecNumber; i++ )
      {
         if( pTA[i].puiIndex )
            free( pTA[i].puiIndex );
         if( pTA[i].pItems )
         {
            for( i1=0; i1 < pTA[i].uiItems; i1++ )
               if( pTA[i].pItems[i1] )
                  hb_itemRelease( pTA[i].pItems[i1] );
            free( pTA[i].pItems );
         }
      }
      free( pTA );
   }
   else
      pData = szOk;

   if( bUnlockAll )
   {
      PAREASTRU pAStru;
      PLETO_LIST_ITEM pListItem = pUStru->AreasList.pItem;
      while( pListItem )
      {
         pAStru = ( PAREASTRU ) ( pListItem + 1 );
         if( pAStru->pTStru && ( !bShareTables || ( leto_SelectArea( pUStru, pAStru->ulAreaID ) != NULL ) ) )
         {
            leto_UnlockAllRec( pAStru );
         }
         pListItem = pListItem->pNext;
      }
   }

   leto_SendAnswer( pUStru, pData, ulResLen );
   pUStru->bAnswerSent = 1;
   if( pResult )
      hb_xfree( pResult );
}

static PHB_ITEM leto_mkCodeBlock( const char * szExp, ULONG ulLen )  // mt
{
   PHB_ITEM pBlock = NULL;

   if( ulLen > 0 )
   {
      char * szMacro = ( char * ) hb_xgrab( ulLen + 5 );

      szMacro[ 0 ] = '{';
      szMacro[ 1 ] = '|';
      szMacro[ 2 ] = '|';
      memcpy( szMacro + 3, szExp, ulLen );
      szMacro[ 3 + ulLen ] = '}';
      szMacro[ 4 + ulLen ] = '\0';

      pBlock = hb_itemNew( NULL );
      hb_vmPushString( szMacro, ulLen+5 );
      hb_xfree( szMacro );
      hb_macroGetValue( hb_stackItemFromTop( -1 ), 0, 64 );
      hb_itemMove( pBlock, hb_stackItemFromTop( -1 ) );
      hb_stackPop();
   }

   return pBlock;
}

static void leto_ResetFilter( PAREASTRU pAStru )
{
   if( pAStru->itmFltExpr )
   {
      hb_itemClear( pAStru->itmFltExpr );
      hb_itemRelease( pAStru->itmFltExpr );
   }
   pAStru->itmFltExpr = NULL;
#ifdef __BM
   if( pAStru->pBM )
   {
      hb_xfree( pAStru->pBM );
      pAStru->pBM = NULL;
   }
   pAStru->fFilter = FALSE;
#endif
}

void leto_Filter( PUSERSTRU pUStru, const char* szData, ULONG ulLen )  // mt
{
   PAREASTRU pAStru;
   char * pFilter;

   if( !pUStru || !pUStru->pCurAStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Filter!\r\n", 0);
      return;
   }

   pAStru = pUStru->pCurAStru;
   pFilter = (char*) szData;

   if( ulLen <= 0 && pFilter[ulLen-1] != ';' )
      leto_SendAnswer( pUStru, szErr2, 4 );
   {
      BOOL bClear, bRes;

      -- ulLen;
      pFilter[ulLen] = '\0';

      if( ulLen > 0 )
         bClear = FALSE;
      else
         bClear = TRUE;

      leto_ResetFilter( pAStru );

      if( bClear )
      {
         if( bNoSaveWA )
            leto_ClearFilter( hb_rddGetCurrentWorkAreaPointer() );

         leto_SendAnswer( pUStru, "++", 2 );
      }
      else
      {
         PHB_ITEM pFilterBlock = NULL;

         if( ! leto_ParseFilter( pUStru, pFilter, ulLen ) )
         {
            bRes = FALSE;
         }
         else
         {
            bRes = TRUE;
            hb_xvmSeqBegin();
            pFilterBlock = leto_mkCodeBlock( pFilter, ulLen );
            if( pFilterBlock == NULL )
               bRes = FALSE;
            else
               hb_vmEvalBlock( pFilterBlock );
#ifdef __BM
            if( hb_setGetOptimize() && hb_setGetForceOpt() )
            {
               AREAP pArea = hb_rddGetCurrentWorkAreaPointer();
               DBFILTERINFO pFilterInfo;
               PHB_ITEM pText = hb_itemPutCL( NULL, pFilter, ulLen );
               pFilterInfo.itmCobExpr = pFilterBlock;
               pFilterInfo.abFilterText = pText;
               pFilterInfo.fFilter = TRUE;
               pFilterInfo.lpvCargo = NULL;
               pFilterInfo.fOptimized = TRUE;
               SELF_SETFILTER( pArea, &pFilterInfo );

               pAStru->pBM = pArea->dbfi.lpvCargo;
               pAStru->fFilter = TRUE;

               pArea->dbfi.itmCobExpr = NULL;
               pArea->dbfi.lpvCargo = NULL;
               pArea->dbfi.fFilter = FALSE;
               pArea->dbfi.fOptimized = FALSE;
               hb_itemRelease( pText );
               pArea->dbfi.abFilterText = NULL;
            }
#endif
            hb_xvmSeqEnd();
            if( pUStru->bHrbError )
            {
               bRes = FALSE;
               if( pFilterBlock )
                  hb_itemRelease( pFilterBlock );
            }
         }
         if( ! bRes )
         {
            leto_SendAnswer( pUStru, "-012", 4 );
            if( iDebugMode >= 1 )
            {
               leto_wUsLog( pUStru, "!WARNING! leto_Filter! filter not optimized or syntax error:\r\n", 0 );
               leto_wUsLog( pUStru, pFilter, ulLen );
            }
         }
         else
         {
            pAStru->itmFltExpr = pFilterBlock;
            if( bNoSaveWA )
               leto_SetFilter( pUStru, pAStru, hb_rddGetCurrentWorkAreaPointer() );
            leto_SendAnswer( pUStru, "++", 2 );
         }
      }
   }

   pUStru->bAnswerSent = 1;
}

#ifdef __BM
HB_FUNC( LETO_BMRESTORE )
{
   int iUserStru = hb_parni( 1 );
   if( HB_ISNUM( 1 ) && ( iUserStru < uiUsersAlloc ) )
   {
      PUSERSTRU pUStru = s_users + iUserStru;
      if( pUStru->pCurAStru )
      {
         PAREASTRU pAStru = pUStru->pCurAStru;
         AREAP pArea = hb_rddGetCurrentWorkAreaPointer();
         if( hb_parl( 2 ) )
            leto_ResetFilter( pAStru );
         leto_BMRestore( pArea, pAStru );
      }
   }
}

HB_FUNC( LETO_BMSAVE )
{
   int iUserStru = hb_parni( 1 );
   if( HB_ISNUM( 1 ) && ( iUserStru < uiUsersAlloc ) )
   {
      PUSERSTRU pUStru = s_users + iUserStru;
      if( pUStru->pCurAStru )
      {
         PAREASTRU pAStru = pUStru->pCurAStru;
         AREAP pArea = hb_rddGetCurrentWorkAreaPointer();

         pAStru->pBM = pArea->dbfi.lpvCargo;
         pAStru->fFilter = pArea->dbfi.fFilter;
         pArea->dbfi.lpvCargo = NULL;
         pArea->dbfi.fFilter = pArea->dbfi.fOptimized = FALSE;
      }
   }
}
#endif

static void letoPutDouble( char * ptr, PHB_ITEM pItem, double dSum, SHORT iDec )
{
   HB_SIZE nLen;
   BOOL bFreeReq;
   char * buffer, *bufstr;

   hb_itemPutNDDec( pItem, dSum, iDec );
   buffer = hb_itemString( pItem, &nLen, &bFreeReq );
   bufstr = buffer;
   while( *bufstr && *bufstr == ' ' )
      bufstr ++;
   strcpy(ptr, bufstr);

   if( bFreeReq )
      hb_xfree( buffer );
}

void leto_GroupBy( PUSERSTRU pUStru, const char* szData, USHORT uiDataLen )
{
   AREAP pArea;
   const char * pOrder, *pGroup, * pFields, * pFilter, *pFlag;
   int nParam;

   if( !pUStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_GroupBy!\r\n", 0);
      return;
   }

   nParam = leto_GetParam( szData, &pGroup, &pFields, &pFilter, &pFlag );
   pOrder = szData;

   if( nParam < 5 )
   {
      leto_SendAnswer( pUStru, szErr2, 4 );
   }
   else
   {
      char  szOrder[16];
      typedef struct
      {
         SHORT Pos;
         PHB_ITEM pBlock;
         USHORT Dec;
      } VALGROUPSTRU;
      USHORT uiCount = 0, uiAllocated = 10, uiIndex, uiGroup;
      BOOL bEnd = FALSE;
      VALGROUPSTRU * pSumFields = hb_xgrab( sizeof( VALGROUPSTRU ) * uiAllocated );
      int iKeyLen, iDec;
      char szFieldName[12];
      PHB_ITEM pGroupBlock = NULL;
      PHB_ITEM pGroupVal = NULL;
      PHB_ITEM pItem = hb_itemNew( NULL ), pBlock, pNumItem;
      PHB_ITEM pFilterBlock = NULL, pValFilter;
      PHB_ITEM pTopScope = NULL;
      PHB_ITEM pBottomScope = NULL;
      BOOL bEof;
      char *ptr, * pNext;
      char cGroupType;
      double dValue;
      LETOTAG * pTag = NULL;

      PHB_ITEM pHash = hb_hashNew( NULL );
      PHB_ITEM pDefault = hb_itemNew( NULL );
      PHB_ITEM pSubArray;

      pArea = hb_rddGetCurrentWorkAreaPointer();

      if( pFields-pGroup-1 < 12)
      {
         memcpy( szFieldName, pGroup, pFields-pGroup-1 );
         szFieldName[ pFields-pGroup-1 ] = '\0';
         uiGroup = hb_rddFieldIndex( pArea, szFieldName );
      }
      else
         uiGroup = 0;
      if( uiGroup )
      {
         PHB_ITEM pItemType = hb_itemNew( NULL );

         if( SELF_FIELDINFO( pArea, uiGroup, DBS_TYPE, pItemType ) == HB_SUCCESS )
         {
            cGroupType = hb_itemGetCPtr( pItemType )[0];
         }
         else
            cGroupType = ' ';
         hb_itemRelease( pItemType );
         pGroupVal = hb_itemNew( NULL );
      }
      else
      {
         cGroupType = leto_ExprGetType( pUStru, pGroup, pFields-pGroup-1 );
         pGroupBlock = leto_mkCodeBlock( pGroup, pFields-pGroup-1 );
      }

      ptr = ( char * ) pFields;
      for( ;; )
      {
         SHORT iPos;

         pNext = strchr(ptr, ',');
         if( ! pNext )
         {
            pNext = ptr + strlen(ptr);
            bEnd = TRUE;
         }
         if( pNext-ptr < 12)
         {
            memcpy( szFieldName, ptr, pNext-ptr );
            szFieldName[ pNext-ptr ] = '\0';
         }
         else
            szFieldName[0] = '\0';

         pBlock = NULL;
         if( szFieldName[0] == '#' )
         {
            iPos = -1;
         }
         else
         {
            if( szFieldName[0] )
               iPos = (SHORT) hb_rddFieldIndex( pArea, szFieldName );
            else
               iPos = 0;
            if( ( iPos == 0 ) && leto_ExprGetType( pUStru, ptr, pNext-ptr ) == 'N' )
               pBlock = leto_mkCodeBlock( ptr, pNext-ptr );
         }

         if( iPos || pBlock )
         {
            uiCount ++;
            if( uiCount > uiAllocated )
            {
               uiAllocated += 10;
               pSumFields = (VALGROUPSTRU *) hb_xrealloc( pSumFields, sizeof( VALGROUPSTRU ) * uiAllocated );
            }
            pSumFields[ uiCount-1 ].Pos = iPos;
            pSumFields[ uiCount-1 ].pBlock = pBlock;
            if( iPos > 0)
            {
               SELF_FIELDINFO( pArea, iPos, DBS_DEC, pItem );
               pSumFields[ uiCount-1 ].Dec = hb_itemGetNI( pItem );
            }
            else
               pSumFields[ uiCount-1 ].Dec = 0;
         }

         if( bEnd ) break;
         ptr = pNext + 1;
      }

      if( uiCount && ( uiGroup || ( pGroupBlock && cGroupType != ' ' ) ) )
      {

         hb_arrayNew( pDefault, uiCount + 1 );
         for( uiIndex = 0; uiIndex < uiCount; uiIndex ++ )
         {
            if( pSumFields[ uiIndex ].Dec == 0 )
               hb_arraySetNL( pDefault, uiIndex + 2, 0);
            else
               hb_arraySetND( pDefault, uiIndex + 2, 0.0);
         }
         hb_hashSetFlags( pHash, HB_HASH_AUTOADD_ALWAYS );
         hb_hashSetDefault( pHash, pDefault );

         if( strlen(pOrder) < 16 )
            strcpy( szOrder, pOrder );
         else
            szOrder[0] = '\0';

         if( *pFilter != '\0' )
         {
            pFilterBlock = leto_mkCodeBlock( pFilter, strlen(pFilter) );
         }

         ptr = ( char * ) pFlag + 2;
         if( ptr < pOrder + uiDataLen )
         {
            iKeyLen = (((int)*ptr++) & 0xFF);

            if( iKeyLen )
            {
               pTopScope = leto_KeyToItem( pArea, ptr, iKeyLen, szOrder );
               ptr += iKeyLen;
               iKeyLen = (((int)*ptr++) & 0xFF);
               if( iKeyLen && ptr < pOrder + uiDataLen )
               {
                  pBottomScope = leto_KeyToItem( pArea, ptr, iKeyLen, szOrder );
               }
            }
         }

         hb_xvmSeqBegin();

         if( szOrder[0] != '\0' )
            leto_SetFocus( pArea, szOrder );

         if( pTopScope )
         {
            hb_setSetDeleted( *pFlag == 0x41 );
            leto_ScopeCommand(pArea, DBOI_SCOPETOP, pTopScope );
            leto_ScopeCommand(pArea, DBOI_SCOPEBOTTOM, (pBottomScope ? pBottomScope : pTopScope ) );
            leto_SetFilter( pUStru, pUStru->pCurAStru, pArea );
         }
         else
            leto_SetAreaEnv( pUStru, pUStru->pCurAStru, pArea, *pFlag == 0x41, szOrder, &pTag, TRUE );

         SELF_GOTOP( pArea );
         for( ;; )
         {
            SELF_EOF( pArea, &bEof );
            if( bEof ) break;

            if( pFilterBlock )
            {
               pValFilter = hb_vmEvalBlock( pFilterBlock );
               bEnd = HB_IS_LOGICAL( pValFilter ) && hb_itemGetL( pValFilter );
            }
            else
               bEnd = TRUE;

            if( bEnd )
            {
               if( uiGroup )
                  bEnd = ( SELF_GETVALUE( pArea, uiGroup, pGroupVal ) == SUCCESS );
               else
               {
                  pGroupVal = hb_vmEvalBlock( pGroupBlock );
                  bEnd = TRUE;
               }
            }

            if( bEnd )
            {
               pSubArray = hb_hashGetItemPtr( pHash, pGroupVal, HB_HASH_AUTOADD_ACCESS );

               if( ! pSubArray)
                  break;

               if( HB_IS_NIL( hb_arrayGetItemPtr( pSubArray, 1 ) ) )
                  hb_arraySet( pSubArray, 1, pGroupVal );

               for( uiIndex = 0; uiIndex < uiCount; uiIndex ++ )
               {
                  if( pSumFields[ uiIndex ].Pos < 0 )
                  {
                     hb_arraySetNL( pSubArray, uiIndex + 2, hb_arrayGetNL( pSubArray, uiIndex + 2) + 1 );
                  }
                  else if( pSumFields[ uiIndex ].pBlock )
                  {
                     pNumItem = hb_vmEvalBlock( pSumFields[ uiIndex ].pBlock );
                     if( HB_IS_DOUBLE( hb_arrayGetItemPtr( pSubArray, uiIndex + 2 ) ) || HB_IS_DOUBLE( pNumItem ) )
                     {
                        dValue = hb_itemGetNDDec( pNumItem, &iDec );
                        hb_arraySetND( pSubArray, uiIndex + 2, hb_arrayGetND( pSubArray, uiIndex + 2) + dValue );
                        if( pSumFields[ uiIndex ].Dec < iDec )
                           pSumFields[ uiIndex ].Dec = iDec;
                     }
                     else
                        hb_arraySetNL( pSubArray, uiIndex + 2, hb_arrayGetNL( pSubArray, uiIndex + 2) + hb_itemGetNL( pNumItem ) );
                  }
                  else if( pSumFields[ uiIndex ].Pos > 0 )
                  {
                     if( SELF_GETVALUE( pArea, pSumFields[ uiIndex ].Pos, pItem ) == SUCCESS )
                     {
                        if( HB_IS_DOUBLE( hb_arrayGetItemPtr( pSubArray, uiIndex + 2 ) ) )
                           hb_arraySetND( pSubArray, uiIndex + 2, hb_arrayGetND( pSubArray, uiIndex + 2) + hb_itemGetND( pItem ) );
                        else
                           hb_arraySetNL( pSubArray, uiIndex + 2, hb_arrayGetNL( pSubArray, uiIndex + 2) + hb_itemGetNL( pItem ) );
                     }
                  }
               }
            }
            SELF_SKIP( pArea, 1);
         }

         if( pTopScope )
         {
            leto_ClearFilter( pArea );
            leto_ScopeCommand( pArea, DBOI_SCOPETOPCLEAR, NULL );
            leto_ScopeCommand( pArea, DBOI_SCOPEBOTTOMCLEAR, NULL );
         }
         else
            leto_ClearAreaEnv( pArea, pTag );

         hb_xvmSeqEnd();

         if( pFilterBlock )
         {
            hb_itemClear( pFilterBlock );
            hb_itemRelease( pFilterBlock );
         }
         if( pTopScope )
            hb_itemRelease( pTopScope );
         if( pBottomScope )
            hb_itemRelease( pBottomScope );

         if( pUStru->bHrbError )
         {
            leto_SendAnswer( pUStru, szErr4, 4 );
         }
         else
         {
            ULONG ulLen = hb_hashLen( pHash ), ulSize, ulRow;
            USHORT uiLen;
            char * pData;
            PHB_ITEM pValue;

            if( uiGroup )
            {
               SELF_FIELDINFO( pArea, uiGroup, DBS_LEN, pItem );
               uiLen = hb_itemGetNI( pItem );
            }
            else
            {
               hb_xvmSeqBegin();
               pGroupVal = hb_vmEvalBlock( pGroupBlock );
               hb_xvmSeqEnd();
               if( HB_IS_STRING( pGroupVal ) )
                  uiLen = hb_itemGetCLen( pGroupVal ) + 2;
               else if( HB_IS_DATE( pGroupVal ) )
                  uiLen = 8;
               else if( HB_IS_NUMBER( pGroupVal ) )
                  uiLen = 20;
               else if( HB_IS_LOGICAL( pGroupVal ) )
                  uiLen = 1;
               else
                  uiLen = 0;
            }
            ulSize = ulLen*( uiLen + 1 + uiCount*21 ) + 25;
            pData = hb_xgrab( ulSize );

            sprintf( pData, "+%lu;%d;%c;", ulLen, uiCount, cGroupType );
            ptr = pData + strlen( pData );

            for( ulRow = 1; ulRow <= ulLen; ulRow ++ )
            {
               pSubArray = hb_hashGetValueAt( pHash, ulRow );
               pValue = hb_arrayGetItemPtr( pSubArray, 1 );

               if( HB_IS_STRING( pValue ) )
               {
                  uiLen = hb_itemGetCLen( pValue );
                  HB_PUT_LE_UINT16( ptr, uiLen );
                  ptr += 2;
                  memcpy( ptr, hb_itemGetCPtr( pValue ), uiLen );
                  ptr += uiLen;
               }
               else if( HB_IS_DATE( pValue ) )
               {
                  hb_itemGetDS( pValue, ptr );
                  ptr += 8;
               }
               else if( HB_IS_NUMBER( pValue ) )
               {
                  char * sNum = hb_itemStr( pValue, NULL, NULL );
                  iKeyLen = strlen(sNum);
                  memcpy( ptr, sNum, iKeyLen );
                  hb_xfree( sNum );
                  ptr += iKeyLen;
               }
               else if( HB_IS_LOGICAL( pValue ) )
                 *ptr++ = hb_itemGetL( pValue ) ? 'T' : 'F';

               for( uiIndex = 0; uiIndex < uiCount; uiIndex ++ )
               {
                  pValue = hb_arrayGetItemPtr( pSubArray, uiIndex + 2 );
                  *ptr++ = ',';
                  if( HB_IS_DOUBLE( pValue) )
                  {
                     letoPutDouble( ptr, pItem, hb_itemGetND( pValue ), pSumFields[ uiIndex ].Dec );
                  }
                  else
                     hb_snprintf(ptr, ulSize - (ptr-pData), "%ld", hb_itemGetNL( pValue ) );
                  ptr += strlen(ptr);
               }
               *ptr++ = ';';
            }

            *ptr++ = ';';
            *ptr = '\0';

            leto_SendAnswer( pUStru, pData, ptr - pData );

            hb_xfree( pData );
         }
      }
      else
         leto_SendAnswer( pUStru, szErr2, 4 );

      for( uiIndex = 0; uiIndex < uiCount; uiIndex ++ )
         if( pSumFields[ uiIndex ].pBlock )
         {
            hb_itemClear( pSumFields[ uiIndex ].pBlock );
            hb_itemRelease( pSumFields[ uiIndex ].pBlock );
         }
      hb_xfree( pSumFields );
      hb_itemRelease( pItem );
      if( pGroupBlock )
      {
         hb_itemClear( pGroupBlock );
         hb_itemRelease( pGroupBlock );
      }
      if( uiGroup )
         hb_itemRelease( pGroupVal );
      hb_itemClear( pHash );
      hb_itemRelease( pHash );
      hb_itemRelease( pDefault );

   }

   pUStru->bAnswerSent = 1;
}

void leto_Sum( PUSERSTRU pUStru, const char* szData, USHORT uiDataLen )  // mt?
{
   AREAP pArea;
   const char * pOrder, * pFields, * pFilter, *pFlag;
   int nParam;

   if( !pUStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Sum!\r\n", 0);
      return;
   }

   nParam = leto_GetParam( szData, &pFields, &pFilter, &pFlag, NULL );
   pOrder = szData;

   if( nParam < 4 )
   {
      leto_SendAnswer( pUStru, szErr2, 4 );
   }
   else
   {
      char  szOrder[16];
      typedef struct
      {
         SHORT Pos;
         PHB_ITEM pBlock;
         USHORT Dec;
         union
         {
            double dSum;
            LONG lSum;
         } value;
      } SUMSTRU;

      SUMSTRU * pSums;
      USHORT uiCount = 0, uiAllocated = 10, uiIndex;
      BOOL bEnd = FALSE;
      int iKeyLen, iDec;
      char szFieldName[12];
      PHB_ITEM pItem = hb_itemNew( NULL ), pNumItem;
      PHB_ITEM pFilterBlock = NULL, pValFilter;
      PHB_ITEM pTopScope = NULL;
      PHB_ITEM pBottomScope = NULL;
      BOOL bEof, bDo;
      char *ptr, * pNext;
      double dValue;
      LETOTAG * pTag = NULL;

      pArea = hb_rddGetCurrentWorkAreaPointer();

      pSums = (SUMSTRU *) hb_xgrab( sizeof( SUMSTRU ) * uiAllocated );
      ptr = ( char * ) pFields;
      for( ;; )
      {
         pNext = strchr(ptr, ',');
         if( ! pNext )
         {
            pNext = ptr + strlen(ptr);
            bEnd = TRUE;
         }
         if( pNext-ptr < 12)
         {
            memcpy( szFieldName, ptr, pNext-ptr );
            szFieldName[ pNext-ptr ] = '\0';
         }
         else
            szFieldName[0] = '\0';

         uiCount ++;
         if( uiCount > uiAllocated )
         {
            uiAllocated += 10;
            pSums = (SUMSTRU *) hb_xrealloc( pSums, sizeof( SUMSTRU ) * uiAllocated );
         }

         pSums[ uiCount-1 ].pBlock = NULL;
         if( szFieldName[0] == '#' )
         {
            pSums[ uiCount-1 ].Pos = -1;
            pSums[ uiCount-1 ].Dec = 0;
         }
         else
         {
            if( szFieldName[0] )
               pSums[ uiCount-1 ].Pos = (SHORT) hb_rddFieldIndex( pArea, szFieldName );
            else
               pSums[ uiCount-1 ].Pos = 0;
            if( pSums[ uiCount-1 ].Pos )
            {
               SELF_FIELDINFO( pArea, pSums[ uiCount-1 ].Pos, DBS_DEC, pItem );
               pSums[ uiCount-1 ].Dec = hb_itemGetNI( pItem );
            }
            else if( leto_ExprGetType( pUStru, ptr, pNext-ptr ) == 'N' )
            {
               pSums[ uiCount-1 ].pBlock = leto_mkCodeBlock( ptr, pNext-ptr );
               pSums[ uiCount-1 ].Dec = 0;
            }
            else
            {
               pSums[ uiCount-1 ].Dec = 0;
            }
         }

         if( pSums[ uiCount-1 ].Dec > 0 )
            pSums[ uiCount-1 ].value.dSum = 0.0;
         else
            pSums[ uiCount-1 ].value.lSum = 0;

         if( bEnd ) break;
         ptr = pNext + 1;
      }

      if( strlen(pOrder) < 16 )
         strcpy( szOrder, pOrder );
      else
         szOrder[0] = '\0';

      if( *pFilter != '\0' )
      {
         pFilterBlock = leto_mkCodeBlock( pFilter, strlen(pFilter) );
      }


      ptr = ( char * ) pFlag + 2;
      if( ptr < pOrder + uiDataLen )
      {
         iKeyLen = (((int)*ptr++) & 0xFF);

         if( iKeyLen )
         {
            pTopScope = leto_KeyToItem( pArea, ptr, iKeyLen, szOrder );
            ptr += iKeyLen;
            iKeyLen = (((int)*ptr++) & 0xFF);
            if( iKeyLen && ptr < pOrder + uiDataLen )
            {
               pBottomScope = leto_KeyToItem( pArea, ptr, iKeyLen, szOrder );
            }
         }
      }

      hb_xvmSeqBegin();

      if( szOrder[0] != '\0' )
         leto_SetFocus( pArea, szOrder );

      if( pTopScope )
      {
         leto_ScopeCommand( pArea, DBOI_SCOPETOP, pTopScope );
         leto_ScopeCommand( pArea, DBOI_SCOPEBOTTOM, (pBottomScope ? pBottomScope : pTopScope ) );
         hb_setSetDeleted( *pFlag == 0x41 );
         leto_SetFilter( pUStru, pUStru->pCurAStru, pArea );
      }
      else
         leto_SetAreaEnv( pUStru, pUStru->pCurAStru, pArea, *pFlag == 0x41, szOrder, &pTag, TRUE );

      SELF_GOTOP( pArea );
      for( ;; )
      {
         SELF_EOF( pArea, &bEof );
         if( bEof ) break;
         if( pFilterBlock )
         {
            pValFilter = hb_vmEvalBlock( pFilterBlock );
            bDo = HB_IS_LOGICAL( pValFilter ) && hb_itemGetL( pValFilter );
         }
         else
            bDo = TRUE;
         if( bDo )
            for( uiIndex = 0; uiIndex < uiCount; uiIndex ++ )
            {
               if( pSums[ uiIndex ].Pos < 0 )
               {
                  pSums[ uiIndex ].value.lSum ++;
               }
               else if( pSums[ uiIndex ].pBlock )
               {
                  pNumItem = hb_vmEvalBlock( pSums[ uiIndex ].pBlock );
                  if( pSums[ uiIndex ].Dec > 0 || HB_IS_DOUBLE( pNumItem ) )
                  {
                     dValue = hb_itemGetNDDec( pNumItem, &iDec );
                     if( pSums[ uiIndex ].Dec == 0 )
                     {
                        pSums[ uiIndex ].Dec = (iDec) ? iDec : 2;
                        if( uiIndex > 0 )
                           pSums[ uiIndex ].value.dSum = (double) pSums[ uiIndex ].value.lSum;
                     }
                     pSums[ uiIndex ].value.dSum += dValue;
                  }
                  else
                     pSums[ uiIndex ].value.lSum += hb_itemGetNL( pNumItem );
               }
               else if( pSums[ uiIndex ].Pos > 0 )
               {
                  if( SELF_GETVALUE( pArea, pSums[ uiIndex ].Pos, pItem ) == SUCCESS )
                  {
                     if( pSums[ uiIndex ].Dec > 0 )
                     {
                        pSums[ uiIndex ].value.dSum += hb_itemGetND( pItem );
                     }
                     else
                     {
                        pSums[ uiIndex ].value.lSum += hb_itemGetNL( pItem );
                     }
                  }
               }
            }
         SELF_SKIP( pArea, 1);
      }

      if( pTopScope )
      {
         leto_ClearFilter( pArea );
         leto_ScopeCommand( pArea, DBOI_SCOPETOPCLEAR, NULL );
         leto_ScopeCommand( pArea, DBOI_SCOPEBOTTOMCLEAR, NULL );
      }
      else
         leto_ClearAreaEnv( pArea, pTag );

      hb_xvmSeqEnd();

      if( pFilterBlock )
      {
         hb_itemClear( pFilterBlock );
         hb_itemRelease( pFilterBlock );
      }
      if( pTopScope )
         hb_itemRelease( pTopScope );
      if( pBottomScope )
         hb_itemRelease( pBottomScope );

      if( pUStru->bHrbError )
      {
         leto_SendAnswer( pUStru, szErr4, 4 );
      }
      else
      {
         char * pData = hb_xgrab( uiCount*21 + 3 );

         pData[0] = '+';
         ptr = pData + 1;
         for( uiIndex = 0; uiIndex < uiCount; uiIndex ++ )
         {
            if( ptr > pData + 1)
               *ptr++ = ',';
            if( pSums[ uiIndex ].Dec > 0 )
            {
               letoPutDouble( ptr, pItem, pSums[ uiIndex ].value.dSum, pSums[ uiIndex ].Dec );
            }
            else
            {
               hb_snprintf(ptr, uiCount*21 + 3 - (ptr-pData), "%ld", pSums[ uiIndex ].value.lSum);
            }
            ptr += strlen(ptr);
         }
         *ptr++ = ';';
         *ptr = '\0';

         leto_SendAnswer( pUStru, pData, strlen( pData ) );

         hb_xfree( pData );
      }
      for( uiIndex = 0; uiIndex < uiCount; uiIndex ++ )
      {
         if( pSums[ uiIndex ].pBlock )
         {
            hb_itemClear( pSums[ uiIndex ].pBlock );
            hb_itemRelease( pSums[ uiIndex ].pBlock );
         }
      }
      hb_xfree( (char *) pSums );
      hb_itemRelease( pItem );
   }

   pUStru->bAnswerSent = 1;
}

char * letoFillTransInfo( LPDBTRANSINFO pTransInfo, const char * pData, AREAP pAreaSrc, AREAP pAreaDst )
{
   char * pp1, * pp2;
   ULONG ulTemp;
   USHORT uiIndex;

   memset( pTransInfo, 0, sizeof( DBTRANSINFO ) );

   pTransInfo->lpaSource    = pAreaSrc;
   pTransInfo->lpaDest      = pAreaDst;

   pp1 = (char *) pData;
   if( pp1 )
   {
      if( *pp1 == ';')
         pp1 ++;
      else if( ( pp2 = strchr( pp1 + 1, ';' ) ) != NULL )
      {
         pTransInfo->dbsci.itmCobFor   = leto_mkCodeBlock( pp1, pp2 - pp1 );
         pTransInfo->dbsci.lpstrFor    = NULL;
         pp1 = pp2 + 1;
      }
   }

   if( pp1 )
   {
      if( *pp1 == ';')
         pp1 ++;
      else if( ( pp2 = strchr( pp1 + 1, ';' ) ) != NULL )
      {
         pTransInfo->dbsci.itmCobWhile = leto_mkCodeBlock( pp1, pp2 - pp1 );
         pTransInfo->dbsci.lpstrWhile  = NULL;
         pp1 = pp2 + 1;
      }
   }

   if( pp1 && ( pp2 = strchr( pp1, ';' ) ) != NULL )
   {
      sscanf( pp1, "%lu;", &ulTemp );
      if( ulTemp )
         pTransInfo->dbsci.lNext       = hb_itemPutNL( NULL, ulTemp );
      pp1 = pp2 + 1;
   }

   if( pp1 && ( pp2 = strchr( pp1, ';' ) ) != NULL )
   {
      sscanf( pp1, "%lu;", &ulTemp );
      if( ulTemp )
         pTransInfo->dbsci.itmRecID    = hb_itemPutNL( NULL, ulTemp );
      pp1 = pp2 + 1;
   }
   if( pp1 && ( pp2 = strchr( pp1, ';' ) ) != NULL )
   {
      pTransInfo->dbsci.fRest       = hb_itemPutL( NULL, ( *pp1 == 'T' ) );
      pp1 = pp2 + 1;
   }

   if( pp1 && ( pp2 = strchr( pp1, ';' ) ) != NULL )
   {
      pTransInfo->dbsci.fIgnoreFilter     = ( *pp1 == 'T' );
      pp1 = pp2 + 1;
   }
   if( pp1 && ( pp2 = strchr( pp1, ';' ) ) != NULL )
   {
      pTransInfo->dbsci.fIncludeDeleted   = ( *pp1 == 'T' );
      pp1 = pp2 + 1;
   }
   if( pp1 && ( pp2 = strchr( pp1, ';' ) ) != NULL )
   {
      pTransInfo->dbsci.fLast             = ( *pp1 == 'T' );
      pp1 = pp2 + 1;
   }
   if( pp1 && ( pp2 = strchr( pp1, ';' ) ) != NULL )
   {
      pTransInfo->dbsci.fIgnoreDuplicates = ( *pp1 == 'T' );
      pp1 = pp2 + 1;
   }
   if( pp1 && ( pp2 = strchr( pp1, ';' ) ) != NULL )
   {
      pTransInfo->dbsci.fBackward         = ( *pp1 == 'T' );
      pp1 = pp2 + 1;
   }
   if( pp1 && ( pp2 = strchr( pp1, ';' ) ) != NULL )
   {
      pTransInfo->dbsci.fOptimized = ( *pp1 == 'T' );
      pp1 = pp2 + 1;
   }
   if( pp1 && ( pp2 = strchr( pp1, ';' ) ) != NULL )
   {
      sscanf( pp1, "%hu;", &pTransInfo->uiFlags );
      pp1 = pp2 + 1;
   }
   if( pp1 && ( pp2 = strchr( pp1, ';' ) ) != NULL )
   {
      sscanf( pp1, "%hu;", &pTransInfo->uiItemCount );
      pp1 = pp2 + 1;
   }

   if( pp1 && pTransInfo->uiItemCount )
   {
      pTransInfo->lpTransItems = ( LPDBTRANSITEM )
         hb_xgrab( pTransInfo->uiItemCount * sizeof( DBTRANSITEM ) );
      for( uiIndex=0; uiIndex < pTransInfo->uiItemCount && pp1 &&
           ( pp2 = strchr( pp1, ';' ) ) != NULL; uiIndex++ )
      {
         sscanf(pp1, "%hu,%hu;",
                        &pTransInfo->lpTransItems[uiIndex].uiSource,
                        &pTransInfo->lpTransItems[uiIndex].uiDest );
         pp1 = pp2 + 1;
      }
   }

   return pp1;
}

static void letoFreeTransInfo( LPDBTRANSINFO pTransInfo )
{
   if( pTransInfo->dbsci.itmCobFor )
      hb_itemRelease( pTransInfo->dbsci.itmCobFor );
   if( pTransInfo->dbsci.itmCobWhile )
      hb_itemRelease( pTransInfo->dbsci.itmCobWhile );
   if( pTransInfo->dbsci.lNext )
      hb_itemRelease( pTransInfo->dbsci.lNext );
   if( pTransInfo->dbsci.itmRecID )
      hb_itemRelease( pTransInfo->dbsci.itmRecID );
   if( pTransInfo->dbsci.fRest )
      hb_itemRelease( pTransInfo->dbsci.fRest );

}

void leto_Trans( PUSERSTRU pUStru, const char* szData, BOOL bSort )
{
   PAREASTRU pAStru;
   AREAP pAreaSrc, pAreaDst;
   ULONG ulAreaDst, ulRecNo;
   const char * pp1, * pp2, * pp3, * pp4;
   char szOrder[16];
   BOOL bDeleted;
   int nParam;

   if( !pUStru || !pUStru->pCurAStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Trans!\r\n", 0);
      return;
   }

   pAStru = pUStru->pCurAStru;
   nParam = leto_GetParam( szData, &pp1, &pp2, &pp3, NULL );

   if( nParam > 3 )
   {
      sscanf( szData, "%lu", &ulRecNo );
      strcpy( szOrder, pp1 );
      bDeleted = (*pp2 == 'T');
      sscanf( pp3, "%lu", &ulAreaDst );
      pp4 = pp3 + strlen( pp3 ) + 1;

      pAreaSrc = hb_rddGetCurrentWorkAreaPointer();
      pAreaDst = leto_SelectArea( pUStru, ulAreaDst );

      if( pAreaDst )
      {
         LETOTAG * pTag = NULL;

         hb_xvmSeqBegin();
         if( bSort )
         {
            DBSORTINFO dbSortInfo;

            pp1 = letoFillTransInfo( &dbSortInfo.dbtri, pp4, pAreaSrc, pAreaDst );

            if( pp1 && ( pp2 = strchr( pp1, ';' ) ) != NULL )
            {
               sscanf( pp1, "%hu;", &dbSortInfo.uiItemCount );
               pp1 = pp2 + 1;
            }
            else
            {
               dbSortInfo.uiItemCount = 0;
               dbSortInfo.lpdbsItem = NULL;
            }

            if( pp1 && dbSortInfo.uiItemCount )
            {
               USHORT uiIndex;

               dbSortInfo.lpdbsItem = ( LPDBSORTITEM )
                  hb_xgrab( dbSortInfo.uiItemCount * sizeof( DBSORTITEM ) );
               for( uiIndex=0; uiIndex < dbSortInfo.uiItemCount && pp1 &&
                  ( pp2 = strchr( pp1, ';' ) ) != NULL; uiIndex++ )
               {
                  sscanf(pp1, "%hu,%hu;",
                               &dbSortInfo.lpdbsItem[uiIndex].uiField,
                               &dbSortInfo.lpdbsItem[uiIndex].uiFlags );
                  pp1 = pp2 + 1;
               }
            }

            leto_SetAreaEnv( pUStru, pAStru, pAreaSrc, bDeleted, szOrder, &pTag, TRUE );
            leto_GotoIf( pAreaSrc, ulRecNo );
            SELF_SORT( pAreaSrc, &dbSortInfo );
            leto_ClearAreaEnv( pAreaSrc, pTag );

            letoFreeTransInfo( &dbSortInfo.dbtri );

            if( dbSortInfo.lpdbsItem )
               hb_xfree( dbSortInfo.lpdbsItem );

         }
         else
         {
            DBTRANSINFO dbTransInfo;

            letoFillTransInfo( &dbTransInfo, pp4, pAreaSrc, pAreaDst );

            leto_SetAreaEnv( pUStru, pAStru, pAreaSrc, bDeleted, szOrder, &pTag, TRUE );
            leto_GotoIf( pAreaSrc, ulRecNo );
            SELF_TRANS( dbTransInfo.lpaSource, &dbTransInfo );
            leto_ClearAreaEnv( pAreaSrc, pTag );

            letoFreeTransInfo( &dbTransInfo );
         }
         hb_xvmSeqEnd();

         if( ! bSort && leto_CheckClientVer( pUStru, 216 ) )
         {
            ULONG ulLen = leto_recLen( pAreaDst );
            char * szData1 = (char *) hb_xgrab( ulLen + 6 );
            PAREASTRU pAStruDst = leto_FindArea( pUStru, ulAreaDst );

            memcpy( szData1, szOk, 4);
            szData1[4] = ';';
            ulLen = (USHORT) leto_rec( pUStru, pAStruDst, pAreaDst, szData1 + 5, ulLen ) + 5;

            leto_SendAnswer( pUStru, szData1, ulLen );
            hb_xfree( szData1 );
         }
         else
         {
            leto_SendAnswer( pUStru, szOk, 4 );
         }
      }
      else
      {
         leto_SendAnswer( pUStru, szErr2, 4 );
      }
   }
   else
   {
      leto_SendAnswer( pUStru, szErr2, 4 );
   }

   pUStru->bAnswerSent = 1;
}

void leto_runFunc( PUSERSTRU pUStru, PHB_DYNS* ppSym, const char* szName, const char* pCommand, ULONG ulLen )  // mt
{
   if( !( *ppSym ) )
      *ppSym = hb_dynsymFindName( szName );

   if( *ppSym )
   {
      hb_vmPushSymbol( hb_dynsymSymbol( *ppSym ) );
      hb_vmPushNil();
      if( pUStru )
      {
         hb_vmPushLong( (LONG ) ( pUStru->iUserStru ) );
         hb_vmPushString( (char*)pCommand, ulLen );
         hb_vmDo( 2 );
         leto_SendAnswer( pUStru, hb_parc( -1 ), hb_parclen( -1 ) );
         pUStru->bAnswerSent = 1;
      }
      else
         hb_vmDo( 0 );
   }
}

void leto_udf( PUSERSTRU pUStru, const char* szData )  // mt?
{
   const char * pp2 = NULL, * pp3 = NULL, * pp4 = NULL;
   int nParam;
   USHORT uiCommand = 0;

   if( !pUStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_udf!\r\n", 0);
      return;
   }

   if( szData[0] == '0' )
   {
      if( szData[1] == '1' )
         uiCommand = 1;
      else if( szData[1] == '2' )
         uiCommand = 2;
      else if( szData[1] == '3' )
         uiCommand = 3;
   }

   if( uiCommand == 0 )
   {
      leto_wUsLog(pUStru,"ERROR! UDF: not a valid command\r\n", 0);
      return;
   }

   if( uiUDF == 0 || leto_WriteDeny( pUStru ) )
   {
      leto_SendAnswer( pUStru, szErrAcc, 4 );
      pUStru->bAnswerSent = 1;
      return;
   }

   nParam = leto_GetParam( szData, &pp2, &pp3, NULL, NULL );

   hb_strUpper( (char *) pp2, strlen(pp2) );
   if( ( uiUDF == 1 ) && ( strlen( pp2 ) < 5 || hb_strnicmp( pp2, "UDF_", 4 ) != 0 ) )
   {
      leto_wUsLog(pUStru,"ERROR! UDF: not a valid name\r\n", 0);

      leto_SendAnswer( pUStru, szErr4, 4 );
      pUStru->bAnswerSent = 1;

      return;
   }

   if( nParam < ( uiCommand == 1 ? 3 : ( uiCommand == 3 ? 1 : 2 ) ) )
      leto_SendAnswer( pUStru, szErr2, 4 );
   else
   {
      PHB_DYNS pSym = hb_dynsymFindName( pp2 );

      if( uiCommand == 3 ) /* leto_udfexist */
      {
         char szData1[16];
         hb_snprintf( szData1, 16, "+%s;", ( pSym ) ? "T" : "F" );
         leto_SendAnswer( pUStru, szData1, strlen( szData1 ) );
      }
      else if( pSym )
      {
         HB_SIZE nSize;
         PHB_ITEM pArray = NULL;
         USHORT uiPCount = 1;

         sscanf( pp3, "%" HB_PFS "u;", &nSize );
         if( nSize )
         {
            pp4 = pp3 + strlen(pp3) + 1;
            pArray = hb_itemDeserialize( &pp4, &nSize );
         }

         hb_xvmSeqBegin();
         hb_vmPushSymbol( hb_dynsymSymbol( pSym ) );
         hb_vmPushNil();
         hb_vmPushLong( (LONG ) ( pUStru->iUserStru ) );
         if( HB_IS_ARRAY( pArray ) )
         {
            USHORT uiALen = (USHORT) hb_arrayLen( pArray ), uiIndex;
            for( uiIndex = 1; uiIndex <= uiALen; uiIndex ++ )
               hb_vmPush( hb_arrayGetItemPtr( pArray, uiIndex ) );
            uiPCount += uiALen;
         }
         hb_vmDo( uiPCount );

         if( pp4 )
            hb_itemRelease( pArray );
         hb_xvmSeqEnd();

         if( !pUStru->bHrbError )
         {
            char * pParam = hb_itemSerialize( hb_param( -1, HB_IT_ANY ), TRUE, &nSize );
            ULONG ulLen = leto_CryptText( pUStru, pParam, nSize );
            hb_xfree( pParam );
            leto_SendAnswer( pUStru, (char*) pUStru->pBufCrypt, ulLen );
         }
         else
            leto_SendAnswer( pUStru, szErr4, 4 );
      }
      else
      {
         leto_SendAnswer( pUStru, szErr2, 4 );
      }
   }
   pUStru->bAnswerSent = 1;
}

void leto_Info( PUSERSTRU pUStru, const char* szData )
{
   const char * pp1;
   int nParam;
   int iCommand;

   if( !pUStru || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_Info!\r\n", 0);
      return;
   }

   nParam = leto_GetParam( szData, &pp1, NULL, NULL, NULL );

   if( nParam < 1 )
      leto_SendAnswer( pUStru, szErr3, 4 );
   else
   {
      AREAP pArea = hb_rddGetCurrentWorkAreaPointer();

      sscanf( szData, "%d", &iCommand );

      switch( iCommand )
      {
         case DBI_TRIGGER:
         {
            PHB_ITEM pItem = hb_itemNew( NULL );
            char szData1[20];
            int iLen;
            if( *pp1 && bSetTrigger )
            {
               if( *pp1 == '.')
                  hb_itemPutL( pItem, (*(pp1+1) == 'T') );
               else
                  hb_itemPutC( pItem, pp1 );
            }
            SELF_INFO( pArea, DBI_TRIGGER, pItem );
            szData1[0] = '+';
            iLen = hb_itemGetCLen( pItem );
            if( iLen > 17 )
               iLen = 17;
            memcpy(szData1+1, hb_itemGetCPtr( pItem ), iLen );
            szData1[ iLen + 1 ] = ';';
            szData1[ iLen + 2 ] = '\0';

            leto_SendAnswer( pUStru, szData1, 0 );
            hb_itemRelease( pItem );
            break;
         }
         default:
         {
            leto_SendAnswer( pUStru, szErr4, 4 );
         }
      }
   }

   pUStru->bAnswerSent = 1;
}

void leto_OrderInfo( PUSERSTRU pUStru, const char* szData )
{
   AREAP pArea;
   const char * pOrder, * pOrdPar;
   const char * pData;
   char szOrder[16];
   int nParam;
   int iCommand;
   char szData1[20];

   if( !pUStru || !pUStru->pCurAStru < 0 || !szData )
   {
      leto_wUsLog(pUStru,"ERROR! leto_OrderInfo!\r\n", 0);
      return;
   }

   nParam = leto_GetParam( szData, &pOrder, &pOrdPar, NULL, NULL );

   if( nParam < 2 )
      pData = szErr2;
   else
   {
      sscanf( szData, "%d", &iCommand );
      strcpy( szOrder, pOrder );

      if( !iCommand || !szOrder[0] )
         pData = szErr3;
      else
      {
         DBORDERINFO pOrderInfo;

         pArea = hb_rddGetCurrentWorkAreaPointer();

         memset( &pOrderInfo, 0, sizeof( DBORDERINFO ) );
         pOrderInfo.itmOrder = hb_itemPutC( NULL, szOrder );
         switch( iCommand )
         {
            case DBOI_ISDESC:
            case DBOI_CUSTOM:
            case DBOI_UNIQUE:
               pOrderInfo.itmResult = hb_itemPutL( NULL, FALSE );
               if( nParam > 3 && pOrdPar[0] )
                  pOrderInfo.itmNewVal = hb_itemPutL( NULL, ( pOrdPar[0] == 'T' ) );
               break;
            case DBOI_KEYADD:
            case DBOI_KEYDELETE:
            {
               ULONG ulRecNo;
               int iKeyLen;
               const char * pOrdKey;

               sscanf( pOrdPar, "%lu", &ulRecNo );
               leto_GotoIf( pArea, ulRecNo );

               pOrdKey = pOrdPar + strlen(pOrdPar) + 1;       // binary data
               iKeyLen = (((int)*pOrdKey) & 0xFF);
               if( iKeyLen )
               {
                  ++ pOrdKey;
                  pOrderInfo.itmNewVal = leto_KeyToItem( pArea, pOrdKey, iKeyLen, szOrder );
               }

               break;
            }
/*
            case DBOI_FINDREC:
            case DBOI_FINDRECCONT:
            case DBOI_KEYGOTO:
            case DBOI_KEYGOTORAW:
            case DBOI_RELKEYPOS:
               pOrderInfo.itmResult = hb_itemPutNL( NULL, 0 );
               if( nParam > 3 && pOrdPar[0] )
               {
                  LONG lNewVal;
                  sscanf( szData, "%ld", &lNewVal );
                  pOrderInfo.itmNewVal = hb_itemPutNL( NULL, lNewVal );
               }
               break;
*/
            default:
               pOrderInfo.itmResult = hb_itemPutL( NULL, FALSE );
               if( nParam > 3 && pOrdPar[0] )
                  pOrderInfo.itmNewVal = hb_itemPutC( NULL, pOrdPar );
               break;
         }
         SELF_ORDINFO( pArea, iCommand, &pOrderInfo );
         szData1[0] = '+';
         szData1[1] = hb_itemGetL( pOrderInfo.itmResult ) ? 'T' : 'F';
         szData1[2] = ';';
         szData1[3] = '\0';
         hb_itemClear( pOrderInfo.itmResult );
         pData = szData1;
      }
   }

   leto_SendAnswer( pUStru, pData, strlen(pData) );
   pUStru->bAnswerSent = 1;

}

BOOL ParseCommand( PUSERSTRU pUStru )
{
   const char * ptr;
   char * ptrPar, * ptr2;
   ULONG ulLen;
   BOOL lRequest = TRUE;
   ULONG ulAreaID;

   static PHB_DYNS pSym_Fopent  = NULL;
   static PHB_DYNS pSym_Fopeni  = NULL;
   static PHB_DYNS pSym_Fcreat  = NULL;
   static PHB_DYNS pSym_Fcreai  = NULL;
   static PHB_DYNS pSym_Freload = NULL;

   pUStru->bHrbError = pUStru->bAnswerSent = 0;
   ptr   = ( const char * ) pUStru->pBufRead;
   ulLen = pUStru->ulDataLen;

   if( ( ptrPar = strchr( ptr, ';' ) ) == NULL )
      return FALSE;

   if( iDebugMode >= 10 )
      leto_wUsLog( pUStru, ptr, ulLen );

   *ptrPar = '\0';
   ++ptrPar;
   ulLen -= ptrPar - ptr;

   switch( *ptr )
   {
      case 'i':
         // intro
         if( !strcmp( ptr,"intro" ) )
         {
            leto_Intro( pUStru, ptrPar );
            lRequest = FALSE;
         }
         break;
      case 'v':
         // var
         if( !strcmp( ptr,"var" ) )
         {
            hb_threadEnterCriticalSection( &mutex_VARS );
            leto_Variables( pUStru, ptrPar );
            hb_threadLeaveCriticalSection( &mutex_VARS );
            lRequest = FALSE;
         }
         break;
      case 's':
         // set
         if( !strcmp( ptr,"set" ) )
         {
            leto_Set( pUStru, ptrPar );
            lRequest = FALSE;
         }
         else if( !strcmp( ptr,"stop" ) )
         {
            leto_GlobalExit = 1;
            pUStru->bAnswerSent = 1;
            lRequest = FALSE;
         }
         break;
      case 'a':
         // admin
         if( !strcmp( ptr,"admin" ) )
         {
            leto_Admin( pUStru, ptrPar );
            lRequest = FALSE;
         }
         break;
      case 'u':
         // udf
         if( !strcmp( ptr,"udf_fun" ) )
         {
            leto_udf( pUStru, ptrPar );
            lRequest = FALSE;
         }
         else if( !strcmp( ptr,"udf_rel" ) )
         {
            leto_runFunc( NULL, &pSym_Freload, "HS_UDFRELOAD", NULL, 0 );
            pUStru->bAnswerSent = 1;
            lRequest = FALSE;
         }
         break;
      case 'f':
         // file
         if( !strcmp( ptr,"file" ) )
         {
            leto_filef( pUStru, ptrPar );
            lRequest = FALSE;
         }
         break;
      case 'o':
         // open
         if( !strcmp( ptr,"open" ) )
         {
            hb_threadEnterCriticalSection( &mutex_Open );
            leto_runFunc( pUStru, &pSym_Fopent, "HS_OPENTABLE", ptrPar, ulLen );
            hb_threadLeaveCriticalSection( &mutex_Open );
            lRequest = FALSE;
         }
         break;
      case 'c':
         // creat, close_all, creat_i, close
         if( !strcmp( ptr,"creat" ) )
         {
            if( leto_IsServerLock( pUStru ) )
            {
               leto_SendAnswer( pUStru, szErr4, 4 );
               pUStru->bAnswerSent = 1;
            }
            else if( leto_WriteDeny( pUStru ) )
            {
               leto_SendAnswer( pUStru, szErrAcc, 4 );
               pUStru->bAnswerSent = 1;
            }
            else
            {
               hb_threadEnterCriticalSection( &mutex_Open );
               leto_runFunc( pUStru, &pSym_Fcreat, "HS_CREATETABLE", ptrPar, ulLen );
               hb_threadLeaveCriticalSection( &mutex_Open );
            }
            lRequest = FALSE;
         }
         else if( !strcmp( ptr,"close_all" ) )
         {
            hb_threadEnterCriticalSection( &mutex_Open );
            leto_CloseAll4Us( pUStru );
            hb_threadLeaveCriticalSection( &mutex_Open );
            lRequest = FALSE;
         }
         if( !strcmp( ptr,"close" ) )
         {
            hb_threadEnterCriticalSection( &mutex_Open );
            leto_CloseT( pUStru, ptrPar );
            hb_threadLeaveCriticalSection( &mutex_Open );
            lRequest = FALSE;
         }
         break;
      case 'p':
         // ping
         if( !strcmp( ptr,"ping" ) )
         {
            leto_SendAnswer( pUStru, "pong;", 5 );
            pUStru->bAnswerSent = 1;
            lRequest = FALSE;
         }
         break;
      case 't':
         // transaction
         if( !strcmp( ptr,"ta" ) )
         {
            leto_Transaction( pUStru, ptrPar, ulLen );
            lRequest = FALSE;
         }
         break;
      case 'm':
         if( !strcmp( ptr,"mgmt" ) )
         {
            hb_threadEnterCriticalSection( &mutex_Open );
            leto_Mgmt( pUStru, ptrPar );
            hb_threadLeaveCriticalSection( &mutex_Open );
            lRequest = FALSE;
         }
         break;
      case 'q':
         // quit
         if( !strcmp( ptr,"quit" ) )
         {
//            hb_threadEnterCriticalSection( &mutex_Open );
            leto_CloseUS( pUStru );
//            hb_threadLeaveCriticalSection( &mutex_Open );
            pUStru->bAnswerSent = 1;
            lRequest = FALSE;
         }
         break;
      case 'd':
         // DbDrop
         if( !strcmp( ptr,"drop" ) )
         {
            hb_threadEnterCriticalSection( &mutex_Open );
            leto_Drop( pUStru, ptrPar );
            hb_threadLeaveCriticalSection( &mutex_Open );
            lRequest = FALSE;
         }
         break;
      case 'e':
         // DbExists
         if( !strcmp( ptr,"exists" ) )
         {
            hb_threadEnterCriticalSection( &mutex_Open );
            leto_Exists( pUStru, ptrPar );
            hb_threadLeaveCriticalSection( &mutex_Open );
            lRequest = FALSE;
         }
         break;
      case 'r':
         // DbRename
         if( !strcmp( ptr,"rename" ) )
         {
            hb_threadEnterCriticalSection( &mutex_Open );
            leto_Rename( pUStru, ptrPar );
            hb_threadLeaveCriticalSection( &mutex_Open );
            lRequest = FALSE;
         }
         break;
   }

   if( lRequest )
   {
      ptr2 = ptrPar;
      sscanf( ptrPar, "%lu;", &ulAreaID );
      if( !ulAreaID || ( ptrPar = strchr( ptr2, ';' ) ) == NULL )
         return FALSE;
      *ptrPar = '\0';
      ++ptrPar;
      ulLen -= ptrPar - ptr2;

      if ( leto_SelectArea( pUStru, ulAreaID ) == NULL )
         return FALSE;

      switch( *ptr )
      {
         case 's':
            // scop, seek, set, skip, sort
            switch( *(ptr+1) )
            {
               case 'k':
                  if( !strcmp( ptr,"skip" ) )
                     leto_Skip( pUStru, ptrPar );
                  break;
               case 'e':
                  if( !strcmp( ptr,"seek" ) )
                     leto_Seek( pUStru, ptrPar );
                  else if( !strcmp( ptr,"setf" ) )
                     leto_Filter( pUStru, ptrPar, ulLen );
                  break;
               case 'c':
                  if( !strcmp( ptr,"scop" ) )
                     leto_Scope( pUStru, ptrPar );
                  break;
               case 'u':
                  if( !strcmp( ptr,"sum" ) )
                     leto_Sum( pUStru, ptrPar, ulLen );
                  break;
               case 'o':
                  if( !strcmp( ptr,"sort" ) )
                     leto_Trans( pUStru, ptrPar, TRUE );
                  break;
            }
            break;
         case 'g':
            // goto
            if( !strcmp( ptr,"goto" ) )
               leto_Goto( pUStru, ptrPar );
            else if( !strcmp( ptr,"group" ) )
               leto_GroupBy( pUStru, ptrPar, ulLen );
            break;
         case 'u':
            // upd, unlock, usr
            if( !strcmp( ptr,"upd" ) )
               leto_UpdateRec( pUStru, ptrPar, FALSE, FALSE );
            else if( !strcmp( ptr,"unlock" ) )
               leto_Unlock( pUStru, ptrPar );
            else if( !strcmp( ptr,"udf_dbf" ) )
               leto_udf( pUStru, ptrPar );
            break;
         case 'a':
            // add
            if( !strcmp( ptr,"add" ) )
               leto_UpdateRec( pUStru, ptrPar, TRUE, FALSE );
            break;
         case 'd':
            // dboi, dbi
            if( !strcmp( ptr,"dboi" ) )
               leto_OrderInfo( pUStru, ptrPar );
            else if( !strcmp( ptr,"dbi" ) )
               leto_Info( pUStru, ptrPar );
            break;
         case 'r':
            // rcou, ?rel_set,rel_clr
            if( !strcmp( ptr,"rcou" ) )
               leto_Reccount( pUStru, ptrPar );
            break;
         case 'f':
            // flush
            if( !strcmp( ptr,"flush" ) )
               leto_Flush( pUStru, ptrPar );
            break;
         case 'l':
            // lock
            if( !strcmp( ptr,"lock" ) )
               leto_Lock( pUStru, ptrPar );
            break;
         case 'i':
            // islock
            if( !strcmp( ptr,"islock" ) )
               leto_IsRecLockedUS( pUStru, ptrPar );
            break;
         case 'm':
            // memo, mgmt
            if( !strcmp( ptr,"memo" ) )
               leto_Memo( pUStru, ptrPar, NULL, NULL );
            break;
         case 'o':
            // ord, open_i
            if( !strcmp( ptr,"ord" ) )
               leto_Ordfunc( pUStru, ptrPar );
            else if( !strcmp( ptr,"open_i" ) )
               leto_runFunc( pUStru, &pSym_Fopeni, "HS_OPENINDEX", ptrPar, ulLen );
            break;
         case 'c':
            // commit(+add,+upd), creat_i
            if( !strcmp( ptr,"creat_i" ) )
            {
               if( leto_IsServerLock( pUStru ) )
               {
                  leto_SendAnswer( pUStru, szErr4, 4 );
                  pUStru->bAnswerSent = 1;
               }
               else if( leto_WriteDeny( pUStru ) )
               {
                  leto_SendAnswer( pUStru, szErrAcc, 4 );
                  pUStru->bAnswerSent = 1;
               }
               else
               {
                  leto_runFunc( pUStru, &pSym_Fcreai, "HS_CREATEINDEX", ptrPar, ulLen );
               }
            }
            else if( !strcmp( ptr,"cmta" ) )
               leto_UpdateRec( pUStru, ptrPar, TRUE, TRUE );
            else if( !strcmp( ptr,"cmtu" ) )
               leto_UpdateRec( pUStru, ptrPar, FALSE, TRUE );
            break;
         case 'p':
            // pack
            if( !strcmp( ptr,"pack" ) )
               leto_Pack( pUStru, ptrPar );
            break;
         case 'z':
            // zap
            if( !strcmp( ptr,"zap" ) )
               leto_Zap( pUStru, ptrPar );
            break;
         case 't':
            // trans
            if( !strcmp( ptr,"trans" ) )
               leto_Trans( pUStru, ptrPar, FALSE );
            break;
      }
   }
   leto_FreeArea( pUStru, FALSE );

   if( !pUStru->bAnswerSent )
      return FALSE;

   return TRUE;
}
