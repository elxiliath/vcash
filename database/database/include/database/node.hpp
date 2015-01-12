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

#ifndef DATABASE_NODE_HPP
#define DATABASE_NODE_HPP

#include <cstdint>
#include <list>
#include <memory>

#include <boost/asio.hpp>

#include <database/stack.hpp>

namespace database {

    class node_impl;
    class stack_impl;
    
    /**
     * The node.
     */
    class node
    {
        public:
        
            /**
             * Constructor
             * @param ios The boost::asio::io_service.
             * @param owner The stack_impl.
             */
            node(boost::asio::io_service &, stack_impl &);
            
            /**
             * Starts the node.
             * @param config The stack::configuration.
             */
            void start(const stack::configuration &);
            
            /**
             * Stops the node.
             */
            void stop();
        
            /**
             * Queues a ping for the given endpoint.
             * @param ep The boost::asio::ip::udp::endpoint.
             */
            void queue_ping(const boost::asio::ip::udp::endpoint &);
        
            /**
             * Performs a store operation.
             * @param query The query.
             */
            std::uint16_t store(const std::string &);
        
            /**
             * Performs a lookup on the query.
             * @param query The query.
             * @param max_results The maximum number of results.
             */
            std::uint16_t find(const std::string &, const std::size_t &);
        
            /**
             * Performs a (tcp) proxy operation given endpoint and buffer.
             * @param addr The address.
             * @param port The port.
             * @param buf The buffer.
             * @param len The length.
             */
            std::uint16_t proxy(
                const char * addr, const std::uint16_t & port,
                const char * buf, const std::size_t & len
            );
        
            /**
             * Returns all of the endpoints in the routing table.
             */
            std::list< std::pair<std::string, std::uint16_t> > endpoints();
        
            /**
             * Called when connected to the network.
             * @param ep The boost::asio::ip::tcp::endpoint.
             */
            void on_connected(const boost::asio::ip::tcp::endpoint & ep);
        
            /**
             * Called when disconnected from the network.
             * @param ep The boost::asio::ip::tcp::endpoint.
             */
            void on_disconnected(const boost::asio::ip::tcp::endpoint & ep);
        
            /**
             * Called when a search result is received.
             * @param transaction_id The transaction id.
             * @param query_string The query string.
             */
            void on_find(
                const std::uint16_t & transaction_id,
                const std::string & query_string
            );
        
            /**
             * Called when a proxy (response) is received.
             * @param tid The transaction identifier.
             * @param ep The boost::asio::ip::tcp::endpoint.
             * @param value The value.
             */
            void on_proxy(
                const std::uint16_t & tid,
                const boost::asio::ip::tcp::endpoint & ep,
                const std::string & value
            );
        
            /**
             * Called when a udp packet doesn't match the protocol fingerprint.
             * @param addr The address.
             * @param port The port.
             * @param buf The buffer.
             * @param len The length.
             */
            void on_udp_receive(
                const char * addr, const std::uint16_t & port, const char * buf,
                const std::size_t & len
            );
        
            /**
             * Sets the bootstrap contacts.
             * @param val The value.
             */
            void set_bootstrap_contacts(
                const std::list<boost::asio::ip::udp::endpoint> &
            );
        
            /**
             * The bootstrap contacts.
             */
            std::list<boost::asio::ip::udp::endpoint> & bootstrap_contacts();
            
            /**
             * The id.
             */
            const std::string & id() const;
            
        private:
        
            // ...
            
        protected:
        
            /**
             * The stack_impl.
             */
            stack_impl & stack_impl_;
        
            /**
             * The node implementation.
             */
            std::shared_ptr<node_impl> node_impl_;
    };
    
} // namespace database

#endif // DATABASE_NODE_HPP
