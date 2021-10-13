/*  $Id$  */

/*
 * Harbour Project source code:
 * Leto db server BMDBF* functions
 *
 * Copyright 2013 Pavel Tsarenko <tpe2 / at / mail.ru>
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

FUNCTION LBM_DbGetFilterArray( nUserStru )
   LOCAL aFilterRec

   leto_BMRestore( nUserStru )
   aFilterRec := BM_DbGetFilterArray()
   leto_BMSave( nUserStru )
   RETURN leto_ATOC( aFilterRec )

FUNCTION LBM_DbSetFilterArray( nUserStru, cFilterRec, aFilterRec )
   IF ValType( cFilterRec ) = "C"
      aFilterRec := leto_CTOA( cFilterRec )
   ENDIF
   IF ValType( aFilterRec ) = "A"
      leto_BMRestore( nUserStru, .T. )
      BM_DbSetFilterArray( aFilterRec )
      leto_BMSave( nUserStru )
   ENDIF
   RETURN Nil

FUNCTION LBM_DbSetFilterArrayAdd( nUserStru, cFilterRec )
   IF ValType( cFilterRec ) = "C"
      leto_BMRestore( nUserStru )
      BM_DbSetFilterArrayAdd( leto_CTOA( cFilterRec ) )
      leto_BMSave( nUserStru )
   ENDIF
   RETURN Nil

FUNCTION LBM_DbSetFilterArrayDel( nUserStru, cFilterRec )
   IF ValType( cFilterRec ) = "C"
      leto_BMRestore( nUserStru )
      BM_DbSetFilterArrayDel( leto_CTOA( cFilterRec ) )
      leto_BMSave( nUserStru )
   ENDIF
   RETURN Nil

/*
 * LBM_DbSetFilter set bitmap filter by order <xOrder>, and for condition,
 * defined in <xScope>, <xScopeBottom>, <cFilter>, <lDeleted> parameters
 * Returns buffer with first filtered record
 * Function call from client:
   
   leto_ParseRec( leto_Udf('LBM_DbSetFilter', <xScope>, <xScopeBottom>, <xOrder>, <cFilter>, <lDeleted> ) )
 */
FUNCTION LBM_DbSetFilter( nUserStru, xScope, xScopeBottom, xOrder, cFilter, lDeleted )
   LOCAL cRec, aFilterRec := {}
   LOCAL lOpt := Set( _SET_FORCEOPT, .F. )
   leto_SetEnv( xScope, xScopeBottom, xOrder, cFilter, lDeleted )
   GO TOP
   cRec := leto_rec( nUserStru )
   WHILE ! Eof()
      AADD( aFilterRec, RecNo() )
      SKIP
   ENDDO
   cRec := leto_rec( nUserStru )
   leto_ClearEnv( xScope, xScopeBottom, cFilter )
   LBM_DbSetFilterArray( nUserStru,, aFilterRec )
   Set( _SET_FORCEOPT, lOpt )
   RETURN cRec
