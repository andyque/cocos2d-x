//
//  State.cpp
//  cocos2d_js_bindings
//
//  Created by guanghui on 5/27/17.
//
//

#include "State.hpp"
#include "Object.hpp"

namespace se {
    State::State()
    :_nativeThisObject(nullptr),
    _thisObject(nullptr),
    _args(nullptr),
    _argc(0)
    {
        
    }
    
    State::~State()
    {
        
    }
    
    State::State(void* nativeThisObject)
    :_nativeThisObject(nativeThisObject)
    {
        this->_thisObject = nullptr;
        this->_args = nullptr;
        this->_argc = 0;
    }
    
    State::State(void* nativeThisObject,
                 const ValueArray& args,
                 uint16_t argc)
    :_nativeThisObject(nativeThisObject),
    _args(&args),
    _argc(argc)
    {
        this->_thisObject = nullptr;
    }
    
    void* State::nativeThisObject() const
    {
        return this->_nativeThisObject;
    }
    const ValueArray& State::args() const
    {
        return *(this->_args);
    }
    Object* State::thisObject()
    {
        if (nullptr == this->_thisObject) {
            this->_thisObject = se::Object::getObjectWithPtr(this->_nativeThisObject);
        }
        return this->_thisObject;
    }
    
    uint16_t State::argc() const
    {
        return this->_argc;
    }
    
    void State::setRetVal(se::Value value)
    {
        this->_retVal = value;
    }
    
    const Value& State::rval()const
    {
        return this->_retVal;
    }
   }
