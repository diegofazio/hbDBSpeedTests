/*  $Id$  */

/*
 * Harbour Project source code:
 * Leto db server (Windows) functions
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


#ifdef __WIN_SERVICE__

#include "hbapi.h"
#include "hbapiitm.h"
#include "hbvm.h"
//#include "hbxvm.h"
//#include "hbset.h"
//#include "hbthread.h"
#include "srvleto.h"

#define _SERVICE_NAME            "LetoDB_Service"
#define _SERVICE_DISPLAY_NAME    "LetoDB Service"


static SERVICE_STATUS_HANDLE     hServiceHandle = 0;
static char                      ServiceEntryFunc[ HB_SYMBOL_NAME_LEN + 1 ];
static TCHAR                     ServiceName[] = _SERVICE_NAME;
static TCHAR                     ServiceDisplayName[] = _SERVICE_DISPLAY_NAME;

extern int leto_GlobalExit;

static ULONG ulSvcError = 0;


void leto_SetServiceStatus( DWORD State )
{
   SERVICE_STATUS SrvStatus;

   SrvStatus.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
   SrvStatus.dwCurrentState            = State;
   SrvStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
   SrvStatus.dwWin32ExitCode           = 0;
   SrvStatus.dwServiceSpecificExitCode = 0;
   SrvStatus.dwCheckPoint              = 0;
   SrvStatus.dwWaitHint                = 0;

   if( hServiceHandle )
      SetServiceStatus( hServiceHandle, &SrvStatus );
}

void WINAPI leto_ServiceControlHandler( DWORD dwCtrlCode )
{
   DWORD State = SERVICE_RUNNING;

   switch( dwCtrlCode )
   {
      case SERVICE_CONTROL_STOP:
         State = SERVICE_STOPPED;
         leto_GlobalExit = 1;
         return;

      case SERVICE_CONTROL_SHUTDOWN:
         State = SERVICE_STOPPED;
         leto_GlobalExit = 1;
         return;
   }

   leto_SetServiceStatus( State );
}

void WINAPI leto_ServiceMainFunction( DWORD dwArgc, LPTSTR * lpszArgv )
{
   HB_SYMBOL_UNUSED( dwArgc );
   HB_SYMBOL_UNUSED( lpszArgv );

   hServiceHandle = RegisterServiceCtrlHandler( ServiceName, ( LPHANDLER_FUNCTION ) leto_ServiceControlHandler );

   if( hServiceHandle != (SERVICE_STATUS_HANDLE) 0 )
   {
      PHB_DYNS pDynSym;

      leto_SetServiceStatus( SERVICE_RUNNING );

      hb_vmThreadInit( NULL );

      pDynSym = hb_dynsymFindName( ServiceEntryFunc );

      if( pDynSym )
      {
          if( hb_vmRequestReenter() )
         {
            hb_vmPushSymbol( hb_dynsymSymbol( pDynSym ) );
            hb_vmPushNil();
            hb_vmProc( 0 );

            hb_vmRequestRestore();
         }
      }

      leto_SetServiceStatus( SERVICE_STOPPED );

      hb_vmThreadQuit();
   }
}

HB_FUNC( LETO_SERVICESTART )
{
   HB_BOOL bRetVal = HB_FALSE;
   SERVICE_TABLE_ENTRY lpServiceTable[ 2 ] = { { ServiceName, ( LPSERVICE_MAIN_FUNCTION ) leto_ServiceMainFunction },
                                               { NULL, NULL } };

   hb_strncpy( ServiceEntryFunc, hb_parcx( 1 ), HB_SIZEOFARRAY( ServiceEntryFunc ) - 1 );

   if( StartServiceCtrlDispatcher( lpServiceTable ) )
      bRetVal = HB_TRUE;
   else
      ulSvcError = ( ULONG ) GetLastError();

   hb_retl( bRetVal );
}

HB_FUNC( LETO_SERVICEINSTALL )
{
   HB_BOOL bRetVal = HB_FALSE;
   TCHAR szPath[ MAX_PATH ];
   SC_HANDLE schSrv;

   if( GetModuleFileName( NULL, szPath, MAX_PATH ) )
   {
      SC_HANDLE schSCM = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

      if( schSCM )
      {
         schSrv = CreateService( schSCM,
                                 ServiceName,
                                 ServiceDisplayName,
                                 SERVICE_ALL_ACCESS,
                                 SERVICE_WIN32_OWN_PROCESS,
                                 SERVICE_AUTO_START,        //SERVICE_DEMAND_START,
                                 SERVICE_ERROR_NORMAL,
                                 szPath,
                                 NULL, NULL, NULL, NULL, NULL );

         if( schSrv )
         {
            CloseServiceHandle( schSrv );
            bRetVal = HB_TRUE;
         }
         else
         {
            ulSvcError = ( ULONG ) GetLastError();
         }

         CloseServiceHandle( schSCM );
      }
      else
         ulSvcError = ( ULONG ) GetLastError();
   }

   hb_retl( bRetVal );
}

HB_FUNC( LETO_SERVICEDELETE )
{
   HB_BOOL bRetVal = HB_FALSE;

   SC_HANDLE schSCM = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

   if( schSCM )
   {
      SC_HANDLE schService;
      SERVICE_STATUS_PROCESS ssp;
      DWORD dwStartTime, dwTimeout, dwWaitTime;
      DWORD dwBytesNeeded;

      // check and stop
      schService = OpenService( schSCM, ServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS );
      if( schService )
      {
         dwStartTime = GetTickCount();
         // Make sure the service is not already stopped.
         if( QueryServiceStatusEx( schService, SC_STATUS_PROCESS_INFO, (LPBYTE) &ssp,
                                       sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded ) )
         {
            // If a stop is pending, wait for it.
            dwTimeout = 30000; // 30-second time-out
            while( ssp.dwCurrentState == SERVICE_STOP_PENDING )
            {
               dwWaitTime = ssp.dwWaitHint / 10;
               if( dwWaitTime < 1000 )
                  dwWaitTime = 1000;
               else if( dwWaitTime > 10000 )
                  dwWaitTime = 10000;

               Sleep( dwWaitTime );

               if( !QueryServiceStatusEx( schService, SC_STATUS_PROCESS_INFO, (LPBYTE) &ssp,
                                             sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded ) )
               {
                  break;
               }

               if( ssp.dwCurrentState == SERVICE_STOPPED )
                  break;

               if( GetTickCount() - dwStartTime > dwTimeout )
                  break;
            }
         }

         // Send a stop code to the service.
         if( ControlService( schService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS) &ssp ) )
         {
            // Wait for the service to stop.
            while( ssp.dwCurrentState != SERVICE_STOPPED )
            {
               dwWaitTime = ssp.dwWaitHint / 10;
               if( dwWaitTime < 1000 )
                  dwWaitTime = 1000;
               else if( dwWaitTime > 10000 )
                  dwWaitTime = 10000;

               Sleep( ssp.dwWaitHint );

               if( !QueryServiceStatusEx( schService, SC_STATUS_PROCESS_INFO, (LPBYTE) &ssp,
                                          sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded ) )
               {
                  break;
               }
               if( ssp.dwCurrentState == SERVICE_STOPPED )
                  break;

               if( GetTickCount() - dwStartTime > dwTimeout )
                  break;
            }
         }

         CloseServiceHandle( schService );
      }

      // delete
      schService = OpenService( schSCM, ServiceName, DELETE );
      if( schService )
      {
         bRetVal = ( HB_BOOL ) DeleteService( schService );
         CloseServiceHandle( schService );
         if( ! bRetVal )
         {
            ulSvcError = ( ULONG ) GetLastError();
         }
      }
      else
         ulSvcError = ( ULONG ) GetLastError();

      CloseServiceHandle( schSCM );
   }

   hb_retl( bRetVal );
}

HB_FUNC( LETOWIN_GETLASTERROR )
{
   hb_retnl( ulSvcError );
}

#endif

/*
extern char carr1[40][40];
extern USHORT uiarr1len;
extern char carr2[100][40];
extern USHORT uiarr2len;

typedef struct
{
   HANDLE hMem;
   LPVOID lpView;
   BOOL bNewArea;
} MEMAREA;

static MEMAREA s_MemArea = { 0,0,0 };


HB_FUNC( LETO_MSG )
{
   LPTSTR lpMsg = HB_TCHAR_CONVTO( hb_parcx( 1 ) );

   MessageBox( GetActiveWindow(), lpMsg, TEXT( "Leto db server" ), MB_OK );

   HB_TCHAR_FREE( lpMsg );
}

HB_FUNC( LETO_CREATEMEMAREA )
{
   char szId[16];

   sprintf( szId, "%s%d", "Leto_DB_", hb_parni(1) );
   s_MemArea.hMem = CreateFileMapping( (HANDLE)0xFFFFFFFF, NULL, PAGE_READWRITE, 0, 1024, (LPCTSTR)szId );

   if( s_MemArea.hMem )
   {
      s_MemArea.bNewArea = !( GetLastError() == ERROR_ALREADY_EXISTS );
      s_MemArea.lpView = MapViewOfFile( s_MemArea.hMem, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0 );
      if( s_MemArea.bNewArea )
         ((char*)s_MemArea.lpView)[0] = 'L';
   }

   hb_retni( (s_MemArea.hMem)?  s_MemArea.bNewArea : -1 );
}

void leto_CloseMemArea( void )
{
   if( s_MemArea.lpView )
      UnmapViewOfFile( s_MemArea.lpView );
   if( s_MemArea.hMem )
      CloseHandle( s_MemArea.hMem );
}

HB_FUNC( LETO_CLOSEMEMAREA )
{
   leto_CloseMemArea();
}

BOOL leto_ReadMemArea( char * szBuffer, int iAddr, int iLength )
{
   if( !s_MemArea.lpView )
      return FALSE;

   memcpy( szBuffer, ((char*)s_MemArea.lpView)+iAddr, iLength );
   szBuffer[iLength] = '\0';
   return TRUE;
}

BOOL leto_WriteMemArea( const char * szBuffer, int iAddr, int iLength )
{
   if( !s_MemArea.lpView )
      return FALSE;

   memcpy( ((char*)s_MemArea.lpView)+iAddr, szBuffer, iLength );
   return TRUE;
}

BOOL leto_ThreadCreate( unsigned int (__stdcall *ThreadFunc)(void*) )
{
   unsigned int uiThreadID;
   if( _beginthreadex( NULL, 0, ThreadFunc, (void *) NULL, 0, &uiThreadID ) )
   {
      return TRUE;
   }
   else
      return FALSE;
}


BOOL leto_ThreadMutexInit( LETO_MUTEX * pMutex )
{
   InitializeCriticalSection( pMutex );
   return TRUE;
}

void leto_ThreadMutexDestroy( LETO_MUTEX * pMutex )
{
   DeleteCriticalSection( pMutex );
}

BOOL leto_ThreadMutexLock( LETO_MUTEX * pMutex )
{
   EnterCriticalSection( pMutex );
   return TRUE;
}

BOOL leto_ThreadMutexUnlock( LETO_MUTEX * pMutex )
{
   LeaveCriticalSection( pMutex );
   return TRUE;
}

BOOL leto_ThreadCondInit( LETO_COND * pCond  )
{
   if( ( pCond->hEvent = CreateEvent( NULL, FALSE, FALSE, NULL ) ) == 0 )
      return FALSE;
   else
      return TRUE;
}

void leto_ThreadCondDestroy( LETO_COND * pCond )
{
   CloseHandle( pCond->hEvent );
}

int leto_ThreadCondWait( LETO_COND * pCond, int iMilliseconds )
{
   DWORD dw;

   InterlockedExchange( &(pCond->ulLocked), 1 );
   dw = WaitForSingleObject( pCond->hEvent,
                 (iMilliseconds<0)? INFINITE : (DWORD)iMilliseconds );
   switch( dw )
   {
      case WAIT_OBJECT_0:
      {
         InterlockedExchange( &(pCond->ulLocked), 0 );
         return 1;
      }
      case WAIT_TIMEOUT:
         return 0;
   }
   return -1;
}

BOOL leto_ThreadCondUnlock( LETO_COND * pCond )
{
   BOOL bRes;

   if( pCond->ulLocked )
   {
      bRes = SetEvent( pCond->hEvent );
      if (!bRes)
         InterlockedExchange( &(pCond->ulLocked), 0 );
      return bRes;
   }
   return FALSE;
}
*/
