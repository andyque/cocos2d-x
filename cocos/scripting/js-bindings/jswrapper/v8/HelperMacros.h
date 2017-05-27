#pragma once

#include "../config.hpp"

#ifdef SCRIPT_ENGINE_V8

#ifdef __GNUC__
#define SE_UNUSED __attribute__ ((unused))
#else
#define SE_UNUSED
#endif

#define SAFE_ADD_REF(obj) if (obj != nullptr) obj->addRef()

#define SAFE_RELEASE(obj) if (obj != nullptr) obj->release()

#define STRINGIFY(X) #X
// --- Get Property

#define SE_SET_RVAL(data)                           \
    se::internal::setReturnValue(data, _v8args);

#define SE_FINALIZE_FUNC(resigterFunc) \
    void resigterFunc##Registry(void* nativeThisObject) \
    {                                                   \
     se::State state(nativeThisObject);                 \
     resigterFunc(state);                               \
    }

#define SE_CTOR_EX(jsname, registerFunc, modulename)       \
    SE_CTOR(registerFunc, __jsb_##modulename##_class, _SE(modulename##_finalized)) \
    __class__->defineFunction(jsname, _SE(registerFunc));

#define SE_CTOR2(resigterFunc, clasPointer, finalizeCb)                 \
    void resigterFunc##Registry(const v8::FunctionCallbackInfo<v8::Value>& _v8args) { \
                                                                        \
        v8::Isolate* _isolate = _v8args.GetIsolate();                   \
        v8::HandleScope _hs(_isolate);                                  \
        SE_UNUSED bool ret = true;                                      \
        se::ValueArray args;                                            \
        se::internal::jsToSeArgs(_v8args, &args);                       \
        se::Object* thisObject = se::Object::_createJSObject(clasPointer, _v8args.This(), false); \
        thisObject->_setFinalizeCallback(finalizeCb);                   \
        se::State state(thisObject, args);                              \
        resigterFunc(state);                                            \
        se::Value _property;                                            \
        bool _found = false;                                            \
        _found = thisObject->getProperty("_ctor", &_property);          \
        if (_found) _property.toObject()->call(args, thisObject);       \
        for (auto& v : args)                                            \
        {                                                               \
            if (v.isObject() && v.toObject()->isRooted())               \
            {                                                           \
                v.toObject()->switchToUnrooted();                       \
            }                                                           \
        }                                                               \
    };


#define SE_CTOR(resigterFunc, clasPointer, finalizeCb)        \
    auto resigterFunc##Registry = [](const v8::FunctionCallbackInfo<v8::Value>& _v8args) -> void { \
                                                                       \
        v8::Isolate* _isolate = _v8args.GetIsolate();                   \
        v8::HandleScope _hs(_isolate);                                  \
        SE_UNUSED bool ret = true;                                      \
        se::ValueArray args;                                            \
        se::internal::jsToSeArgs(_v8args, &args);                       \
        se::Object* thisObject = se::Object::_createJSObject(clasPointer, _v8args.This(), false); \
        thisObject->_setFinalizeCallback(finalizeCb);                   \
        se::State state(thisObject, args);                              \
        resigterFunc(state);                                            \
        se::Value _property;                                            \
        bool _found = false;                                            \
        _found = thisObject->getProperty("_ctor", &_property);          \
        if (_found) _property.toObject()->call(args, thisObject);       \
        for (auto& v : args)                                            \
        {                                                               \
            if (v.isObject() && v.toObject()->isRooted())               \
            {                                                           \
                v.toObject()->switchToUnrooted();                       \
            }                                                           \
        }                                                               \
        };


#define SE_GET_PROPERTY(funcName) \
    auto funcName##Registry = [](v8::Local<v8::String> _property, const v8::PropertyCallbackInfo<v8::Value>& _v8args) ->void { \
                                                                       \
     v8::Isolate* _isolate = _v8args.GetIsolate();                      \
     v8::HandleScope _hs(_isolate);                                     \
     SE_UNUSED bool ret = true;                                         \
     void* nativeThisObject = se::internal::getPrivate(_isolate, _v8args.This()); \
     se::State state(nativeThisObject);                                 \
     funcName(state);                                                   \
     if (!state.rval().isUndefined())                                   \
         SE_SET_RVAL(state.rval());                                     \
     };


// --- Set Property
#define SE_SET_PROPERTY(funcName) \
    auto funcName##Registry = [](v8::Local<v8::String> _property, v8::Local<v8::Value> _value, const v8::PropertyCallbackInfo<void>& _v8args) ->void { \
                                                                       \
     v8::Isolate* _isolate = _v8args.GetIsolate();                      \
     v8::HandleScope _hs(_isolate);                                     \
     SE_UNUSED bool ret = true;                                         \
     void* nativeThisObject = se::internal::getPrivate(_isolate, _v8args.This()); \
     se::State state(nativeThisObject);                                 \
     se::Value data;                                                    \
     se::internal::jsToSeValue(_isolate, _value, &data);                \
     state.setRetVal(data);                                             \
     funcName(state);                                                   \
     };

// FIXME: expose nativeThisObjectect for all macros

#define SE_DECLARE_FUNC(funcName) \
    void funcName(const v8::FunctionCallbackInfo<v8::Value>& v8args)

#define SE_DECLARE_MODULE(modulename)   \
    extern se::Object* __jsb_##modulename##_proto; \
    bool jsb_register_##modulename();

#define SE_IMPLEMENT_MODULE(modulename) \
    se::Object* __jsb_##modulename##_proto = nullptr;                                          \
    se::Class* __jsb_##modulename##_class = nullptr;                                           \
    void modulename##_finalized(se::State& state)                                              \
    {                                                                                          \
        auto nativeThisObject = state.nativeThisObject();                                      \
        if (nativeThisObject)                                                                  \
        {                                                                                      \
            modulename* thiz = (modulename*) nativeThisObject;                                 \
            printf("STRINGIFY(modulename)_finalized %p ...                                              \n", thiz->getUserData()); \
            SAFE_RELEASE(thiz);                                                                \
        }                                                                                      \
    }                                                                                          \
    SE_FINALIZE_FUNC(modulename##_finalized)                                                   \
    \
    bool modulename##_constructor(se::State& state)                                            \
    {                                                                                          \
        printf("STRINGIFY(modulename)_constructor ...                                                   \n");                      \
        modulename* obj = new modulename();                                                    \
        state.thisObject()->setPrivateData(obj);                                               \
        return true;                                                                           \
    }                                                                                          \
    SE_CTOR2(modulename##_constructor, __jsb_##modulename##_class, _SE(modulename##_finalized)) \

#define _SE(name) \
    name##Registry

#define SE_FUNC_BODY(registerFunc) \
    auto registerFunc##Registry = [](const v8::FunctionCallbackInfo<v8::Value>& _v8args) -> void{ \
        v8::Isolate* _isolate = _v8args.GetIsolate();                   \
        v8::HandleScope _hs(_isolate);                                  \
        SE_UNUSED bool ret = true;                                      \
        SE_UNUSED unsigned argc = (unsigned)_v8args.Length();           \
        se::ValueArray args;                                            \
        se::internal::jsToSeArgs(_v8args, &args);                       \
        void* nativeThisObject = se::internal::getPrivate(_isolate, _v8args.This()); \
        se::State state(nativeThisObject, args, argc);                  \
        registerFunc(state);                                            \
        if (!state.rval().isUndefined())                                \
            SE_SET_RVAL(state.rval());                                  \
        for (auto& v : args)                                            \
        {                                                               \
            if (v.isObject() && v.toObject()->isRooted())               \
            {                                                           \
                v.toObject()->switchToUnrooted();                       \
            }                                                           \
        }                                                               \
    };                                                                  \

#define SE_FUNC(jsname, registerFunc) \
    SE_FUNC_BODY(registerFunc) \
    __class__->defineFunction(jsname, _SE(registerFunc));

#define SE_STATIC_FUNC(jsname, registerFunc)   \
    SE_FUNC_BODY(registerFunc)                              \
    __class__->defineStaticFunction(jsname, _SE(registerFunc));

#define SE_MODULE_REGISTER_BEGIN(modulename) \
    bool jsb_register_##modulename () \
    { \
    auto __class__ = se::Class::create(STRINGIFY(modulename), __ccObj, nullptr, _SE(modulename##_constructor));

#define SE_MODULE_REGISTER_END(modulename)              \
    __class__->defineFinalizedFunction(_SE(modulename##_finalized));  \
    __class__->install();                               \
    __jsb_##modulename##_proto = __class__->getProto(); \
    __jsb_##modulename##_class = __class__;             \
    return true;   \
    }

#define SE_PROPERTY(name, getFunc, setFunc) \
    SE_GET_PROPERTY(getFunc) \
    SE_SET_PROPERTY(setFunc) \
    __class__->defineProperty(name, _SE(getFunc), _SE(setFunc));

#define SE_FUNCTION(registerFunc)                                       \
    void registerFunc##Registry(const v8::FunctionCallbackInfo<v8::Value>& _v8args) \
    {                                                                   \
        v8::Isolate* _isolate = _v8args.GetIsolate();                   \
        v8::HandleScope _hs(_isolate);                                  \
        SE_UNUSED bool ret = true;                                      \
        SE_UNUSED unsigned argc = (unsigned)_v8args.Length();           \
        se::ValueArray args;                                            \
        se::internal::jsToSeArgs(_v8args, &args);                       \
        void* nativeThisObject = se::internal::getPrivate(_isolate, _v8args.This()); \
        se::State state(nativeThisObject, args, argc);                  \
        registerFunc(state);                                            \
        if (!state.rval().isUndefined())                                \
            SE_SET_RVAL(state.rval());                                  \
        for (auto& v : args)                                            \
        {                                                               \
            if (v.isObject() && v.toObject()->isRooted())               \
            {                                                           \
                v.toObject()->switchToUnrooted();                       \
            }                                                           \
        }                                                               \
    }




#define SE_FUNC_BEGIN(funcName, needThisObject)                         \
    void funcName(const v8::FunctionCallbackInfo<v8::Value>& _v8args)   \
    {                                                                   \
    v8::Isolate* _isolate = _v8args.GetIsolate();                       \
    v8::HandleScope _hs(_isolate);                                      \
    SE_UNUSED bool ret = true;                                          \
    SE_UNUSED unsigned argc = (unsigned)_v8args.Length();               \
    se::ValueArray args;                                                \
    se::internal::jsToSeArgs(_v8args, &args);                           \
    se::Object* thisObject = nullptr;                                   \
    void* nativeThisObject = se::internal::getPrivate(_isolate, _v8args.This()); \
    if (nativeThisObject != nullptr && needThisObject)                  \
    {                                                                   \
     thisObject = se::Object::getObjectWithPtr(nativeThisObject);       \
    }                                                                   \

#define SE_FUNC_END                                     \
    for (auto& v : args)                                \
    {                                                   \
        if (v.isObject() && v.toObject()->isRooted())   \
        {                                               \
            v.toObject()->switchToUnrooted();           \
        }                                               \
    }                                                   \
    SAFE_RELEASE(thisObject);                           \
    }

#define SE_FINALIZE_FUNC_BEGIN(funcName)        \
    void funcName(void* nativeThisObject)       \
    {                                           \

#define SE_FINALIZE_FUNC_END                    \
    }

#define SE_DECLARE_FINALIZE_FUNC(funcName)      \
    void funcName(void* nativeThisObject);

// v8 doesn't need to create a new JSObject in SE_CTOR_BEGIN while SpiderMonkey needs.
#define SE_CTOR_BEGIN(funcName, clsName, finalizeCb)                    \
    void funcName(const v8::FunctionCallbackInfo<v8::Value>& _v8args)   \
    {                                                                   \
        v8::Isolate* _isolate = _v8args.GetIsolate();                   \
        v8::HandleScope _hs(_isolate);                                  \
        SE_UNUSED bool ret = true;                                      \
        se::ValueArray args;                                            \
        se::internal::jsToSeArgs(_v8args, &args);                       \
        se::Object* thisObject = se::Object::_createJSObject(__jsb_##clsName##_class, _v8args.This(), false); \
        thisObject->_setFinalizeCallback(finalizeCb);


#define SE_CTOR_END                                             \
    se::Value _property;                                        \
    bool _found = false;                                        \
    _found = thisObject->getProperty("_ctor", &_property);      \
    if (_found) _property.toObject()->call(args, thisObject);   \
    for (auto& v : args)                                        \
    {                                                           \
        if (v.isObject() && v.toObject()->isRooted())           \
        {                                                       \
            v.toObject()->switchToUnrooted();                   \
        }                                                       \
    }                                                           \
    }

#define SE_CTOR2_BEGIN SE_CTOR_BEGIN
#define SE_CTOR2_END SE_CTOR_END

// --- Get Property

#define SE_GET_PROPERTY_BEGIN(funcName, needThisObject)                 \
    void funcName(v8::Local<v8::String> _property, const v8::PropertyCallbackInfo<v8::Value>& _v8args) \
    {                                                                   \
        v8::Isolate* _isolate = _v8args.GetIsolate();                   \
        v8::HandleScope _hs(_isolate);                                  \
        SE_UNUSED bool ret = true;                                      \
        se::Object* thisObject = nullptr;                               \
        void* nativeThisObject = se::internal::getPrivate(_isolate, _v8args.This()); \
        if (nativeThisObject != nullptr && needThisObject)              \
        {                                                               \
            thisObject = se::Object::getObjectWithPtr(nativeThisObject); \
        }                                                               \

#define SE_GET_PROPERTY_END                     \
    SAFE_RELEASE(thisObject);                   \
    }

#define SE_SET_RVAL(data)                           \
    se::internal::setReturnValue(data, _v8args);

// --- Set Property

#define SE_SET_PROPERTY_BEGIN(funcName, needThisObject)                 \
    void funcName(v8::Local<v8::String> _property, v8::Local<v8::Value> _value, const v8::PropertyCallbackInfo<void>& _v8args) \
    {                                                                   \
        v8::Isolate* _isolate = _v8args.GetIsolate();                   \
        v8::HandleScope _hs(_isolate);                                  \
        SE_UNUSED bool ret = true;                                      \
        se::Object* thisObject = nullptr;                               \
        void* nativeThisObject = se::internal::getPrivate(_isolate, _v8args.This()); \
        if (nativeThisObject != nullptr && needThisObject)              \
        {                                                               \
            thisObject = se::Object::getObjectWithPtr(nativeThisObject); \
        }                                                               \
        se::Value data;                                                 \
        se::internal::jsToSeValue(_isolate, _value, &data);

#define SE_SET_PROPERTY_END                     \
    SAFE_RELEASE(thisObject);                   \
    }

#endif // #ifdef SCRIPT_ENGINE_V8
