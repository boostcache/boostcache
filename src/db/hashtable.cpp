
/**
 * This file is part of the boostcache package.
 *
 * (c) Azat Khuzhin <a3at.mail@gmail.com>
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */


#include "hashtable.h"

#include <v8.h>


namespace Db
{
    HashTable::HashTable()
        : Interface()
    {
    }

    std::string HashTable::get(const CommandHandler::Arguments &arguments)
    {
        // get shared lock
        boost::shared_lock<boost::shared_mutex> lock(m_access);

        Table::const_iterator value = m_table.find(arguments[1]);
        if (value == m_table.end()) {
            return CommandHandler::REPLY_NIL;
        }
        return CommandHandler::toReplyString(value->second);
    }

    std::string HashTable::set(const CommandHandler::Arguments &arguments)
    {
        // get exclusive lock
        boost::upgrade_lock<boost::shared_mutex> lock(m_access);
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);

        m_table[arguments[1] /* key */] = arguments[2] /* value */;

        return CommandHandler::REPLY_OK;
    }

    std::string HashTable::del(const CommandHandler::Arguments &arguments)
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

    std::string HashTable::foreach(const CommandHandler::Arguments &arguments)
    {
        /**
         * Common TODO's for using js vm:
         * - move executor into separate object
         * - more error handling
         * - adopt to the newer version of v8 (need Isolate* in constructor)
         */
        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        v8::Locker locker(isolate);

        v8::HandleScope scope;
        v8::Handle<v8::Context> context = v8::Context::New();
        v8::Context::Scope enterScope(context);
        v8::Handle<v8::String> source = v8::String::New(arguments[1].c_str());
        v8::Handle<v8::Script> script = v8::Script::Compile(source);
        v8::Handle<v8::Value> sourceResult = script->Run();
        v8::Handle<v8::Function> function = v8::Handle<v8::Function>::Cast(sourceResult);

        for (std::pair<const Key, Value> &i : m_table) {
            std::string key(i.first);
            std::string &value = i.second;

            v8::Local<v8::Value> args[] = {
                v8::String::New(key.c_str()),
                v8::String::New(value.c_str())
            };
            function->Call(context->Global(), 2, args);
        }

        return CommandHandler::REPLY_TRUE;
    }
}
