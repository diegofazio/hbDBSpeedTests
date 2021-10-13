/* $Id$ */
/*
 * This sample demonstrates how to use functions Leto_dbTrans(), Leto_dbCopy(),
 * Leto_dbApp() instead of standard __dbTrans(), __dbCopy(), __dbApp()
 */

#include "dbinfo.ch"
#include "std.ch"

REQUEST HB_GT_WIN
REQUEST HB_GT_WIN_DEFAULT 

Field Code, Name

Function main
Local cPath := '//127.0.0.1:2812/'

REQUEST LETO
RDDSETDEFAULT( "LETO" )

cls

dbCreate(cPath + 'test', {{'Code','N',2,0},{'Name','C',20,0}},, .T.)
dbAppend()
Field->Code := 1
Field->Name := "First"
dbAppend()
Field->Code := 2
Field->Name := "Second"
dbCommit()

leto_dbCopy( (cPath+"test2.dbf"), { }, 'Code==2',,,, .F.,,, )
dbUseArea(.t.,, cPath + 'test2')
? Reccount(), Field->Code, Field->Name

return nil

#pragma BEGINDUMP

#include "hbapi.h"
#include "hbapierr.h"
#include "hbapirdd.h"
#include "funcleto.h"

#if !defined (SUCCESS)
#define SUCCESS            0
#define FAILURE            1
#endif

/* Leto_dbTrans( nDstArea, aFieldsStru, cFor, cWhile, nNext, nRecord, lRest ) */
HB_FUNC( LETO_DBTRANS )
{
#ifdef __XHARBOUR__
   HB_THREAD_STUB
#endif
   if( HB_ISNUM( 1 ) )
   {
      USHORT uiSrcArea, uiDstArea;
      AREAP pSrcArea, pDstArea;

      uiSrcArea = hb_rddGetCurrentWorkAreaNumber();
      pSrcArea = ( AREAP ) hb_rddGetCurrentWorkAreaPointer();
      uiDstArea = hb_parni( 1 );
      hb_rddSelectWorkAreaNumber( uiDstArea );
      pDstArea = ( AREAP ) hb_rddGetCurrentWorkAreaPointer();

      if( pSrcArea && pDstArea )
      {
         DBTRANSINFO dbTransInfo;
         PHB_ITEM pFields = hb_param( 2, HB_IT_ARRAY );
         ERRCODE errCode;

         memset( &dbTransInfo, 0, sizeof( DBTRANSINFO ) );
         errCode = hb_dbTransStruct( pSrcArea, pDstArea, &dbTransInfo,
                                     NULL, pFields );
         if( errCode == SUCCESS )
         {
            hb_rddSelectWorkAreaNumber( dbTransInfo.lpaSource->uiArea );

            dbTransInfo.dbsci.itmCobFor   = NULL;
            dbTransInfo.dbsci.lpstrFor    = hb_param( 3, HB_IT_STRING );
            dbTransInfo.dbsci.itmCobWhile = NULL;
            dbTransInfo.dbsci.lpstrWhile  = hb_param( 4, HB_IT_STRING );
            dbTransInfo.dbsci.lNext       = hb_param( 5, HB_IT_NUMERIC );
            dbTransInfo.dbsci.itmRecID    = HB_ISNIL( 6 ) ? NULL : hb_param( 6, HB_IT_ANY );
            dbTransInfo.dbsci.fRest       = hb_param( 7, HB_IT_LOGICAL );

            dbTransInfo.dbsci.fIgnoreFilter     = TRUE;
            dbTransInfo.dbsci.fIncludeDeleted   = TRUE;
            dbTransInfo.dbsci.fLast             = FALSE;
            dbTransInfo.dbsci.fIgnoreDuplicates = FALSE;
            dbTransInfo.dbsci.fBackward         = FALSE;

            errCode = SELF_TRANS( dbTransInfo.lpaSource, &dbTransInfo );
         }

         if( dbTransInfo.lpTransItems )
            hb_xfree( dbTransInfo.lpTransItems );

         hb_retl( errCode == SUCCESS );
      }
      else
         hb_errRT_DBCMD( EG_NOTABLE, EDBCMD_NOTABLE, NULL, "__DBTRANS" );

      hb_rddSelectWorkAreaNumber( uiSrcArea );
   }
   else
      hb_errRT_DBCMD( EG_ARG, EDBCMD_USE_BADPARAMETER, NULL, "__DBTRANS" );
}

HB_FUNC( LETO_DBAPP )
{
#ifdef __XHARBOUR__
   HB_THREAD_STUB
#endif
   AREAP pArea = ( AREAP ) hb_rddGetCurrentWorkAreaPointer();
   if( pArea )
      hb_retl( SUCCESS == hb_rddTransRecords( pArea,
               hb_parc( 1 ),                     /* file name */
               hb_parc( 8 ),                     /* RDD */
               hb_parnl( 9 ),                    /* connection */
               hb_param( 2, HB_IT_ARRAY ),       /* Fields */
               FALSE,                            /* Export? */
               NULL,                             /* cobFor */
               hb_param( 3, HB_IT_STRING ),      /* lpStrFor */
               NULL,                             /* cobWhile */
               hb_param( 4, HB_IT_STRING ),      /* lpStrWhile */
               hb_param( 5, HB_IT_NUMERIC ),     /* Next */
               HB_ISNIL( 6 ) ? NULL : hb_param( 6, HB_IT_ANY ),   /* RecID */
               hb_param( 7, HB_IT_LOGICAL ),     /* Rest */
               hb_parc( 10 ),                    /* Codepage */
               hb_param( 11, HB_IT_ANY ) ) );    /* Delimiter */
   else
      hb_errRT_DBCMD( EG_NOTABLE, EDBCMD_NOTABLE, NULL, "APPEND FROM" );
}

HB_FUNC( LETO_DBCOPY )
{
#ifdef __XHARBOUR__
   HB_THREAD_STUB
#endif
   AREAP pArea = ( AREAP ) hb_rddGetCurrentWorkAreaPointer();
   if( pArea )
      hb_retl( SUCCESS == hb_rddTransRecords( pArea,
               hb_parc( 1 ),                     /* file name */
               hb_parc( 8 ),                     /* RDD */
               hb_parnl( 9 ),                    /* connection */
               hb_param( 2, HB_IT_ARRAY ),       /* Fields */
               TRUE,                             /* Export? */
               NULL,                             /* cobFor */
               hb_param( 3, HB_IT_STRING ),      /* lpStrFor */
               NULL,                             /* cobWhile */
               hb_param( 4, HB_IT_STRING ),      /* lpStrWhile */
               hb_param( 5, HB_IT_NUMERIC ),     /* Next */
               HB_ISNIL( 6 ) ? NULL : hb_param( 6, HB_IT_ANY ),   /* RecID */
               hb_param( 7, HB_IT_LOGICAL ),     /* Rest */
               hb_parc( 10 ),                    /* Codepage */
               hb_param( 11, HB_IT_ANY ) ) );    /* Delimiter */
   else
      hb_errRT_DBCMD( EG_NOTABLE, EDBCMD_NOTABLE, NULL, "COPY TO" );
}

#pragma ENDDUMP
