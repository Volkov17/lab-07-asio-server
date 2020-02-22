#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <chrono>
#include "server.h"
#include <boost/thread/recursive_mutex.hpp>
#include <mutex>

//boost::recursive_mutex mx;

using namespace boost::asio;

extern boost::asio::io_service service;
extern std::vector<std::shared_ptr<talk_to_client>> clients;
extern std::recursive_mutex mutex;


void accept_thread()
{
    boost::asio::ip::tcp::acceptor acceptor(service,
                                            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 8001));
    while (true)
    {
        auto client = std::make_shared<talk_to_client>();
        acceptor.accept(client->sock());
        std::lock_guard<std::recursive_mutex> lock(mutex);
        clients.push_back(client);

    }
}

void handle_clients_thread()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::lock_guard<std::recursive_mutex> lock(mutex);
        for (auto b = clients.begin(), e = clients.end(); b != e; ++b)
            (*b)->answer_to_client();
        clients.erase(std::remove_if(clients.begin(),
                clients.end(), boost::bind(&talk_to_client::timed_out, _1)), clients.end());
    }
}


int main() {
    std::thread thread1(accept_thread);
    std::thread thread2(handle_clients_thread);
    thread1.join();
    thread2.join();
    return 0;
}

