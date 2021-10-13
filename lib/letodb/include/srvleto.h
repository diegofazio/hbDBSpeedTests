/*  $Id$  */

/*
 * Harbour Project source code:
 * Header file for Leto DB Server
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

#define HB_EXTERNAL_RDDDBF_USE
#include "hbthread.h"
#include "hbsocket.h"
#include "hbrdddbf.h"
// #include "hbapirdd.h"
#include "funcleto.h"
#include "rddleto.ch"


/*
#if defined( HB_OS_WIN_32 ) || defined( HB_OS_WIN )
   #include "windows.h"
   #include <process.h>

   #define  LETO_MUTEX   CRITICAL_SECTION

   typedef struct _LETO_COND_
   {
     HANDLE   hEvent;
     LONG     ulLocked;
   } LETO_COND;

   #define LETO_THREAD_FUNC  unsigned __stdcall

   extern BOOL leto_ThreadCreate( unsigned int (__stdcall *ThreadFunc)(void*) );
#else
   #define _MULTI_THREADED
   #include <pthread.h>

   #define  LETO_MUTEX   pthread_mutex_t

   typedef struct _LETO_COND_
   {
     pthread_cond_t  cond;
     pthread_mutex_t mutex;
     BOOL     bLocked;
   } LETO_COND;

   #define LETO_THREAD_FUNC  void *
   extern BOOL leto_ThreadCreate( void* (*ThreadFunc)(void*) );
#endif
*/

typedef struct _LETO_LIST_ITEM
{
   struct _LETO_LIST_ITEM * pNext;
} LETO_LIST_ITEM, *PLETO_LIST_ITEM;

typedef struct _LETO_LOCK_ITEM
{
   struct _LETO_LOCK_ITEM * pNext;
   ULONG ulRecNo;
} LETO_LOCK_ITEM, *PLETO_LOCK_ITEM;

typedef struct
{
   PHB_ITEM        pMutex;
   ULONG           ulSize;
   PLETO_LIST_ITEM pItem;
} LETO_LIST, *PLETO_LIST;

typedef struct _VAR_LINK
{
   USHORT      uiGroup;
   USHORT      uiVar;
} VAR_LINK;

typedef struct _DATABASE
{
   char *      szPath;
   USHORT      uiDriver;
   struct _DATABASE * pNext;
} DATABASE;

typedef struct
{
   char *      BagName;
   char *      szFullName;
   USHORT      uiAreas;
   BOOL        bCompound;
} INDEXSTRU, *PINDEXSTRU;

typedef struct
{
   ULONG       ulAreaID;               /* Global area number (virtual) */
   USHORT      uiDriver;
   USHORT      uiAreas;                /* Number of referens? */
   BOOL        bShared;
   BOOL        bLocked;
   BYTE *      szTable;
   LETO_LIST   LocksList;              /* List of records locked */
   LETO_LIST   IndexList;              /* Index List */
   USHORT      uiIndexCount;
   ULONG       ulFlags;                /* Lock flags */
   long        lWriteDate;             /* Last write date and time */
   long        lWriteTime;
} TABLESTRU, *PTABLESTRU;

typedef struct _LETOTAG
{
   PINDEXSTRU  pIStru;
   char        szTagName[12];
   PHB_ITEM    pTopScope;
   PHB_ITEM    pBottomScope;
   struct _LETOTAG * pNext;
} LETOTAG;

typedef struct
{
   ULONG       ulAreaID;               /* Global area number (virtual) */
   PTABLESTRU  pTStru;
   BOOL        bLocked;
   LETO_LIST   LocksList;              /* List of records locked */
   USHORT      uiSkipBuf;
   LETOTAG *   pTag;
   PHB_ITEM    itmFltExpr;
   char        szAlias[HB_RDD_MAX_ALIAS_LEN + 1];  /* alias (client) */
   BOOL        bNotDetach;             /* Detached */
   BOOL        bUseBuffer;
   long        lReadDate;              /* Last read date and time */
   long        lReadTime;
#ifdef __BM
   BOOL        fFilter;
   void *      pBM;
#endif
} AREASTRU, *PAREASTRU;

typedef struct
{
   int         iUserStru;
   HB_SOCKET   hSocket;
   BYTE *      pBuffer;
   ULONG       ulBufferLen;
   BYTE *      pSendBuffer;
   ULONG       ulSendBufferLen;
   char *      szVersion;
   unsigned int uiMajorVer;
   unsigned int uiMinorVer;
   BYTE *      szAddr;
   BYTE *      szNetname;
   BYTE *      szExename;
   PHB_CODEPAGE cdpage;
   char *      szDateFormat;
   BOOL        bCentury;
   char        szAccess[2];
   char        szDopcode[2];
   double      dLastAct;
   LETO_LIST   AreasList;
   USHORT      uiAreasCount;
   VAR_LINK *  pVarLink;
   USHORT      uiVarsOwnCurr;
   HB_THREAD_HANDLE hThread;
   HB_THREAD_ID hThreadID;
   /* variables for the current command */
   ULONG       ulDataLen;                 /* Lenght of buffer */
   BYTE *      pBufRead;                  /* buffer (command) */
   ULONG       ulCurAreaID;               /* Global area number (virtual) */
   PAREASTRU   pCurAStru;
   BOOL        bAnswerSent;
   BOOL        bHrbError;
   BYTE     *  pBufCrypt;
   ULONG       ulBufCryptLen;
   BOOL        bBufKeyNo;
   BOOL        bBufKeyCount;
} USERSTRU, *PUSERSTRU;

typedef struct
{
   AREAP       pArea;
   ULONG       ulRecNo;
   BOOL        bAppend;
   USHORT      uiFlag;
   USHORT      uiItems;
   USHORT *    puiIndex;
   PHB_ITEM *  pItems;
} TRANSACTSTRU;

typedef struct
{
   ULONG *     pulAreaID;
   ULONG       ulNextID;
   int         iCurIndex;
   int         iAllocIndex;
} AVAILAREAID;


// extern BOOL leto_ReadMemArea( char * szBuffer, int iAddr, int iLength );
// extern BOOL leto_WriteMemArea( const char * szBuffer, int iAddr, int iLength );
// extern BOOL leto_ThreadMutexInit( LETO_MUTEX * pMutex );
// extern void leto_ThreadMutexDestroy( LETO_MUTEX * pMutex );
// extern BOOL leto_ThreadMutexLock( LETO_MUTEX * pMutex );
// extern BOOL leto_ThreadMutexUnlock( LETO_MUTEX * pMutex );
// extern BOOL leto_ThreadCondInit( LETO_COND * phEvent  );
// extern void leto_ThreadCondDestroy( LETO_COND * phEvent );
// extern int leto_ThreadCondWait( LETO_COND * phEvent, int iMilliseconds );
// extern BOOL leto_ThreadCondUnlock( LETO_COND * pCond );
