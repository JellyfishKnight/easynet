#pragma once

#include "connection.hpp"
#include "socket_base.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace net {

class UdpClient: public SocketClient {
public:
    NET_DECLARE_PTRS(UdpClient)

    UdpClient(const std::string& ip, const std::string& service);

    UdpClient(const UdpClient&) = delete;

    UdpClient(UdpClient&&) = default;

    UdpClient& operator=(const UdpClient&) = delete;

    UdpClient& operator=(UdpClient&&) = default;

    ~UdpClient();

    std::optional<std::string> read(std::vector<uint8_t>& data) override;

    std::optional<std::string> write(const std::vector<uint8_t>& data) override;

    std::optional<std::string> close() override;

    std::shared_ptr<UdpClient> get_shared();

protected:
    [[deprecated("Udp doesn't need connection, this function will cause no effect"
    )]] std::optional<std::string>
    connect() override;
};

/**
 * @brief UdpServer class
 * @note Connection in udp server is representing a client which has sent a message to the server,
 *       the server will send the message back to the client, you can also construct a connection 
 *       object with the client's ip and service to send message to the client you want
 */
class UdpServer: public SocketServer {
public:
    NET_DECLARE_PTRS(UdpServer)

    UdpServer(const std::string& ip, const std::string& service);

    UdpServer(const UdpServer&) = delete;

    UdpServer(UdpServer&&) = default;

    UdpServer& operator=(const UdpServer&) = delete;

    UdpServer& operator=(UdpServer&&) = default;

    ~UdpServer();

    std::optional<std::string> close() override;

    std::shared_ptr<UdpServer> get_shared();

    /**
     * @brief read message from client
     * @param data the buffer to store the message
     * @param conn the connection object representing the client
     * @return std::optional<std::string> error message if any
     * @note if the 2nd param is null, it's broadcasting to every client saved on this server
     */
    std::optional<std::string> read(std::vector<uint8_t>& data, Connection::SharedPtr conn);

    std::optional<std::string> write(const std::vector<uint8_t>& data, Connection::SharedPtr conn);

    void on_message(
        std::function<
            void(std::vector<uint8_t>&, std::vector<uint8_t>&, Connection::ConstSharedPtr)> handler
    );

protected:
    [[deprecated("Udp doesn't need connection, this function will cause no effect")]] void
    on_accept(std::function<void(Connection::ConstSharedPtr conn)> handler) override;

    [[deprecated("Udp doesn't need connection, this function will cause no effect")]] void
    handle_connection(Connection::SharedPtr conn) override;

    [[deprecated("Udp doesn't need connection, this function will cause no effect"
    )]] std::optional<std::string>
    listen() override;

    [[deprecated("Udp doesn't need connection, this function will cause no effect"
    )]] std::optional<std::string>
    start() override;

    [[deprecated("Udp doesn't need connection, this function will cause no effect"
    )]] std::optional<std::string>
    read(std::vector<uint8_t>& data, Connection::ConstSharedPtr conn) override;

    [[deprecated("Udp doesn't need connection, this function will cause no effect"
    )]] std::optional<std::string>
    write(const std::vector<uint8_t>& data, Connection::ConstSharedPtr conn) override;

    std::function<void(std::vector<uint8_t>&, std::vector<uint8_t>&, Connection::ConstSharedPtr)>
        m_message_handler;
};

}; // namespace net