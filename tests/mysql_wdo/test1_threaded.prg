#DEFINE CONNECTORS 10   // Total mysql connectors available
#DEFINE QUERYS     10   // Total querys to process concurrently

static nTotTime         // Total process time

FUNCTION Main()

   LOCAL a
   LOCAL aData, aO[CONNECTORS]

   if ! hb_mtvm()
      ? "This program needs HVM with MT support"
      quit
   endif

   hb_SetEnv( 'WDO_PATH_MYSQL', "c:/xampp/apache/bin/" )

   ? "Creating ", CONNECTORS, " MySQL connectors..."

   for i = 1 to CONNECTORS

      aO[i] := WDO():Rdbms( 'MYSQL', "192.168.0.9", "harbour", "", "harbourdb", 3306 )

      IF !aO[i]:lConnect
         ? aO[i]:cError
      endif

   next

   nTotTime := 0

   for i = 1 to QUERYS

      hb_threadStart( @Query(), aO[ iif( CONNECTORS - i <= 0, CONNECTORS, i ) ], i )

   next

   hb_threadWaitForAll()
   ? "Total process time: ", nTotTime, "ms"

RETURN NIL

function Query( o, nTh )

   LOCAL aData, hRes := {=>}, nThTime

   ? "Getting data... Thread:", nTh
   a =  hb_MilliSeconds()
   hRes := o:Query( "select count(*) from db where KAR_TIPO = 1 AND KAR_TIPO = 1" )
   aData := o:FetchAll( hRes )
   ? "OK Thread:", nTh
   nThTime := hb_MilliSeconds() - a
   nTotTime += nThTime
   ? "Total time Thread:", nTh , " ", nThTime, "ms"
   ? aData[1][1]

RETURN
