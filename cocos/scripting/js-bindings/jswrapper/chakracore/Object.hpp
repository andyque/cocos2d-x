#pragma once

#include "../config.hpp"

#ifdef SCRIPT_ENGINE_CHAKRACORE

#include "Base.h"
#include "../Value.hpp"
#include "../Ref.hpp"

namespace se {

    class Class;

    class Object : public Ref
    {
    private:
        Object();
    public:
        virtual ~Object();

        static Object* createPlainObject(bool rooted);

        static Object* createObject(const char* clsName, bool rooted);
        static Object* getObjectWithPtr(void* ptr);
        static Object* getOrCreateObjectWithPtr(void* ptr, const char* clsName, bool rooted);

        static Object* _createJSObject(Class* cls, JsValueRef obj, bool rooted);

        bool init(JsValueRef obj, bool rooted);

        // --- Getter/Setter
        bool getProperty(const char* name, Value* data);
        void setProperty(const char* name, const Value& v);

        JsValueRef _getJSObject() const;
        Class* _getClass() const;
        void _cleanup(void* nativeObject = nullptr);
        void _setFinalizeCallback(JsFinalizeCallback finalizeCb);

        // --- Function
        bool isFunction() const;
        bool _isNativeFunction() const;
        bool call(const ValueArray& args, Object* thisObject, Value* rval = nullptr);

        bool defineFunction(const char *funcName, JsNativeFunction func);

        // --- TypedArrays
        bool isTypedArray() const;
        void getAsUint8Array(unsigned char **ptr, unsigned int *length);
        void getAsUint16Array(unsigned short **ptr, unsigned int *length);
        void getAsUint32Array(unsigned int **ptr, unsigned int *length);
        void getAsFloat32Array(float **ptr, unsigned int *length);

        // --- Arrays
        bool isArray() const;
        void getArrayLength(unsigned int *length);
        void getArrayElement(unsigned int index, Value *data);

        // --- Private
        void setPrivateData(void* data);
        void* getPrivateData();


        void switchToRooted();
        void switchToUnrooted();
        bool isRooted() const;

        bool isSame(Object* o) const;
        bool attachChild(Object* child);
        bool detachChild(Object* child);

    private:
        void debug(const char *what);

        Class* _cls;
        JsValueRef _obj;
        bool _isRooted;  /* wrapper is in rooted mode */
        bool _hasPrivateData;
        bool _isCleanup;
        JsFinalizeCallback _finalizeCb;

        friend class ScriptEngine;
    };


    extern std::unordered_map<void* /*native*/, Object* /*jsobj*/> __nativePtrToObjectMap;

} // namespace se {

#endif // SCRIPT_ENGINE_CHAKRACORE

