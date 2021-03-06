
/**
 * This file is part of the boostcache package.
 *
 * (c) Azat Khuzhin <a3at.mail@gmail.com>
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#include "jsvm.h"
#include "util/log.h"
#include "kernel/exception.h"
#include "config.h" /** HAVE_* */


/**
 * XXX: get rid of support for old v8
 */

namespace Js
{
#ifdef HAVE_V8_FUNCTIONCALLBACKINFO
    typedef ::v8::FunctionCallbackInfo< ::v8::Value > Args;

    std::string logArgs2String(const Args &args)
    {
        std::string message;
        for (int i = 0; i < args.Length(); i++) {
            ::v8::HandleScope scope(args.GetIsolate());
            ::v8::String::Utf8Value str(args[i]);
            message += *str;
        }
        return message;
    }
    void log(const Args &args)
    {
        LOG(info) << logArgs2String(args);
    }
    void error(const Args &args)
    {
        LOG(error) << logArgs2String(args);
    }
#else
    typedef ::v8::Arguments Args;

    std::string logArgs2String(const Args &args)
    {
        std::string message;
        for (int i = 0; i < args.Length(); i++) {
            ::v8::HandleScope scope;
            ::v8::String::Utf8Value str(args[i]);
            message += *str;
        }
        return message;
    }
    ::v8::Handle< ::v8::Value > log(const Args &args)
    {
        LOG(info) << logArgs2String(args);
        return v8::Handle<v8::Value>();
    }
    ::v8::Handle< ::v8::Value > error(const Args &args)
    {
        LOG(error) << logArgs2String(args);
        return v8::Handle<v8::Value>();
    }
#endif
}

namespace {
#ifdef HAVE_V8_WITH_MOST_CONSTRUCTORS_ISOLATE
    v8::Local<v8::String> newUtf8String(const char *data)
    {
        return v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), data);
    }
    v8::Local<v8::FunctionTemplate> newFunctionTemplate(v8::FunctionCallback callback = NULL)
    {
        return v8::FunctionTemplate::New(v8::Isolate::GetCurrent(), callback);
    }
    v8::Local<v8::Context> newContext(v8::ExtensionConfiguration* extensions = NULL,
                                      v8::Handle<v8::ObjectTemplate> global_template = v8::Handle<v8::ObjectTemplate>())
    {
        return v8::Context::New(v8::Isolate::GetCurrent(), extensions, global_template);
    }
#else
    v8::Local<v8::String> newUtf8String(const char *data)
    {
        return v8::String::New(data);
    }
    v8::Local<v8::FunctionTemplate> newFunctionTemplate(v8::InvocationCallback callback = NULL)
    {
        return v8::FunctionTemplate::New(callback);
    }
    v8::Persistent<v8::Context> newContext(v8::ExtensionConfiguration* extensions = NULL,
                                      v8::Handle<v8::ObjectTemplate> global_template = v8::Handle<v8::ObjectTemplate>())
    {
        return v8::Context::New(extensions, global_template);
    }
#endif
}

JsVm::JsVm(const std::string &code)
    : m_isolate(v8::Isolate::New())
    , m_isolateScope(m_isolate)
    , m_locker(m_isolate)
#ifdef HAVE_V8_WITH_MOST_CONSTRUCTORS_ISOLATE
    , m_scope(m_isolate)
#endif
    , m_global(v8::ObjectTemplate::New())
{
    v8::Handle<v8::ObjectTemplate> console = v8::ObjectTemplate::New();
    m_global->Set(newUtf8String("console"), console);

    console->Set(newUtf8String("log"),
                 newFunctionTemplate(&Js::log));
    console->Set(newUtf8String("error"),
                 newFunctionTemplate(&Js::error));

    /**
     * We can't do this inside initialization list, since we are modifying
     * "m_global", and during ths gc can update references to it, since we are
     * adding _properties_ to it.
     *
     * And also to avoid Context::Scope for every call(), just enter context
     * here, since this module already provides "box" for executing
     * user-specific code in current thread.
     */
    m_context = newContext(NULL, m_global);
    m_context->Enter();

    m_source = newUtf8String(code.c_str());
}
JsVm::~JsVm()
{
    m_context->Exit();
}

bool JsVm::init()
{
    v8::Handle<v8::Script> script = v8::Script::Compile(m_source);
    if (!*script) {
        fillTryCatch();
        return false;
    }

    v8::Handle<v8::Value> sourceResult = script->Run();
    if (sourceResult.IsEmpty()) {
        fillTryCatch();
        return false;
    }

    m_function = v8::Handle<v8::Function>::Cast(sourceResult);
    return true;
}

std::string JsVm::call(const Db::Interface::Key &key, const Db::Interface::Value &value)
{
    v8::Local<v8::Value> args[] = {
        newUtf8String(key.c_str()),
        newUtf8String(value.c_str())
    };

    v8::Local<v8::Value> ret = m_function->Call(m_context->Global(), 2, args);
    if (ret.IsEmpty()) {
        fillTryCatch();
        throw Exception("Error while calling function");
    }
    v8::String::Utf8Value newValue(ret);
    return std::string(*newValue);
}

void JsVm::fillTryCatch()
{
    v8::Handle<v8::Value> exception = m_trycatch.Exception();
    v8::String::Utf8Value exceptionMessage(exception);
    LOG(error) << *exceptionMessage;
}
