//
// Created by Александр Волков on 14.01.19.
//

#ifndef ASIO_SERVER_LAB_SERVER_H
#define ASIO_SERVER_LAB_SERVER_H


#include <iostream>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <boost/aligned_storage.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <mutex>
#include <vector>

class talk_to_client;
 boost::asio::io_service service;
 std::vector<std::shared_ptr<talk_to_client>> clients;
 std::recursive_mutex mutex;


class talk_to_client {
private:

    boost::asio::ip::tcp::socket sock_;
    enum { max_msg = 1024 };
    char buff_[1024];
    bool clients_changed_;
    std::string username_;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_ping;
public:

    talk_to_client(): sock_(service){}

    std::string username() {
        return username_;
    }

    boost::asio::ip::tcp::socket& sock(){
        return sock_;
    }

    bool timed_out()  {
        auto now = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_ping);
        return ms.count() > 50000;
    }

    void stop(){
        boost::system::error_code err;
        sock_.close(err);
    }

    void read_request(){
        if ( sock_.available())
            sock_.read_some(boost::asio::buffer(buff_, max_msg));
    }


    void process_request(){
        unsigned enter = 0;
        for (unsigned i = 0; i < max_msg; i++) {
            if (buff_[i] == '\n') enter = i;
        }
        if (enter == 0) return;
        last_ping = std::chrono::high_resolution_clock::now();
        std::string msg ="";
        for (unsigned i = 0; i < enter; i++) {
            msg+=buff_[i];
        }
        for (unsigned i = 0; i < max_msg; i++) {
            buff_[i] = ' ';
        }
        if ( msg.find("login") == 0) on_login(msg);
        else if ( msg.find("ping") == 0) on_ping();
        else if ( msg.find("ask_clients") == 0) on_clients();
        else
            std::cerr << "invalid msg " << msg << std::endl;
    }


    void on_login(const std::string & msg){
        username_ = "";
        for (unsigned i = 6; i < msg.size(); i++) {
            if ((msg[i] != ' ') && (msg[i] != ':') && (msg[i] != '='))
                username_+=msg[i];
        }
        write("login ok\n");
        set_clients_changed();
    }

    void on_ping() {
        write(clients_changed_ ? "ping client_list_changed\n" : "ping ok\n");
        clients_changed_ = false;
    }

    void on_clients(){
        std::string msg;
        std::lock_guard<std::recursive_mutex> lock(mutex);
        for (auto& client : clients)
            msg += client->username() + " ";
        write("clients " + msg + "\n");
        clients_changed_ = false;
    }

    void write(const std::string & msg){
        sock_.write_some(boost::asio::buffer(msg));
    }

    void answer_to_client() {
        {
            try {
                read_request();
                process_request();
            }
            catch (boost::system::system_error &) {
                stop();
            }
            if (timed_out())
                stop();
        }
    }

    void set_clients_changed(){
        clients_changed_ = true;
    }

};

#endif //ASIO_SERVER_LAB_SERVER_H
