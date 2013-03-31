
/**
 * This file is part of the boostcache package.
 *
 * (c) Azat Khuzhin <a3at.mail@gmail.com>
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#include "commands.h"

#include <boost/format.hpp>

namespace PlaceHolders = std::placeholders;

Commands::Callback Commands::find(const std::string& commandName)
{
    HashTable::const_iterator command = m_commands.find(commandName);
    if (command == m_commands.end()) {
        return std::bind(&Commands::notImplementedYetCallback,
                         this, PlaceHolders::_1);
    }
    return command->second;
}

Commands::Commands()
{
    // "db" operations
    m_commands["HGET"] =           std::bind(&Db::HashTable::get,
                                             &m_dbHashTable, PlaceHolders::_1);
    m_commands["HSET"] =           std::bind(&Db::HashTable::set,
                                             &m_dbHashTable, PlaceHolders::_1);
    m_commands["HDEL"] =           std::bind(&Db::HashTable::del,
                                             &m_dbHashTable, PlaceHolders::_1);
}

std::string Commands::notImplementedYetCallback(const Command::Arguments& arguments)
{
    return str(boost::format("-ERR %s is not implemented\r\n") % arguments[0]);
}