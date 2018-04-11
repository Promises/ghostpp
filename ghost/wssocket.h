//
// Created by Michael Schneider on 18.10.17.
//

#ifndef PROJECTS_WSSOCKET_H
#define PROJECTS_WSSOCKET_H


class CTCPClient;

struct WSHeader {
    unsigned header_size;
    bool fin;
    bool mask;
    enum opcode_type {
        CONTINUTATION = 0x0,
        TEXT_FRAME = 0x1,
        BINARY_FRAME = 0x2,
        CLOSE = 8,
        PING = 9,
        PONG = 0xa,
    } opcode;
    int N0;
    uint64_t N;
    uint8_t masking_key[4];
};

struct WSAction {
    enum action_type {
        AUTH = 0,
        GAME_ACTION = 1,
        PLAYER_ACTION = 2
    };
};

struct WSGameAction {
    enum type {
        CREATE = 0,
        FINISH = 1,
        STARTED = 2,
        MESSAGE = 3
    };
};

struct WSPlayerAction {
    enum type {
        JOINED = 0,
        LEFT = 1,
        SLOT_CHANGED = 2,
        PING_CHANGED = 3,
        MESSAGE = 4,
    };
};

class WSSocket {

private:
    CTCPClient *m_Socket;
    bool m_Connected;
    bool m_HandshakeShared;
    string m_IP;
    uint16_t m_Port;
    uint32_t m_LastPing;
    std::vector<uint8_t> rxbuf;
    bool m_WaitingToConnect;
    uint32_t m_LastConnectTry;
    uint32_t m_BotId;

public:
    WSSocket( string *m_IP, uint16_t m_Port, uint32_t m_BotId );
    virtual ~WSSocket( );

    bool Update( void *fd, void *send_fd );
    unsigned int SetFD( void *fd, void *send_fd, int *nfds );
    void ExtractPackets( );
    void Connect( void *send_fd );
    void sendData(WSHeader::opcode_type type, string message);
    void SendAction(WSAction::action_type type, string message);
    void SendGameAction(WSGameAction::type type, string gamename, string message);
    void SendPlayerAction(WSPlayerAction::type type, string gamename, string name, string realm, unsigned char sid, string msg);
    void ProcessMessage( string msg );
    void recvData( );
};

#endif //PROJECTS_WSSOCKET_H
