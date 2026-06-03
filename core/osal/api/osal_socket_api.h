/************************************************************************
 * OSAL Public API - Network Socket Operations
 *
 * This file provides the public API for network socket operations
 * including TCP, UDP, and CAN sockets.
 ************************************************************************/

#ifndef OSAL_SOCKET_API_H
#define OSAL_SOCKET_API_H

#include "osal_types_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************
 * Socket Creation and Management
 ************************************************************************/

/**
 * @brief Create a socket
 *
 * Creates an endpoint for communication.
 *
 * @param domain Communication domain (AF_INET, AF_INET6, AF_CAN, etc.)
 * @param type Socket type (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW)
 * @param protocol Protocol (0 for default, IPPROTO_TCP, IPPROTO_UDP, etc.)
 * @return Socket file descriptor on success, negative error code on failure
 *
 * @note Caller must close the socket with OSAL_close()
 */
int32_t OSAL_socket(int32_t domain, int32_t type, int32_t protocol);

/**
 * @brief Bind socket to address
 *
 * Assigns a local address to a socket.
 *
 * @param sockfd Socket file descriptor
 * @param addr Socket address structure
 * @param addrlen Size of address structure
 * @return OSAL_SUCCESS on success, negative error code on failure
 */
int32_t OSAL_bind(int32_t sockfd, const osal_sockaddr_t *addr, uint32_t addrlen);

/**
 * @brief Listen for connections
 *
 * Marks a socket as passive, ready to accept connections.
 *
 * @param sockfd Socket file descriptor
 * @param backlog Maximum queue length for pending connections
 * @return OSAL_SUCCESS on success, negative error code on failure
 *
 * @note Only applicable to SOCK_STREAM sockets
 */
int32_t OSAL_listen(int32_t sockfd, int32_t backlog);

/**
 * @brief Accept a connection
 *
 * Extracts the first connection request from the queue of pending
 * connections and creates a new connected socket.
 *
 * @param sockfd Listening socket file descriptor
 * @param[out] addr Address of connecting peer (may be NULL)
 * @param[in,out] addrlen Size of addr buffer (updated with actual size)
 * @return New socket file descriptor on success, negative error code on failure
 *
 * @note Blocks until a connection arrives (unless socket is non-blocking)
 */
int32_t OSAL_accept(int32_t sockfd, osal_sockaddr_t *addr, uint32_t *addrlen);

/**
 * @brief Connect to remote address
 *
 * Initiates a connection to a remote socket.
 *
 * @param sockfd Socket file descriptor
 * @param addr Remote socket address
 * @param addrlen Size of address structure
 * @return OSAL_SUCCESS on success, negative error code on failure
 *
 * @note For SOCK_STREAM: establishes connection
 * @note For SOCK_DGRAM: sets default destination for send/recv
 */
int32_t OSAL_connect(int32_t sockfd, const osal_sockaddr_t *addr, uint32_t addrlen);

/**
 * @brief Shutdown socket
 *
 * Shuts down part or all of a full-duplex connection.
 *
 * @param sockfd Socket file descriptor
 * @param how SHUT_RD, SHUT_WR, or SHUT_RDWR
 * @return OSAL_SUCCESS on success, negative error code on failure
 */
int32_t OSAL_shutdown(int32_t sockfd, int32_t how);

/************************************************************************
 * Data Transfer
 ************************************************************************/

/**
 * @brief Send data on connected socket
 *
 * Transmits data to a connected socket.
 *
 * @param sockfd Socket file descriptor
 * @param buf Data buffer
 * @param len Number of bytes to send
 * @param flags Send flags (MSG_DONTWAIT, MSG_NOSIGNAL, etc.)
 * @return Number of bytes sent on success, negative error code on failure
 *
 * @note May send fewer bytes than requested
 */
int32_t OSAL_send(int32_t sockfd, const void *buf, uint32_t len, int32_t flags);

/**
 * @brief Receive data from connected socket
 *
 * Receives data from a connected socket.
 *
 * @param sockfd Socket file descriptor
 * @param[out] buf Buffer to store received data
 * @param len Buffer size
 * @param flags Receive flags (MSG_DONTWAIT, MSG_PEEK, etc.)
 * @return Number of bytes received on success, 0 on EOF, negative on error
 */
int32_t OSAL_recv(int32_t sockfd, void *buf, uint32_t len, int32_t flags);

/**
 * @brief Send data to specific address
 *
 * Transmits data to a specific destination (for datagram sockets).
 *
 * @param sockfd Socket file descriptor
 * @param buf Data buffer
 * @param len Number of bytes to send
 * @param flags Send flags
 * @param dest_addr Destination address
 * @param addrlen Size of destination address
 * @return Number of bytes sent on success, negative error code on failure
 */
int32_t OSAL_sendto(int32_t sockfd, const void *buf, uint32_t len, int32_t flags,
                    const osal_sockaddr_t *dest_addr, uint32_t addrlen);

/**
 * @brief Receive data with source address
 *
 * Receives data and stores the sender's address.
 *
 * @param sockfd Socket file descriptor
 * @param[out] buf Buffer to store received data
 * @param len Buffer size
 * @param flags Receive flags
 * @param[out] src_addr Source address (may be NULL)
 * @param[in,out] addrlen Size of address buffer (updated with actual size)
 * @return Number of bytes received on success, negative error code on failure
 */
int32_t OSAL_recvfrom(int32_t sockfd, void *buf, uint32_t len, int32_t flags,
                      osal_sockaddr_t *src_addr, uint32_t *addrlen);

/************************************************************************
 * Socket Options
 ************************************************************************/

/**
 * @brief Set socket option
 *
 * Sets options on a socket.
 *
 * @param sockfd Socket file descriptor
 * @param level Protocol level (SOL_SOCKET, IPPROTO_TCP, etc.)
 * @param optname Option name (SO_REUSEADDR, SO_RCVBUF, etc.)
 * @param optval Option value buffer
 * @param optlen Size of option value
 * @return OSAL_SUCCESS on success, negative error code on failure
 */
int32_t OSAL_setsockopt(int32_t sockfd, int32_t level, int32_t optname,
                        const void *optval, uint32_t optlen);

/**
 * @brief Get socket option
 *
 * Retrieves options from a socket.
 *
 * @param sockfd Socket file descriptor
 * @param level Protocol level
 * @param optname Option name
 * @param[out] optval Buffer to store option value
 * @param[in,out] optlen Size of buffer (updated with actual size)
 * @return OSAL_SUCCESS on success, negative error code on failure
 */
int32_t OSAL_getsockopt(int32_t sockfd, int32_t level, int32_t optname,
                        void *optval, uint32_t *optlen);

/************************************************************************
 * Network Interface
 ************************************************************************/

/**
 * @brief Convert interface name to index
 *
 * Maps a network interface name to its numeric index.
 *
 * @param ifname Interface name (e.g., "eth0", "can0")
 * @return Interface index on success, 0 on failure
 */
uint32_t OSAL_if_nametoindex(const char *ifname);

/**
 * @brief Convert interface index to name
 *
 * Maps a network interface index to its name.
 *
 * @param ifindex Interface index
 * @param[out] ifname Buffer to store interface name (size: IF_NAMESIZE)
 * @return Interface name on success, NULL on failure
 */
const char *OSAL_if_indextoname(uint32_t ifindex, char *ifname);

/************************************************************************
 * Byte Order Conversion
 ************************************************************************/

/**
 * @brief Convert 16-bit value from host to network byte order
 *
 * @param hostshort Value in host byte order
 * @return Value in network byte order (big-endian)
 */
uint16_t OSAL_htons(uint16_t hostshort);

/**
 * @brief Convert 32-bit value from host to network byte order
 *
 * @param hostlong Value in host byte order
 * @return Value in network byte order (big-endian)
 */
uint32_t OSAL_htonl(uint32_t hostlong);

/**
 * @brief Convert 16-bit value from network to host byte order
 *
 * @param netshort Value in network byte order
 * @return Value in host byte order
 */
uint16_t OSAL_ntohs(uint16_t netshort);

/**
 * @brief Convert 32-bit value from network to host byte order
 *
 * @param netlong Value in network byte order
 * @return Value in host byte order
 */
uint32_t OSAL_ntohl(uint32_t netlong);

/************************************************************************
 * Address Conversion
 ************************************************************************/

/**
 * @brief Convert IP address from text to binary form
 *
 * @param af Address family (AF_INET or AF_INET6)
 * @param src IP address string
 * @param[out] dst Binary address buffer
 * @return 1 on success, 0 on invalid format, negative on error
 */
int32_t OSAL_inet_pton(int32_t af, const char *src, void *dst);

/**
 * @brief Convert IP address from binary to text form
 *
 * @param af Address family (AF_INET or AF_INET6)
 * @param src Binary address buffer
 * @param[out] dst String buffer (size: INET_ADDRSTRLEN or INET6_ADDRSTRLEN)
 * @param size Size of string buffer
 * @return Pointer to dst on success, NULL on failure
 */
const char *OSAL_inet_ntop(int32_t af, const void *src, char *dst, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_SOCKET_API_H */
