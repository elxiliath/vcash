/*
 * Copyright (c) 2008-2014 John Connor (BM-NC49AxAjcqVcF5jNPu85Rb8MJ2d9JqZt)
 *
 * This file is part of coinpp.
 *
 * coinpp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#if (!defined __arm__ && !defined __thumb__ && \
    !defined _M_ARM && !defined _M_ARMT)
    
#include <chrono>
#include <thread>

#include <database/logger.hpp>
#include <database/network.hpp>
#include <database/upnp_client.hpp>

#if (defined USE_UPNP && USE_UPNP)
#include <miniupnpc/upnpcommands.h>
#endif // USE_UPNP

using namespace database;

upnp_client::upnp_client(boost::asio::io_service & ios)
    : io_service_(ios)
    , strand_(ios)
    , timer_(ios)
#if (defined USE_UPNP && USE_UPNP)
    , upnp_urls_()
    , igd_data_()
    , discovery_did_succeed_(false)
#endif // USE_UPNP
{
    // ...
}

void upnp_client::start()
{
#if (defined USE_UPNP && USE_UPNP)
    do_start();
#endif // USE_UPNP
}
        
void upnp_client::stop()
{
    io_service_.post(
        strand_.wrap(std::bind(&upnp_client::do_stop, shared_from_this()))
    );
}

void upnp_client::add_mapping(
    const protocol_t & protocol, const std::uint16_t & port
    )
{
    if (discovery_did_succeed_)
    {
        io_service_.post(
            strand_.wrap(std::bind(&upnp_client::do_add_mapping,
            shared_from_this(), protocol, port))
        );
    }
}

void upnp_client::remove_mapping(
    const protocol_t & protocol, const std::uint16_t & port
    )
{
    if (discovery_did_succeed_)
    {
        io_service_.post(
            strand_.wrap(std::bind(&upnp_client::do_remove_mapping,
            shared_from_this(), protocol, port))
        );
    }
}

void upnp_client::do_start()
{
#if (defined USE_UPNP && USE_UPNP)
    /**
     * Start discovering devices.
     */
    discover_thread_ = std::thread(&upnp_client::do_discover_devices, this);
#endif // USE_UPNP
}
        
void upnp_client::do_stop()
{
#if (defined USE_UPNP && USE_UPNP)
    timer_.cancel();
    
    for (auto & i : mappings_)
    {
        do_remove_mapping(i.protocol, i.external_port);
    }
    
    FreeUPNPUrls(&upnp_urls_);
#endif // USE_UPNP
}

void upnp_client::do_add_mapping(
    const protocol_t & protocol, const std::uint16_t & port
    )
{
#if (defined USE_UPNP && USE_UPNP)
    std::string proto = protocol == protocol_udp ? "UDP" : "TCP";
    std::string hostname = boost::asio::ip::host_name();
    
    boost::asio::ip::address local_address = network::local_address();
    
    /**
     * Add a mapping for three hours re-mapping every one hour.
     */
    int ret = UPNP_AddPortMapping(
        upnp_urls_.controlURL, igd_data_.first.servicetype,
        std::to_string(port).c_str(), std::to_string(port).c_str(),
        local_address.to_string().c_str(), hostname.c_str() /* desc */,
        proto.c_str(), 0, "10800"
    );
    
    if (ret == UPNPCOMMAND_SUCCESS)
    {
        bool found = false;
        
        for (auto & i : mappings_)
        {
            if (
                i.protocol == protocol && i.internal_port == port &&
                i.external_port == port
                )
            {
                found = true;
                
                break;
            }
        }
        
        if (!found)
        {
            mapping_t m;
            
            m.protocol = protocol;
            m.internal_port = port;
            m.external_port = port;
            m.time = std::time(0);
            
            mappings_.push_back(m);
        }
    }
    else
    {
        log_debug("UPnP client add mapping failed " << ret << ".");
    }
#endif // USE_UPNP
}

void upnp_client::do_remove_mapping(
    const protocol_t & protocol, const std::uint16_t & port
    )
{
#if (defined USE_UPNP && USE_UPNP)
    std::string proto = protocol == protocol_udp ? "UDP" : "TCP";
    
    int ret = UPNP_DeletePortMapping(
        upnp_urls_.controlURL, igd_data_.first.servicetype,
        std::to_string(port).c_str(), proto.c_str(), 0
    );
 
    if (ret == UPNPCOMMAND_SUCCESS)
    {
        for (auto it = mappings_.begin(); it != mappings_.end(); ++it)
        {
            if (it->protocol == protocol && it->external_port == port)
            {
                mappings_.erase(it);
                
                break;
            }
        }
    }
    else
    {
        log_debug("UPnP client remove mapping failed " << ret << ".");
    }
#endif // USE_UPNP
}

void upnp_client::do_discover_devices()
{
#if (defined USE_UPNP && USE_UPNP)
    enum { search_time = 2 };
    
    int err;
    
    UPNPDev * devlist = upnpDiscover(search_time * 1000, 0, 0, 0, 0, &err);

    if (err == UPNPDISCOVER_SUCCESS)
    {
        int ret = UPNP_GetValidIGD(devlist, &upnp_urls_, &igd_data_, 0, 0);
        
        freeUPNPDevlist(devlist);

        if (ret == 1)
        {
            discovery_did_succeed_ = true;
            
            log_debug(
                "UPnP client discovered " << upnp_urls_.controlURL <<
                " services."
            );
            
            /**
             * Start the timer.
             */
            auto timeout = std::chrono::seconds(1800);
            
            timer_.expires_from_now(timeout);
            timer_.async_wait(
                strand_.wrap(std::bind(&upnp_client::tick, shared_from_this(),
                std::placeholders::_1))
            );
        }
        else
        {
            log_debug("UPnP discovery failed, ret = " << ret << ".");
            
            do_stop();
        }
    }
    else
    {
        log_debug("UPnP discovery failed.");
        
        do_stop();
    }
    
    discover_thread_.detach();
#endif // USE_UPNP
}

void upnp_client::tick(const boost::system::error_code & ec)
{
    if (ec)
    {
        // ...
    }
    else
    {
        /**
         * Re-add mappings every hour.
         */
        for (auto & i : mappings_)
        {
            if ((std::time(0) - i.time) > 3600)
            {
                i.time = std::time(0);
                
                do_add_mapping(i.protocol, i.external_port);
            }
        }
    
        /**
         * Start the timer.
         */
        auto timeout = std::chrono::seconds(1800);
        
        timer_.expires_from_now(timeout);
        timer_.async_wait(
            strand_.wrap(std::bind(&upnp_client::tick, shared_from_this(),
            std::placeholders::_1))
        );
    }
}

#include <iostream>

int upnp_client::run_test()
{
    struct io_service_s
    {
        bool should_run;
        boost::asio::io_service ios;
        void run() { while (should_run) { ios.run(); ios.reset(); } }
    } io_service;
    
    std::shared_ptr<upnp_client> c(new upnp_client(io_service.ios));
    
    io_service.should_run = true;
    
    std::thread t(std::bind(&io_service_s::run, &io_service));
    
    c->start();
    
    c->add_mapping(protocol_udp, 9999);
    c->add_mapping(protocol_tcp, 9999);
    
    std::cin.get();
    
    c->stop();
    
    std::cin.get();

    io_service.should_run = false;
    
    t.join();
    
    return 0;
}

#endif
