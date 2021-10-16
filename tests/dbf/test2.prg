REQUEST DBFCDX
REQUEST HB_MEMIO

FUNCTION Main()

   LOCAL cPathData := 'c:\bases\dbf\'
   LOCAL nCount := 0
   LOCAL dDesde := 0d20040101
   LOCAL dHasta := 0d20041231
   LOCAL hRes := { => }
   LOCAL recordset := {}
   LOCAL a

   ? "Connecting DBF..."
   a =  hb_MilliSeconds()

   dbSelectArea( 1 )
   dbUseArea( .F., "DBFCDX", cPathData + 'db.dbf',, .T. )

   if neterr()
      ?? "Error open DBF"
   endif

   ?? 'OK ', hb_MilliSeconds() - a, 'ms' 
   ? "Getting data..."
   a =  hb_MilliSeconds()

   INDEX ON KAR_FECHA TO MEM:kar

   SET SCOPETOP TO dDesde
   SET SCOPEBOTTOM TO dHasta
   dbGoTop()

   DO WHILE !Eof()

      hRes := { => }
      hRes[ 'ID' ] := RecNo()
      hRes[ 'KAR_RUBRO' ] := KAR_RUBRO
      hRes[ 'KAR_FECHA' ] := KAR_FECHA
      hRes[ 'KAR_CLIE'  ] := KAR_CLIE 
      hRes[ 'KAR_TIPO'  ] := KAR_TIPO 
      hRes[ 'KAR_NUMERO'] := KAR_NUMERO
      hRes[ 'KAR_DEPO'  ] := KAR_DEPO 
      hRes[ 'KAR_CANT'  ] := KAR_CANT 
      hRes[ 'KAR_PRECIO'] := KAR_PRECIO
      hRes[ 'KAR_DESCTO'] := KAR_DESCTO
      hRes[ 'KAR_VENDED'] := KAR_VENDED
      hRes[ 'KAR_PIEZAS'] := KAR_PIEZAS
      hRes[ 'KAR_ENTSAL'] := KAR_ENTSAL
      hRes[ 'KAR_ARTIC' ] := KAR_ARTIC
      AAdd( recordset, hRes )
      dbSkip()

   ENDDO
   ?? 'OK'
   ? "Total time:", hb_MilliSeconds() - a, "ms"
   ? "Result: ", len(recordset)

RETURN NIL
