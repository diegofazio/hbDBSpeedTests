/*  $Id$  */

/*
 * Harbour Project source code:
 * Leto db server core function, Harbour mt model
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
#include "hbthread.ch"

#if defined(HB_OS_UNIX)
  #include <unistd.h>
  #include <sys/time.h>
  #include <sys/timeb.h>
#endif

#ifndef HB_OS_PATH_DELIM_CHR
   #define HB_OS_PATH_DELIM_CHR OS_PATH_DELIMITER
#endif

#if !defined (SUCCESS)
#define SUCCESS            0
#define FAILURE            1
#endif

#define MAX_RECV_BLOCK     10000000

const char * szRelease = "Leto DB Server v.";

static int iServerPort;
static int iTimeOut = -1;
static HB_SOCKET hSocketMain = HB_NO_SOCKET;          // Initial server socket
int leto_GlobalExit = 0;

//static HB_CRITICAL_NEW( mutex_Ustru );
extern PUSERSTRU s_users;
extern USHORT uiUsersMax;
extern USHORT uiUsersCurr;

extern ULONG ulBytesSent;
extern ULONG ulOperations;
extern ULONG ulBytesRead;

extern PTABLESTRU s_tables;

extern USHORT uiTablesAlloc;

extern double dMaxWait;
extern double dMaxDayWait;
extern long   ulToday;
extern double dSumWait[];
extern unsigned int uiSumWait[];

extern BOOL   bPass4L;
extern BOOL   bPass4M;
extern BOOL   bPass4D;
extern BOOL   bCryptTraf;

extern int iDebugMode;
extern BOOL bLockConnect;

extern ULONG leto_MilliSec( void );
extern long leto_Date( void );
extern void leto_CloseTable( USHORT nTableStru );
extern int leto_InitUS( HB_SOCKET hSocket );
extern void leto_CloseUS( PUSERSTRU pUStru );
extern PAREASTRU leto_FindArea( PUSERSTRU pUStru, ULONG ulAreaID );
extern BOOL ParseCommand( PUSERSTRU pUStru );
extern PUSERSTRU leto_FindUserStru( HB_THREAD_ID hThreadID );
extern void leto_wUsLog( PUSERSTRU pUStru, const char* s, int n );
extern void leto_CloseAllSocket( void );
extern void leto_initSet( void );


void leto_errInternal( ULONG ulIntCode, const char * szText, const char * szPar1, const char * szPar2 )
{
   FILE * hLog;
   PUSERSTRU pUStru;
   PAREASTRU pAStru = NULL;
   PTABLESTRU pTStru;
   USHORT ui;
   int i;

/*debug*/ leto_writelog( NULL, 0, "leto_errInternal!!!!!!!!!!!!!!!!!!\r\n" );

   pUStru = leto_FindUserStru( HB_THREAD_SELF() );

   hLog = hb_fopen( "letodb_crash.log", "a+" );

   if( hLog )
   {
      char szTime[ 9 ];
      int iYear, iMonth, iDay;

      hb_dateToday( &iYear, &iMonth, &iDay );
      hb_dateTimeStr( szTime );

      fprintf( hLog, HB_I_("Breakdown at: %04d.%02d.%02d %s\n"), iYear, iMonth, iDay, szTime );
      fprintf( hLog, "Unrecoverable error %lu: ", ulIntCode );
      if( szText )
         fprintf( hLog, "%s %s %s\n", szText, szPar1, szPar2 );
      fprintf( hLog, "------------------------------------------------------------------------\n");
      if( pUStru )
      {
         fprintf( hLog, "User: %s %s %s\n", pUStru->szAddr, pUStru->szNetname, pUStru->szExename );
         if( pUStru->pBufRead )
            fprintf( hLog, "Command: %s\n", pUStru->pBufRead );
         if( pUStru->pCurAStru && pUStru->pCurAStru->pTStru )
            fprintf( hLog, "Table: %s\n", pUStru->pCurAStru->pTStru->szTable );
      }

      fclose( hLog );
   }

   pTStru = s_tables;
   ui = 0;
   i = -1;
   if( pTStru )
   {
      while( ui < uiTablesAlloc )
      {
         if( !pAStru || pAStru->pTStru != pTStru )
         {
            if( pTStru->szTable )
               leto_CloseTable( ui );
         }
         else if( pAStru )
            i = (int) ui;
         ui ++;
         pTStru ++;
      }
      if( i >= 0 )
      {
         if( pAStru->pTStru->szTable )
            leto_CloseTable( (USHORT)i );
      }
      hb_xfree( s_tables );
      s_tables = NULL;
   }

}

static void leto_timewait( double dLastAct )
{
   static int iCurIndex = 0;
   static long ulCurSec = 0;
   long ulDay = leto_Date();

   if( ulDay != ulToday )
   {
      ulToday = ulDay;
      dMaxDayWait = 0;
   }

   if( dLastAct )
   {
      double dCurrSec = hb_dateSeconds(), dSec;
      long ulSec = ( (long)dCurrSec ) / 10;

      if( ( dSec = ( dCurrSec - dLastAct ) ) > dMaxWait )
         dMaxWait = dSec;

      if( dSec > dMaxDayWait )
         dMaxDayWait = dSec;

      if( ulSec != ulCurSec )
      {
         ulCurSec = ulSec;
         if( ++iCurIndex >= 6 )
            iCurIndex = 0;
         dSumWait[iCurIndex]  = 0;
         uiSumWait[iCurIndex] = 0;
      }
      dSumWait[iCurIndex] += dSec;
      uiSumWait[iCurIndex] ++;
   }
}

static ULONG leto_SockSend( HB_SOCKET hSocket, char * pBuf, ULONG ulLen )
{
   ULONG ulSent = 0;
   long lTmp;

   while( ulSent < ulLen )
   {
      if( ( lTmp = hb_socketSend( hSocket, pBuf + ulSent, ulLen - ulSent, 0, -1 ) ) > 0 )
         ulSent += lTmp;
      else if( hb_socketGetError() != HB_SOCKET_ERR_TIMEOUT || hb_vmRequestQuery() != 0 )
         break;
   }
   return ulSent;
}

static ULONG leto_SockRecv( HB_SOCKET hSocket, char * pBuf, ULONG ulLen )
{
   ULONG ulRead = 0;
   long lTmp;

   while( ulRead < ulLen )
   {
//      if( ( lTmp = hb_socketRecv( hSocket, pBuf + ulRead, ulLen - ulRead, 0, 3000 ) ) <= 0 )
      if( ( lTmp = hb_socketRecv( hSocket, pBuf + ulRead, ulLen - ulRead, 0, iTimeOut ) ) <= 0 )
      {
//         if( hb_socketGetError() != HB_SOCKET_ERR_TIMEOUT )
            break;
      }
      else
         ulRead += lTmp;
   }

   return ulRead;
}

void leto_SendAnswer( PUSERSTRU pUStru, const char* szData, ULONG ulLen )
{
   HB_SOCKET hSocket = pUStru->hSocket;
   ULONG ulSendLen = LETO_MSGSIZE_LEN + ulLen;
   char * ptr;

   if( iDebugMode >= 15 )
   {
      ULONG DEBUGulLen = ( iDebugMode >= 20 && ulLen ) ? ulLen : strlen( szData );
      if( DEBUGulLen > ulLen )
         DEBUGulLen = ulLen;
      leto_wUsLog( pUStru, szData, DEBUGulLen );
   }

   if( pUStru->ulSendBufferLen < ulSendLen + 3 )
   {
      pUStru->ulSendBufferLen = ulSendLen + 3;
      pUStru->pSendBuffer     = (BYTE*) hb_xrealloc( pUStru->pSendBuffer, pUStru->ulSendBufferLen );
   }

   ptr = (char*) pUStru->pSendBuffer;
   memcpy( ptr + LETO_MSGSIZE_LEN, szData, ulLen );

   if( (((int)*szData) & 0xFF) >= 10 && *(szData+ulLen-1) != '\n' )
   {
      *(ptr+ulSendLen)   = '\r';
      *(ptr+ulSendLen+1) = '\n';
      *(ptr+ulSendLen+2) = '\0';
      ulSendLen += 2;
   }

   HB_PUT_LE_UINT32( ptr, ulSendLen - LETO_MSGSIZE_LEN );
   leto_SockSend( hSocket, ptr, ulSendLen );

   ulBytesSent += ulSendLen;
}

HB_FUNC( LETO_SENDMESSAGE )
{
   char szBuffer[32];
   long int ulSendLen;
   BOOL bRetVal = FALSE;

   void *pSockAddr;
   unsigned int uiLen;
   HB_SOCKET hSocket;

   int iPort = hb_parni( 1 );
   const char *szMessage = hb_parc( 2 );
   const char *szAddress = (HB_ISNIL(3))? (char*)NULL : (char*)hb_parc(3);

   hb_socketInit();

   if( ( hSocket = hb_socketOpen( HB_SOCKET_AF_INET, HB_SOCKET_PT_STREAM, 0 ) ) != HB_NO_SOCKET )
   {
      hb_socketInetAddr( &pSockAddr, &uiLen, (szAddress)? szAddress : "127.0.0.1", iPort );
      if( hb_socketConnect( hSocket, pSockAddr, uiLen, -1 ) == 0 )
      {
         // skip server answer
         char *pBuffer;
         ULONG ulRecvLen = leto_SockRecv( hSocket, szBuffer, LETO_MSGSIZE_LEN );
         if( ulRecvLen == LETO_MSGSIZE_LEN )
         {
            ulRecvLen = HB_GET_LE_UINT32( szBuffer );
            pBuffer = hb_xalloc( ulRecvLen + 1 );
            if( leto_SockRecv( hSocket, pBuffer, ulRecvLen ) == ulRecvLen )
            {
               strcpy( szBuffer+LETO_MSGSIZE_LEN, szMessage );
               strcpy( szBuffer+LETO_MSGSIZE_LEN+strlen(szMessage), ";\r\n" );
               ulSendLen = strlen( szBuffer+LETO_MSGSIZE_LEN );
               HB_PUT_LE_UINT32( szBuffer, ulSendLen );
               leto_SockSend( hSocket, szBuffer, ulSendLen+LETO_MSGSIZE_LEN );
               bRetVal = TRUE;
            }
            hb_xfree( pBuffer );
         }
      }
      hb_socketClose( hSocket );
      hb_xfree( pSockAddr );
   }
   hb_retl( bRetVal );
}

static HB_THREAD_STARTFUNC ( thread2 )
{
   int iUserStru = (int) ( (HB_SIZE)Cargo & 0xFFFF );
   PUSERSTRU pUStru;
   static double dSec4GC = 0;
   double dSec;
   HB_SOCKET  hSocket;
   ULONG ulLen, ulRecvLen;
   USHORT uiLenLen;

   hb_vmThreadInit( NULL );
   leto_initSet();

   pUStru   = s_users + iUserStru;
   hSocket  = pUStru->hSocket;

   while( TRUE )
   {
      BYTE * ptr;

      pUStru = s_users + iUserStru;

      ulRecvLen = leto_SockRecv( hSocket, (char*) pUStru->pBuffer, LETO_MSGSIZE_LEN );
      if( ulRecvLen == 0 )   // connect closed
         break;
      if( ulRecvLen != LETO_MSGSIZE_LEN )
      {
         leto_writelog( NULL,0,"ERROR! thread2() leto_SockRecv LETO_MSGSIZE_LEN\r\n" );
         break;
      }

      ulRecvLen = HB_GET_LE_UINT32( pUStru->pBuffer );
      if( ulRecvLen == 0 )
      {
         leto_writelog( NULL,0,"ERROR! thread2() ulRecvLen==0\r\n" );
         break;
      }
      if( ulRecvLen > MAX_RECV_BLOCK )
      {
         leto_writelog( NULL,0,"ERROR! thread2() too big packet\r\n" );
         break;
      }
      if( pUStru->ulBufferLen <= ulRecvLen )
      {
         pUStru->ulBufferLen = ulRecvLen + 1;
         pUStru->pBuffer     = hb_xrealloc( pUStru->pBuffer, pUStru->ulBufferLen );
      }

      if( leto_SockRecv( hSocket, (char*) pUStru->pBuffer, ulRecvLen ) != ulRecvLen )
      {
         leto_writelog( NULL,0,"ERROR! thread2() leto_SockRecv!=ulRecvLen\r\n" );
         break;
      }
      ptr = pUStru->pBuffer;
      ulBytesRead += ulRecvLen;
      ulLen = ulRecvLen;

      if( ( uiLenLen = ( ((int)*ptr) & 0xFF ) ) < 10 )
      {
         if( ((ULONG)uiLenLen) < ulRecvLen )
         {
            ulLen = leto_b2n( (char*)ptr+1, uiLenLen );
            if( ulLen > 0 && ulLen+uiLenLen+1 <= ulRecvLen )
            {
               ptr += 1 + uiLenLen;
               ulLen = ulRecvLen - 1 - uiLenLen;
            }
            else
               ulLen = 0;
         }
         else
            ulLen = 0;
      }
      else if( *(ptr+ulLen-1) == '\n' && *(ptr+ulLen-2) == '\r' )
         ulLen -= 2;
      else
         ulLen = 0;

      if( ulLen == 0 )
      {
         leto_SendAnswer( pUStru, "-001", 4 );
         leto_writelog( NULL,0,"!ERROR! error command format:\r\n" );  //debug
         leto_writelog( NULL, ulRecvLen, (char*)pUStru->pBuffer );
         continue;
      }
      *(ptr+ulLen) = '\0';

//      hb_threadEnterCriticalSection( &mutex_Ustru );
      ulOperations ++;
      pUStru->pBufRead    = ptr;
      pUStru->ulDataLen   = ulLen;
      pUStru->dLastAct    = hb_dateSeconds();
      pUStru->ulCurAreaID = 0;
      pUStru->pCurAStru   = NULL;
//      hb_threadLeaveCriticalSection( &mutex_Ustru );

      if( !ParseCommand( pUStru ) )
      {
         leto_writelog( NULL,0,"ERROR! ParseCommand()\r\n" );
         leto_writelog( NULL, pUStru->ulDataLen, (char*) pUStru->pBufRead );
         leto_SendAnswer( pUStru, "-001", 4 );
      }

      pUStru->pBufRead    = NULL;
      pUStru->ulDataLen   = 0;
      pUStru->ulCurAreaID = 0;
      pUStru->pCurAStru   = NULL;

      leto_timewait( pUStru->dLastAct );
      if( ( dSec = hb_dateSeconds() ) - dSec4GC > 300 )
      {
         dSec4GC = dSec;
         hb_gcCollectAll( FALSE );
      }
   }

   if( hSocket != HB_NO_SOCKET )
      hb_socketClose( hSocket );

   leto_CloseUS( pUStru );

   hb_vmThreadQuit();
   HB_THREAD_END
}

HB_FUNC( LETO_SERVER )
{
   char szBuffer[32], *szAddress;
   long int lTemp;

   HB_SOCKET incoming;
   void *pSockAddr;
   unsigned int uiLen;
   double dStartSec;

   iServerPort = hb_parni(1);
   iTimeOut = hb_parni(2);
   szAddress = (HB_ISNIL(3))? (char*)NULL : (char*)hb_parc(3);

   hb_socketInit();
   if( ( hSocketMain = hb_socketOpen( HB_SOCKET_AF_INET, HB_SOCKET_PT_STREAM, 0 ) ) != HB_NO_SOCKET )
   {
      hb_socketSetKeepAlive( hSocketMain, HB_TRUE );
      hb_socketSetNoDelay( hSocketMain, HB_TRUE );
//       hb_socketSetSndBufSize( hSocketMain, int iSize );

      hb_socketInetAddr( &pSockAddr, &uiLen, szAddress, iServerPort );
      if( hb_socketBind( hSocketMain, pSockAddr, uiLen ) != 0 || hb_socketListen( hSocketMain, 10 ) != 0 )
      {
         hb_socketClose( hSocketMain );
         hSocketMain = HB_NO_SOCKET;
      }
      hb_xfree( pSockAddr );
   }
   if( hSocketMain == HB_NO_SOCKET )
   {
      hb_retl( FALSE );
      return;
   }

   //  main thread

   // Unlock HVM stack
   hb_vmUnlock();

   while( TRUE )
   {
      incoming = hb_socketAccept( hSocketMain, &pSockAddr, &uiLen, 3000 );

      ulOperations ++;
      if( incoming != HB_NO_SOCKET && bLockConnect )
      {
         hb_socketClose( incoming );
      }
      else if( incoming != HB_NO_SOCKET )
      {
         int iUserStru;
         PUSERSTRU pUStru;

         hb_socketSetKeepAlive( incoming, HB_TRUE );
         hb_socketSetNoDelay( incoming, HB_TRUE );

         iUserStru = leto_InitUS( incoming );
         pUStru = s_users + iUserStru;
         pUStru->szAddr = (BYTE*) hb_socketAddrGetName( pSockAddr, uiLen );
         hb_xfree( pSockAddr );

         if( iDebugMode > 0 )
         {
            char ddd[400];
            hb_snprintf( ddd, 400, "DEBUG! new connect %s (%d : %d : %d)\r\n", pUStru->szAddr, iUserStru, uiUsersCurr, uiUsersMax );
            leto_wUsLog(NULL,ddd, 0);
         }

         pUStru->hThread = hb_threadCreate( &pUStru->hThreadID, thread2, (void*) ((HB_SIZE)iUserStru) );
         if( !pUStru->hThread )
         {
            leto_writelog( NULL,0,"!ERROR! thread create error...\r\n" );
            leto_SendAnswer( pUStru, "-ERR:MAX_THREADS", 16 );
            leto_CloseUS( pUStru );
            hb_socketClose( incoming );
         }
         else
         {
            hb_threadDetach( pUStru->hThread );

            leto_timewait( 0 );

            hb_snprintf( szBuffer, 32, "%s%s", szRelease, HB_LETO_VERSION_STRING );
            lTemp = strlen( szBuffer );
            szBuffer[lTemp++] = ';';
            szBuffer[lTemp++] = (bCryptTraf)? 'Y':'N';
            if( bPass4L || bPass4M || bPass4D )
            {
               hb_snprintf( szBuffer+lTemp, 32-lTemp, "%d;", (int) (ulOperations%100) );
               pUStru->szDopcode[0] = *( szBuffer+lTemp );
               pUStru->szDopcode[1] = *( szBuffer+lTemp+1 );
            }
            else
            {
               szBuffer[lTemp++] = ';';
               szBuffer[lTemp] = '\0';
            }

            leto_SendAnswer( pUStru, szBuffer, strlen(szBuffer) );
         }
      }
      else if( hb_socketGetError() != HB_SOCKET_ERR_TIMEOUT )
      {
         // error!!!
         break;
      }

      if( leto_GlobalExit )
         break;

   }

   if( hSocketMain != HB_NO_SOCKET )
      hb_socketClose( hSocketMain );
   hSocketMain = HB_NO_SOCKET;

   // if( !s_AvailIDS.pulAreaID )
   //    hb_xfree( s_AvailIDS.pulAreaID );

   // lock HVM stack before executing any PRG code
   hb_vmLock();

   // wait other threads
   leto_CloseAllSocket();
   dStartSec = hb_dateSeconds();
   while( hb_dateSeconds() - dStartSec < 30 )
   {
      leto_CloseAllSocket();
      if( ! uiUsersCurr )
         break;
#if defined( HB_OS_WIN_32 ) || defined( HB_OS_WIN )
      Sleep(1);
#else
      sleep(1);
#endif
   }

   hb_retl( TRUE );
}
