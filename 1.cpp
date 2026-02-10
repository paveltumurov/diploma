// platform detection
using namespace std;
#define PLATFORM_WINDOWS  1
#define PLATFORM_MAC      2
#define PLATFORM_UNIX     3

#if defined(_WIN32)
#define PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define PLATFORM PLATFORM_MAC
#else
#define PLATFORM PLATFORM_UNIX
#endif
#if PLATFORM == PLATFORM_WINDOWS

    #include <winsock2.h>

#elif PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX

    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <fcntl.h>

#endif
#if PLATFORM == PLATFORM_WINDOWS
#pragma comment( lib, "wsock32.lib" )
#endif

    inline bool InitializeSockets()
{
    #if PLATFORM == PLATFORM_WINDOWS
    WSADATA WsaData;
    return WSAStartup( MAKEWORD(2,2), &WsaData ) == NO_ERROR;
    #else
    return true;
    #endif
}

inline void ShutdownSockets()
{
    #if PLATFORM == PLATFORM_WINDOWS
    WSACleanup();
    #endif
}

int handle = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

if ( handle <= 0 )
{
    printf( "failed to create socket\n" );
    return false;
}

sockaddr_in address;
address.sin_family = AF_INET;
address.sin_addr.s_addr = INADDR_ANY;
address.sin_port = htons( (unsigned short) port );

if ( bind( handle, (const sockaddr*) &address, sizeof(sockaddr_in) ) < 0 )
{
    printf( "failed to bind socket\n" );
    return false;
}

#if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX

    int nonBlocking = 1;
    if ( fcntl( handle, F_SETFL, O_NONBLOCK, nonBlocking ) == -1 )
    {
        printf( "failed to set non-blocking socket\n" );
        return false;
    }

#elif PLATFORM == PLATFORM_WINDOWS

    DWORD nonBlocking = 1;
    if ( ioctlsocket( handle, FIONBIO, &nonBlocking ) != 0 )
    {
        printf( "failed to set non-blocking socket\n" );
        return false;
    }

#endif

int sent_bytes = sendto( handle, (const char*)packet_data, packet_size,
                            0, (sockaddr*)&address, sizeof(sockaddr_in) );

if ( sent_bytes != packet_size )
{
    printf( "failed to send packet: return value = %d\n", sent_bytes );
    return false;
}


unsigned int a = 207;
unsigned int b = 45;
unsigned int c = 186;
unsigned int d = 98;
unsigned short port = 30000;

unsigned int destination_address = ( a << 24 ) | ( b << 16 ) | ( c << 8 ) | d;
unsigned short destination_port = port;

sockaddr_in address;
address.sin_family = AF_INET;
address.sin_addr.s_addr = htonl( destination_address );
address.sin_port = htons( destination_port );

while (true)
{
    unsigned char packet_data[256];
    unsigned int maximum_packet_size = sizeof( packet_data );

    #if PLATFORM == PLATFORM_WINDOWS
    typedef int socklen_t;
    #endif

    sockaddr_in from;
    socklen_t fromLength = sizeof( from );

    int received_bytes = recvfrom( socket, (char*)packet_data, maximum_packet_size,
                                0, (sockaddr*)&from, &fromLength );

    if ( received_bytes <= 0 )
        break;

    unsigned int from_address = ntohl( from.sin_addr.s_addr );
    unsigned int from_port = ntohs( from.sin_port );

    // process received packet
}

#if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
close( socket );
#elif PLATFORM == PLATFORM_WINDOWS
closesocket( socket );
#endif

class Socket
{
public:

    Socket();
    ~Socket();
    bool Open( unsigned short port );
    void Close();
    bool IsOpen() const;
    bool Send( const Address & destination, const void * data, int size );
    int Receive( Address & sender, void * data, int size );

private:

    int handle;
};

class Address
{
public:

    Address();
    Address( unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned short port );
    Address( unsigned int address, unsigned short port );
    unsigned int GetAddress() const;
    unsigned char GetA() const;
    unsigned char GetB() const;
    unsigned char GetC() const;
    unsigned char GetD() const;
    unsigned short GetPort() const;
    bool operator == ( const Address & other ) const;
    bool operator != ( const Address & other ) const;

private:

    unsigned int address;
    unsigned short port;
};

// create socket

const int port = 30000;
Socket socket;
if ( !socket.Open( port ) )
{
    printf( "failed to create socket!\n" );
    return false;
}

// send a packet

const char data[] = "hello world!";
socket.Send( Address(127,0,0,1,port), data, sizeof( data ) );

// receive packets

while ( true )
{
    Address sender;
    unsigned char buffer[256];
    int bytes_read = socket.Receive( sender, buffer, sizeof( buffer ) );
    if ( !bytes_read )
        break;
    // process packet
}

