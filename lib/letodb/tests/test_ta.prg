/* $Id$ */
/*
 * This sample demonstrates how to use transactions with Leto db server
 * Just change the cPath value to that one you need.
 */

Function Main( cPath )
Field NORD, DORD, NPROD, SUMMA

   REQUEST LETO
   RDDSETDEFAULT( "LETO" )

   IF Empty( cPath )
      cPath := "//127.0.0.1:2812/temp/"
   ELSE
      cPath := "//" + cPath + Iif( Right(cPath,1) $ "/\", "", "/" )
   ENDIF
   ? "Start"
   dbCreate( cPath+"nakl1", { {"NORD","N",10,0},{"DORD","D",8,0},{"SUMMA","N",12,2},{"NORM","M",10,0} } )
   dbCreate( cPath+"nakl2", { {"NORD","N",10,0},{"DORD","D",8,0},{"NPROD","N",3,0},{"SUMMA","N",12,2},{"NORM","M",10,0} } )
   ? "Files has been created"

   use ( cPath+"nakl1" ) New
   index on Dtos(DORD)+Str(NORD,10) TAG DATA
   use ( cPath+"nakl2" ) New
   index on Dtos(DORD)+Str(NORD,10)+Str(NPROD,3) TAG DATA
   ? "Files has been opened and indexed"

   AddNakl( 1, Date(), { 1400.5, 28632.28, 800.51 } )
   AddNakl( 2, Date(), { 58003, 930.5 } )
   ? "Records has been added"

   select NAKL2
   if dbSeek( Dtos(Date())+Str(1,10)+Str(2,3) )
      if ChangeNakl( 35688.24 )
         ? "Records has been changed"
      else
         ? "Failure - Rollback"
      endif
   endif

   ? "End"
   ?

Return Nil

Function AddNakl( n_ord, d_ord, aSumm )
Local i, sumAll := 0

   leto_BeginTransaction()

   select NAKL2
   for i := 1 to Len( aSumm )
      append blank
      replace NORD with n_ord, DORD with d_ord, NPROD with i, SUMMA with aSumm[i]
      sumAll += aSumm[i]
   next

   select NAKL1
   append blank
   replace NORD with n_ord, DORD with d_ord, SUMMA with sumAll

   leto_CommitTransaction()

Return .T.

Function ChangeNakl( nSummNew )
Local nDelta

   leto_BeginTransaction()

   select NAKL1
   if !dbSeek( Dtos(NAKL2->DORD)+STR(NAKL2->NORD) ) .or. !DoRlock( 3 )
      leto_Rollback(.F.)
      Return .F.
   endif

   select NAKL2
   if DoRlock( 3 )
      nDelta := nSummNew - SUMMA
      replace SUMMA with nSummNew
   else
      leto_Rollback()
      Return .F.
   endif

   select NAKL1
   replace SUMMA with SUMMA + nDelta

   leto_CommitTransaction()

Return .T.

Function DoRLock( n )
Local i := 0

   do while i < n
      if Rlock()
         exit
      endif
      Inkey( 0.1 )
      i ++
   enddo

Return ( i < n )
