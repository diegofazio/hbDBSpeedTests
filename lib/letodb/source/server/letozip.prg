/*  $Id$  */

/*
 * Harbour Project source code:
 * Leto db server zip/unzip functions
 *
 * Copyright 2015 Pavel Tsarenko <tpe2 / at / mail.ru>
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

#ifdef __LINUX__
   #define DEF_SEP      '/'
#else
   #define DEF_SEP      '\'
#endif

FUNCTION leto_Zip( nUserStru, cDirName, acFiles, nLevel, lOverwrite, cPassword, acExclude, lWithPath )
   LOCAL cZip, cOldDir, lSuccess
   LOCAL cFileName := "letotemp.zip"
   LOCAL cPath := leto_GetAppOptions( 1 )

   HB_SYMBOL_UNUSED( nUserStru )
   hb_default( @lWithPath, .T. )

   IF ! Empty( cDirName )
      IF Right( cPath, 1 ) != DEF_SEP
         cPath += DEF_SEP
      ENDIF
      cPath += cDirName
   ENDIF
   IF ! Empty( cPath )
      cOldDir := hb_CWD( cPath )
   ENDIF

   lSuccess := hb_ZipFile( ;
      cFileName, ;
      acFiles, ;
      nLevel, ;
      , ;
      lOverwrite, ;
      cPassword, ;
      lWithPath, ;
      , ;
      , ;
      .T., ;
      acExclude )

   IF lSuccess .and. File( cFileName )
      cZip := hb_MemoRead( cFileName )
      FErase( cFileName )
   ENDIF

   IF ! Empty( cOldDir )
      hb_CWD( cOldDir )
   ENDIF

   RETURN cZip

FUNCTION leto_UnZip( nUserStru, cDirName, cZip, cPassword, lWithPath )
   LOCAL cFileName := "letotemp.zip"
   LOCAL cPath := leto_GetAppOptions( 1 )
   LOCAL lSuccess, cOldDir

   HB_SYMBOL_UNUSED( nUserStru )
   hb_default( @lWithPath, .T. )

   IF ! Empty( cDirName )
      IF Right( cPath, 1 ) != DEF_SEP
         cPath += DEF_SEP
      ENDIF
      cPath += cDirName
   ENDIF
   IF ! Empty( cPath )
      cOldDir := hb_CWD( cPath )
   ENDIF

   lSuccess := hb_MemoWrit( cFileName, cZip )

   IF lSuccess

      lSuccess := hb_UnzipFile( cFileName, , lWithPath, cPassword, , , )
      IF lSuccess
         FErase( cFileName )
      ENDIF

   ENDIF

   IF ! Empty( cOldDir )
      hb_CWD( cOldDir )
   ENDIF

   RETURN lSuccess
