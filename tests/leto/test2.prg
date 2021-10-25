REQUEST HB_MEMIO
FUNCTION Main()

   LOCAL cIp := "127.0.0.1"
   LOCAL cPort := "2812"
   LOCAL cPathData := '//' + cIp + ":" + cPort + "/"
   LOCAL dDesde := 0d20040101
   LOCAL dHasta := 0d20041231
   LOCAL aRecordSet := {}
   LOCAL aRecord := Array( 13 )
   LOCAL a

   ? "Connecting DBF VIA LETO..."
   a =  hb_MilliSeconds()

   IF ( leto_Connect( "//" + cIp + ":" + cPort + "/" ) ) == -1
      ?? "Server not found"
      QUIT
   ENDIF

   dbSelectArea( 1 )
   dbUseArea( .F., "LETO", cPathData + 'dbf/db.dbf',, .T. )
   ?? 'OK ', hb_MilliSeconds() - a, 'ms'
   ? "Getting data..."
   a =  hb_MilliSeconds()

   INDEX ON KAR_FECHA TO MEM:kar

   SET SCOPETOP TO dDesde
   SET SCOPEBOTTOM TO dHasta
   dbGoTop()

   DO WHILE !Eof()

      aRecord[ 1 ]  := KAR_RUBRO
      aRecord[ 2 ]  := KAR_FECHA
      aRecord[ 3 ]  := KAR_CLIE
      aRecord[ 4 ]  := KAR_TIPO
      aRecord[ 5 ]  := KAR_NUMERO
      aRecord[ 6 ]  := KAR_DEPO
      aRecord[ 7 ]  := KAR_CANT
      aRecord[ 8 ]  := KAR_PRECIO
      aRecord[ 9 ]  := KAR_DESCTO
      aRecord[ 10 ] := KAR_VENDED
      aRecord[ 11 ] := KAR_PIEZAS
      aRecord[ 12 ] := KAR_ENTSAL
      aRecord[ 13 ] := KAR_ARTIC
      AAdd( aRecordSet, aRecord )
      dbSkip()

   ENDDO

   ?? 'OK'
   ? "Total time:", hb_MilliSeconds() - a, "ms"
   ? "Result: ", Len( aRecordSet )

RETURN NIL
