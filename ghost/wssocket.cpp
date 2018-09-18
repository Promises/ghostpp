#include "ghost.h"
#include "socket.h"
#include "wssocket.h"
#include "util.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

WSSocket :: WSSocket(string *nIP, uint16_t nPort, uint32_t nBotId) : m_IP( *nIP ), m_Port( nPort ), m_BotId( nBotId )
{
    m_Socket = new CTCPClient();
    m_Connected = false;
    m_HandshakeShared = false;
    m_LastPing = 0;
    m_WaitingToConnect  = false;
    m_LastConnectTry = 0;
}

WSSocket :: ~WSSocket( )
{
    delete m_Socket;
}


bool WSSocket :: Update( void *fd, void *send_fd )
{
    if( !m_Socket->GetConnecting( ) && !m_Socket->GetConnected( ) && !m_WaitingToConnect)
    {
        CONSOLE_Print("[WSSocket] Socket is not connected, reseting socket");
        m_Socket->Reset( );
        m_HandshakeShared = false;
	m_WaitingToConnect = true;
    }

    if(m_Socket->GetConnected( )) {
        m_Socket->DoRecvPlain((fd_set *)fd);
        ExtractPackets( );
        m_Socket->DoSendPlain( (fd_set *)send_fd);
	if(! m_HandshakeShared) {
            CONSOLE_Print("[WSSocket] Sending handshake");
            m_HandshakeShared = true;
        }
        if(m_HandshakeShared && (GetTime( ) - m_LastPing > 15 || m_LastPing == 0 )) {
     	    sendData(WSHeader::PONG, string());
            m_LastPing = GetTime( );
        }
    }

    if( m_Socket->GetConnecting( ) ) {
        if( m_Socket->CheckConnect( ) ) {
	    m_Socket->DoRecvPlain((fd_set *)fd);
	    ExtractPackets( );
	    m_Socket->DoSendPlain( (fd_set *)send_fd);
        } else {
            CONSOLE_Print("[WSSocket] Connection check failed, reseting socket");
            m_Socket->Reset( );
            m_HandshakeShared = false;
	    m_WaitingToConnect = false;
        }
    }

    if( !m_Socket->GetConnecting( ) && !m_Socket->GetConnected( ) && ( GetTime() - m_LastConnectTry >= 15 ) || m_LastConnectTry == 0) {
        CONSOLE_Print("[WSSocket] Server isn't connected, init a new connection");
        Connect(&send_fd);
	m_LastConnectTry = GetTime( );
    }
}

unsigned int WSSocket :: SetFD( void *fd, void *send_fd, int *nfds )
{
    unsigned int NumFDs = 0;

    if( m_Socket && !m_Socket->HasError( ) && m_Socket->GetConnected( )) {
        m_Socket->SetFD( (fd_set *)fd, (fd_set *)send_fd, nfds );
        ++NumFDs;
    }
    return NumFDs;
}

void WSSocket :: ExtractPackets( )
{
    string *RecvBuffer = m_Socket->GetBytes( );
    string Buffer = *RecvBuffer;
    // a packet is at least 4 bytes so loop as long as the buffer contains 4 bytes
    if( !Buffer.empty( ) )
    {
        uint32_t length = Buffer.size();
        if(length > 0 ) {
            string message = Buffer.substr(0, length);
            ProcessMessage(message);
	    *RecvBuffer = RecvBuffer->substr(length);
        }
    }
}

void WSSocket :: Connect( void *send_fd )
{
    m_Socket = new CTCPClient();
    m_Socket->Connect("", m_IP, m_Port);

    if(m_Socket->HasError( )) {
        CONSOLE_Print("[WSSocket] disconnected from WSSocket due socket error");
        CONSOLE_Print("[WSSocket] "+m_Socket->GetErrorString( ));
    } else {
        CONSOLE_Print("[WSSocket] Got any response from server (could be cloudflare). Sending handshake request ...");
        string toSend="GET / HTTP/1.1\r\nHost: "+m_IP+":"+UTIL_ToString(m_Port)+"\r\nUpgrade: websocket\r\nConnection: Keep-Alive, Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://dota.vision\r\nSec-Websocket-Protocol: chat, superchat\r\nSec-WebSocket-Version: 13\r\n\r\n";
        m_Socket->PutBytes(toSend);
        m_Socket->DoSendPlain( (fd_set *)send_fd);
    }
}


void WSSocket :: sendData(WSHeader::opcode_type type, string message) {
    if(type != WSHeader::PONG) {
	    CONSOLE_Print("[WSSocket] Sending data: " + message);
    }

    if( message.find("[WSSocket]") != -1) {
        return;
    }

    const uint8_t masking_key[4] = { 0x12, 0x34, 0x56, 0x78 };
    std::vector<uint8_t> header;
    std::vector<uint8_t> txbuf;

    uint64_t message_size = message.size();

    header.assign(2 + (message_size >= 126 ? 2 : 0) + (message_size >= 65536 ? 6 : 0) + 4, 0);
    header[0] = 0x80 | type;

    if (false) { }
    else if (message_size < 126) {
        header[1] = (message_size & 0xff) | 0x80;
        header[2] = masking_key[0];
        header[3] = masking_key[1];
        header[4] = masking_key[2];
        header[5] = masking_key[3];
    }
    else if (message_size < 65536) {
        header[1] = 126 | 0x80;
        header[2] = (message_size >> 8) & 0xff;
        header[3] = (message_size >> 0) & 0xff;
        header[4] = masking_key[0];
        header[5] = masking_key[1];
        header[6] = masking_key[2];
        header[7] = masking_key[3];
    }
    else {
        header[1] = 127 | 0x80;
        header[2] = (message_size >> 56) & 0xff;
        header[3] = (message_size >> 48) & 0xff;
        header[4] = (message_size >> 40) & 0xff;
        header[5] = (message_size >> 32) & 0xff;
        header[6] = (message_size >> 24) & 0xff;
        header[7] = (message_size >> 16) & 0xff;
        header[8] = (message_size >> 8) & 0xff;
        header[9] = (message_size >> 0) & 0xff;
        header[10] = masking_key[0];
        header[11] = masking_key[1];
        header[12] = masking_key[2];
        header[13] = masking_key[3];
    }

    txbuf.insert(txbuf.end(), header.begin(), header.end());
    txbuf.insert(txbuf.end(), message.begin(), message.end());

    for (size_t i = 0; i != message.size(); ++i) {
        *(txbuf.end() - message.size() + i) ^= masking_key[i&0x3];
    }

    m_Socket->PutBytes(txbuf);
}

void WSSocket :: recvData( ) {
    WSHeader ws = WSHeader();
    if(rxbuf.size()<2) { return; }

    const uint8_t * data = (uint8_t *) &rxbuf[0];
    ws.fin = (data[0] & 0x80) == 0x80;
    ws.opcode = (WSHeader::opcode_type) (data[0] & 0x0f);
    ws.mask = (data[1] & 0x80) == 0x80;
    ws.N0 = (data[1] & 0x7f);
    ws.header_size = 2 + (ws.N0 == 126? 2 : 0) + (ws.N0 == 127? 6 : 0) + (ws.mask? 4 : 0);

    if (rxbuf.size() < ws.header_size) { return; }
    int i;
    if (ws.N0 < 126) {
        ws.N = ws.N0;
        i = 2;
    }
    else if (ws.N0 == 126) {
        ws.N = 0;
        ws.N |= ((uint64_t) data[2]) << 8;
        ws.N |= ((uint64_t) data[3]) << 0;
        i = 4;
    }
    else if (ws.N0 == 127) {
        ws.N = 0;
        ws.N |= ((uint64_t) data[2]) << 56;
        ws.N |= ((uint64_t) data[3]) << 48;
        ws.N |= ((uint64_t) data[4]) << 40;
        ws.N |= ((uint64_t) data[5]) << 32;
        ws.N |= ((uint64_t) data[6]) << 24;
        ws.N |= ((uint64_t) data[7]) << 16;
        ws.N |= ((uint64_t) data[8]) << 8;
        ws.N |= ((uint64_t) data[9]) << 0;
        i = 10;
    }
    if (ws.mask) {
        ws.masking_key[0] = ((uint8_t) data[i+0]) << 0;
        ws.masking_key[1] = ((uint8_t) data[i+1]) << 0;
        ws.masking_key[2] = ((uint8_t) data[i+2]) << 0;
        ws.masking_key[3] = ((uint8_t) data[i+3]) << 0;
    }
    else {
        ws.masking_key[0] = 0;
        ws.masking_key[1] = 0;
        ws.masking_key[2] = 0;
        ws.masking_key[3] = 0;
    }
    if (rxbuf.size() < ws.header_size+ws.N) { return; }

    if (ws.opcode == WSHeader::TEXT_FRAME && ws.fin) {
        if (ws.mask) {
            for (size_t i = 0; i != ws.N; ++i) {
                rxbuf[i+ws.header_size] ^= ws.masking_key[i&0x3];
            }
        }
        std::string d(rxbuf.begin()+ws.header_size, rxbuf.begin()+ws.header_size+(size_t)ws.N);
        ProcessMessage(d);
    } else if (ws.opcode == WSHeader::PING) {
        if (ws.mask) {
            for (size_t i = 0; i != ws.N; ++i) {
                rxbuf[i+ws.header_size] ^= ws.masking_key[i&0x3];
            }
        }
        std::string d(rxbuf.begin()+ws.header_size, rxbuf.begin()+ws.header_size+(size_t)ws.N);
        sendData(WSHeader::PONG, d);
    }
    else if (ws.opcode == WSHeader::PONG) { }
    else if (ws.opcode == WSHeader::CLOSE) { }

    rxbuf.erase(rxbuf.begin(), rxbuf.begin() + ws.header_size+(size_t)ws.N);
}


bool invalidChar (char c)
{
    return !(c>=0 && c <128);
}

void WSSocket :: ProcessMessage( string msg ) {
    msg.erase(remove_if(msg.begin(), msg.end(), invalidChar), msg.end());

    if(msg.find("Switching Protocols")!=std::string::npos) {
        CONSOLE_Print("[WSSocket] handshake complete");
        CONSOLE_Print("[WSSocket] Successfully connected to socket");
        sendData(WSHeader::PONG, std::string());
        SendAction(WSAction::AUTH, UTIL_ToString(m_BotId));
        m_HandshakeShared = true;
        m_Connected = true;
    } else {
	    boost::property_tree::ptree ptree;
        // TODO: catch errors?!
        boost::property_tree::read_json(msg, ptree);
        uint32_t action = ptree.get<uint32_t>("action");
        if(action == WSAction::COMMAND) {
            ProcessCommand(ptree);
        }
        CONSOLE_Print("[WSSocket] Recieved message: " + msg);
    }
}

void WSSocket :: SendAction(WSAction::action_type type, string message) {
    boost::property_tree::ptree ptree;
    ptree.put("action", type);
    ptree.put("msg", message);

    std::stringstream ss;
    boost::property_tree::json_parser::write_json(ss, ptree);
    sendData(WSHeader::TEXT_FRAME, ss.str());
}

void WSSocket :: SendGameAction(WSGameAction::type type, string gamename, string message) {
    boost::property_tree::ptree ptree;
    ptree.put("action", WSAction::GAME_ACTION);
    ptree.put("subaction", type);
    ptree.put("gamename", gamename);
    ptree.put("msg", message);

    std::stringstream ss;
    boost::property_tree::json_parser::write_json(ss, ptree);
    sendData(WSHeader::TEXT_FRAME, ss.str());
}

void WSSocket :: SendPlayerAction(WSPlayerAction::type type, string gamename, string name, string realm, unsigned char sid, string msg) {
    boost::property_tree::ptree ptree;
    ptree.put("action", WSAction::PLAYER_ACTION);
    ptree.put("subaction", type);
    ptree.put("gamename", gamename);
    ptree.put("name", name);
    ptree.put("realm", realm);
    ptree.put("sid", sid);
    ptree.put("msg", msg);

    std::stringstream ss;
    boost::property_tree::json_parser::write_json(ss, ptree);
    sendData(WSHeader::TEXT_FRAME, ss.str());
}

void WSSocket :: ProcessCommand(boost::property_tree::ptree ptree) {

}