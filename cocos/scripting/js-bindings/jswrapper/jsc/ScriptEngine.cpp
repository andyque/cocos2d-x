#include "ScriptEngine.hpp"

#include "Object.hpp"
#include "Class.hpp"
#include "Utils.hpp"

#ifdef SCRIPT_ENGINE_JSC

extern "C" JS_EXPORT void JSSynchronousGarbageCollectForDebugging(JSContextRef);

namespace se {

    namespace {
        ScriptEngine* __instance = nullptr;

        JSValueRef __forceGC(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                             size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
        {
            printf("GC begin ...\n");
//            JSGarbageCollect(ctx);
            JSSynchronousGarbageCollectForDebugging(ctx);
            printf("GC end ...\n");
            return JSValueMakeUndefined(ctx);
        }

        JSValueRef __log(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                         size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
        {
            if (argumentCount > 0)
            {
                assert(JSValueIsString(ctx, arguments[0]));
                Value v;
                internal::jsToSeValue(ctx, arguments[0], &v);
                printf("%s\n", v.toString().c_str());
            }
            return JSValueMakeUndefined(ctx);
        }
    }

    ScriptEngine *ScriptEngine::getInstance()
    {
        if (__instance == nullptr)
        {
            __instance = new ScriptEngine();
            if (!__instance->init())
            {
                delete __instance;
                __instance = nullptr;
            }
        }

        return __instance;
    }

    void ScriptEngine::destroyInstance()
    {
        delete __instance;
        __instance = nullptr;
    }

    ScriptEngine::ScriptEngine()
            : _cx(nullptr)
            , _globalObj(nullptr)
            , _isValid(false)
    {
    }

    bool ScriptEngine::init()
    {
        printf("Initializing JavaScriptCore \n");

        _cx = JSGlobalContextCreate(nullptr);

        if (nullptr == _cx)
            return false;

        Class::setContext(_cx);
        Object::setContext(_cx);

        JSObjectRef globalObj = JSContextGetGlobalObject(_cx);

        if (nullptr == globalObj)
            return false;

        _globalObj = Object::_createJSObject(nullptr, globalObj, true);

        JSStringRef propertyName = JSStringCreateWithUTF8CString("log");
        JSObjectSetProperty(_cx, globalObj, propertyName, JSObjectMakeFunctionWithCallback(_cx, propertyName, __log), kJSPropertyAttributeReadOnly, nullptr);
        JSStringRelease(propertyName);

        propertyName = JSStringCreateWithUTF8CString("forceGC");
        JSObjectSetProperty(_cx, globalObj, propertyName, JSObjectMakeFunctionWithCallback(_cx, propertyName, __forceGC), kJSPropertyAttributeReadOnly, nullptr);
        JSStringRelease(propertyName);

        _isValid = true;

        return true;
    }

    ScriptEngine::~ScriptEngine()
    {
        cleanup();
    }

    void ScriptEngine::cleanup()
    {
        SAFE_RELEASE(_globalObj);

        Class::cleanup();
        
        JSGlobalContextRelease(_cx);
    }

    std::string ScriptEngine::formatException(JSValueRef exception)
    {
        std::string ret;
        internal::forceConvertJsValueToStdString(_cx, exception, &ret);

        JSType type = JSValueGetType(_cx, exception);

        if (type == kJSTypeObject)
        {
            JSObjectRef obj = JSValueToObject(_cx, exception, nullptr);
            JSPropertyNameArrayRef nameArr = JSObjectCopyPropertyNames(_cx, obj);

            size_t count =JSPropertyNameArrayGetCount(nameArr);
            for (size_t i = 0; i < count; ++i)
            {
                JSStringRef jsName = JSPropertyNameArrayGetNameAtIndex(nameArr, i);
                JSValueRef jsValue = JSObjectGetProperty(_cx, obj, jsName, nullptr);

                std::string name;
                internal::jsStringToStdString(_cx, jsName, &name);
                std::string value;
                internal::forceConvertJsValueToStdString(_cx, jsValue, &value);

                if (name == "line")
                {
                    ret += ", line: " + value;
                }
                else if (name == "sourceURL")
                {
                    ret += ", sourceURL: " + value;
                }
            }

            JSPropertyNameArrayRelease(nameArr);
        }

        return ret;
    }

    Object* ScriptEngine::getGlobalObject()
    {
        return _globalObj;
    }

    void ScriptEngine::gc()
    {
        JSGarbageCollect(_cx);
    }

    bool ScriptEngine::executeScriptBuffer(const char *string, Value *data, const char *fileName)
    {
        return executeScriptBuffer(string, strlen(string), data, fileName);
    }

    bool ScriptEngine::executeScriptBuffer(const char *script, size_t length, Value *data, const char *fileName)
    {
        std::string exceptionStr;
        std::string scriptStr(script, length);

        JSValueRef exception = nullptr;
        JSStringRef jsSourceUrl = JSStringCreateWithUTF8CString(fileName);
        JSStringRef jsScript = JSStringCreateWithUTF8CString(scriptStr.c_str());

        bool ok = JSCheckScriptSyntax(_cx, jsScript, jsSourceUrl, 1, &exception);;
        if (ok)
        {
            JSEvaluateScript(_cx, jsScript, nullptr, jsSourceUrl, 1, &exception);

            if (exception)
            {
                exceptionStr = formatException(exception);
                ok = false;
            }
        }
        else
        {
            if (exception)
            {
                exceptionStr = formatException(exception);
            }
            else
            {
                printf("Unknown syntax error parsing file %s\n", fileName);
            }
        }

        JSStringRelease(jsScript);
        JSStringRelease(jsSourceUrl);

        if (!exceptionStr.empty())
        {
            printf("%s\n", exceptionStr.c_str());
        }

        return ok;
    }

    bool ScriptEngine::executeScriptFile(const std::string &filePath, Value *rval/* = nullptr*/)
    {
        bool ret = false;
        FILE* fp = fopen(filePath.c_str(), "rb");
        if (fp != nullptr)
        {
            fseek(fp, 0, SEEK_END);
            long fileSize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            char* buffer = (char*) malloc(fileSize);
            fread(buffer, fileSize, 1, fp);
            ret = executeScriptBuffer(buffer, fileSize, rval, filePath.c_str());
            free(buffer);
            fclose(fp);
        }

        return ret;
    }

    void ScriptEngine::_retainScriptObject(void* owner, void* target)
    {
        auto iterOwner = __nativePtrToObjectMap.find(owner);
        if (iterOwner == __nativePtrToObjectMap.end())
        {
            return;
        }

        auto iterTarget = __nativePtrToObjectMap.find(target);
        if (iterTarget == __nativePtrToObjectMap.end())
        {
            return;
        }

        iterOwner->second->attachChild(iterTarget->second);
    }

    void ScriptEngine::_releaseScriptObject(void* owner, void* target)
    {
        auto iterOwner = __nativePtrToObjectMap.find(owner);
        if (iterOwner == __nativePtrToObjectMap.end())
        {
            return;
        }

        auto iterTarget = __nativePtrToObjectMap.find(target);
        if (iterTarget == __nativePtrToObjectMap.end())
        {
            return;
        }

        iterOwner->second->detachChild(iterTarget->second);
    }

    void ScriptEngine::_onReceiveNodeEvent(void* node, NodeEventType type)
    {
//        printf("ScriptEngine::_onReceiveNodeEvent, node: %p, type: %d\n", node, (int) type);

        auto iter = __nativePtrToObjectMap.find(node);
        if (iter  == __nativePtrToObjectMap.end())
            return;

        Object* target = iter->second;
        const char* funcName = nullptr;
        if (type == NodeEventType::ENTER)
        {
            funcName = "onEnter";
        }
        else if (type == NodeEventType::EXIT)
        {
            funcName = "onExit";
        }
        else if (type == NodeEventType::ENTER_TRANSITION_DID_FINISH)
        {
            funcName = "onEnterTransitionDidFinish";
        }
        else if (type == NodeEventType::EXIT_TRANSITION_DID_START)
        {
            funcName = "onExitTransitionDidStart";
        }
        else if (type == NodeEventType::CLEANUP)
        {
            funcName = "cleanup";
        }
        else
        {
            assert(false);
        }

        Value funcVal;
        bool ok = target->getProperty(funcName, &funcVal);
        if (ok && !funcVal.toObject()->_isNativeFunction())
        {
            funcVal.toObject()->call(EmptyValueArray, target);
        }
    }

} // namespace se {

#endif // SCRIPT_ENGINE_JSC
