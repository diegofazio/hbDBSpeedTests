REQUEST DBFCDX
REQUEST HB_MEMIO

FUNCTION Main()

   LOCAL cPathData := 'c:\bases\dbf\'
   LOCAL dDesde := 0d20040101
   LOCAL dHasta := 0d20041231
   LOCAL aRecordSet := {}
   LOCAL aRecord 
   LOCAL a

   ? "Connecting DBF..."
   a =  hb_MilliSeconds()

   dbSelectArea( 1 )
   dbUseArea( .F., "DBFCDX", cPathData + 'db.dbf',, .T. )

   IF NetErr()
      ?? "Error open DBF"
   ENDIF

   ?? 'OK ', hb_MilliSeconds() - a, 'ms'
   ? "Getting data..."
   a =  hb_MilliSeconds()

   INDEX ON KAR_FECHA TO MEM:kar

   SET SCOPETOP TO dDesde
   SET SCOPEBOTTOM TO dHasta
   dbGoTop()

   DO WHILE !Eof()
   
      aRecord       := Array( 13 )
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
