/*	----------------------------------------------------------------------------
	Name:			LIB WDO - Web Database Objects
	Description: 	Acceso a Base de Datos
	Autor:			Carles Aubia
	Date: 			26/07/19	
-------------------------------------------------------------------------------- */

//	Se han de definir estos comandos pues los usamos en algunos m├│dulos...
//	-------------------------------------------------------------------------------- 

#include "hbclass.ch" 
#include "hboo.ch"   
#include "hbhash.ch"
#include "hbextern.ch"

#include "wdo.prg" 					//	WDO
#include "rdbms.prg" 				//	Base RDBMS
#ifdef WITH_ADO
	#include "rdbms_ado.prg" 		//	ADO
#endif
#include "rdbms_dbf.prg" 			//	Dbf
#include "rdbms_mysql.prg" 			//	MySql
//#include "rdbms_pg.prg" 			//	PostgreSql
#include "tdataset.prg" 			//	Clase TDataset
//	---------------------------------------------------------------------------- //
