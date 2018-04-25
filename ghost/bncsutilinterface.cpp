/*

	ent-ghost
	Copyright [2011-2013] [Jack Lu]

	This file is part of the ent-ghost source code.

	ent-ghost is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	ent-ghost source code is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with ent-ghost source code. If not, see <http://www.gnu.org/licenses/>.

	ent-ghost is modified from GHost++ (http://ghostplusplus.googlecode.com/)
	GHost++ is Copyright [2008] [Trevor Hogan]

*/

#include "ghost.h"
#include "util.h"
#include "bncsutilinterface.h"

#include <bncsutil/bncsutil.h>

//
// CBNCSUtilInterface
//

CBNCSUtilInterface :: CBNCSUtilInterface( string userName, string userPassword )
{
	// m_nls = (void *)nls_init( userName.c_str( ), userPassword.c_str( ) );
	m_NLS = new NLS( userName, userPassword );
}

CBNCSUtilInterface :: ~CBNCSUtilInterface( )
{
	// nls_free( (nls_t *)m_nls );
	delete (NLS *)m_NLS;
}

void CBNCSUtilInterface :: Reset( string userName, string userPassword )
{
	// nls_free( (nls_t *)m_nls );
	// m_nls = (void *)nls_init( userName.c_str( ), userPassword.c_str( ) );
	delete (NLS *)m_NLS;
	m_NLS = new NLS( userName, userPassword );
}

bool CBNCSUtilInterface :: HELP_SID_AUTH_CHECK( bool TFT, string &war3Path, string &keyROC, string &keyTFT, string valueStringFormula, string mpqFileName, BYTEARRAY &clientToken, BYTEARRAY &serverToken )
{
    string FileWar3EXE = war3Path + "Warcraft III.exe";

	bool ExistsWar3EXE = UTIL_FileExists( FileWar3EXE );
	if (!UTIL_FileExists( FileWar3EXE ))
		return false;

}

bool CBNCSUtilInterface :: HELP_SID_AUTH_ACCOUNTLOGON( )
{
	// set m_ClientKey

	char buf[32];
	// nls_get_A( (nls_t *)m_nls, buf );
	( (NLS *)m_NLS )->getPublicKey( buf );
	m_ClientKey = UTIL_CreateByteArray( (unsigned char *)buf, 32 );
	return true;
}

bool CBNCSUtilInterface :: HELP_SID_AUTH_ACCOUNTLOGONPROOF( BYTEARRAY salt, BYTEARRAY serverKey )
{
	// set m_M1

	char buf[20];
	// nls_get_M1( (nls_t *)m_nls, buf, string( serverKey.begin( ), serverKey.end( ) ).c_str( ), string( salt.begin( ), salt.end( ) ).c_str( ) );
	( (NLS *)m_NLS )->getClientSessionKey( buf, string( salt.begin( ), salt.end( ) ).c_str( ), string( serverKey.begin( ), serverKey.end( ) ).c_str( ) );
	m_M1 = UTIL_CreateByteArray( (unsigned char *)buf, 20 );
	return true;
}

bool CBNCSUtilInterface :: HELP_PvPGNPasswordHash( string userPassword )
{
	// set m_PvPGNPasswordHash

	char buf[20];
	hashPassword( userPassword.c_str( ), buf );
	m_PvPGNPasswordHash = UTIL_CreateByteArray( (unsigned char *)buf, 20 );
	return true;
}

BYTEARRAY CBNCSUtilInterface :: CreateKeyInfo( string key, uint32_t clientToken, uint32_t serverToken )
{
	BYTEARRAY KeyInfo;
	CDKeyDecoder Decoder( key.c_str( ), key.size( ) );

	if( Decoder.isKeyValid( ) )
	{
        unsigned char Zeros[] = { 0, 0, 0, 0 };
		UTIL_AppendByteArray( KeyInfo, UTIL_CreateByteArray( (uint32_t)key.size( ), false ) );
		UTIL_AppendByteArray( KeyInfo, UTIL_CreateByteArray( Decoder.getProduct( ), false ) );
		UTIL_AppendByteArray( KeyInfo, UTIL_CreateByteArray( Decoder.getVal1( ), false ) );
		UTIL_AppendByteArray( KeyInfo, UTIL_CreateByteArray( Zeros, 4 ) );
		size_t Length = Decoder.calculateHash( clientToken, serverToken );
		char *buf = new char[Length];
		Length = Decoder.getHash( buf );
		UTIL_AppendByteArray( KeyInfo, UTIL_CreateByteArray( (unsigned char *)buf, Length ) );
		delete [] buf;
	}

	return KeyInfo;
}
