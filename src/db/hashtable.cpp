
/**
 * This file is part of the boostcache package.
 *
 * (c) Azat Khuzhin <a3at.mail@gmail.com>
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */


#include "hashtable.h"


namespace Db
{
    HashTable::HashTable()
        : Interface()
    {
    }

    CommandReply HashTable::get(const CommandHandler::Arguments &arguments)
    {
        // get shared lock
        boost::shared_lock<boost::shared_mutex> lock(m_access);

        Table::const_iterator value = m_table.find(arguments[1]);
        if (value == m_table.end()) {
            return CommandHandler::REPLY_NIL;
        }
        return CommandHandler::toReplyString(value->second);
    }

    CommandReply HashTable::set(const CommandHandler::Arguments &arguments)
    {
        // get exclusive lock
        boost::upgrade_lock<boost::shared_mutex> lock(m_access);
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

        m_table[arguments[1] /* key */] = arguments[2] /* value */;

        return CommandHandler::REPLY_OK;
    }

    CommandReply HashTable::del(const CommandHandler::Arguments &arguments)
    {
        // get exclusive lock
        boost::upgrade_lock<boost::shared_mutex> lock(m_access);
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

        Table::const_iterator value = m_table.find(arguments[1]);
        if (value == m_table.end()) {
            return CommandHandler::REPLY_FALSE;
        }

        m_table.erase(value);
        return CommandHandler::REPLY_TRUE;
    }
}