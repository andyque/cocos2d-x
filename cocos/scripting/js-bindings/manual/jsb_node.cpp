//
//  jsb_node.cpp
//  cocos2d_js_bindings
//
//  Created by James Chen on 4/26/17.
//
//

#include "jsb_node.hpp"
#include "jsb_global.h"
#include "ScriptingCore.h"

#include "cocos2d.h"

using namespace cocos2d;

se::Object* __jsb_Node_proto = nullptr;
se::Class* __jsb_Node_class = nullptr;

void Node_finalized(void* nativeThisObject)
{
    if (nativeThisObject)
    {
        Node* thiz = (Node*) nativeThisObject;
        printf("Node_finalized %p ...\n", thiz->getUserData());
        SAFE_RELEASE(thiz);
    }
}

SE_FINALIZE_FUNC(Node_finalized)

void Node_constructor(se::Object* thisObject, const se::ValueArray& args)
{
    printf("Node_constructor ...\n");
    Node* obj = new Node();
    thisObject->setPrivateData(obj);
}

SE_CTOR(Node_constructor, __jsb_Node_class, Node_finalized)

void Node_ctor(se::Object* thisObject, const se::ValueArray& args)
{
    printf("Node_ctor ...\n");
    Node* obj = new Node();
    thisObject->setPrivateData(obj);
}
SE_CTOR(Node_ctor, __jsb_Node_class, SE_FUN_WRAPPER(Node_finalized))

se::Object* Node_create(se::Object* thisObject, const se::ValueArray& args,int argc, void* nativeThisObject)
{
    Node* node = Node::create();
    node->retain();
    auto obj = se::Object::createObjectWithClass(__jsb_Node_class, false);
    obj->setPrivateData(node);
    return obj;
}

SE_FUNCTION_RETVAL(Node_create, se::DONT_NEED_THIS)

void Node_onEnter(se::Object* thisObject, const se::ValueArray& args,int argc, void* nativeThisObject)
{
    ScriptingCore::getInstance()->setCalledFromScript(true);
    Node* thiz = (Node*)nativeThisObject;
    thiz->onEnter();
}

SE_FUNCTION_VOID(Node_onEnter, se::DONT_NEED_THIS);

void Node_onExit(se::Object* thisObject, const se::ValueArray& args, int argc,void* nativeThisObject)
{
    ScriptingCore::getInstance()->setCalledFromScript(true);
    Node* thiz = (Node*)nativeThisObject;
    thiz->onExit();
}

SE_FUNCTION_VOID(Node_onExit, se::DONT_NEED_THIS);

void Node_onEnterTransitionDidFinish(se::Object* thisObject, const se::ValueArray& args, int argc, void* nativeThisObject)
{
    ScriptingCore::getInstance()->setCalledFromScript(true);
    Node* thiz = (Node*)nativeThisObject;
    thiz->onEnterTransitionDidFinish();
}

SE_FUNCTION_VOID(Node_onEnterTransitionDidFinish, se::DONT_NEED_THIS);

void Node_onExitTransitionDidStart(se::Object* thisObject, const se::ValueArray& args, int argc, void* nativeThisObject)
{
    ScriptingCore::getInstance()->setCalledFromScript(true);
    Node* thiz = (Node*) nativeThisObject;
    thiz->onExitTransitionDidStart();
}

SE_FUNCTION_VOID(Node_onExitTransitionDidStart, se::DONT_NEED_THIS);


void Node_cleanup(se::Object* thisObject, const se::ValueArray& args, int argc, void* nativeThisObject)
{
    ScriptingCore::getInstance()->setCalledFromScript(true);
    Node* thiz = (Node*)nativeThisObject;
    thiz->cleanup();
}

SE_FUNCTION_VOID(Node_cleanup, se::DONT_NEED_THIS);

void Node_addChild(se::Object* thisObject, const se::ValueArray& args, int argc, void* nativeThisObject)
{
    Node* thiz = (Node*)nativeThisObject;
    Node* child = (Node*)args[0].toObject()->getPrivateData();
    thiz->addChild(child);

}

SE_FUNCTION_VOID(Node_addChild, se::DONT_NEED_THIS);



static std::unordered_map<se::Object*, std::unordered_map<se::Object*, std::string>> __jsthis_schedulekey_map;

static bool isScheduleExist(se::Object* jsFunc, se::Object* jsThis, se::Object** outJsFunc, se::Object** outJsThis, std::string* outKey)
{
    bool found = false;
    for (const auto& e : __jsthis_schedulekey_map)
    {
        if (e.first->isSame(jsThis))
        {
            for (const auto& e2 : e.second)
            {
                if (e2.first->isSame(jsFunc))
                {
                    *outJsThis = e.first;
                    *outJsFunc = e2.first;
                    *outKey = e2.second;
                    break;
                }
            }
        }
        if (!(*outKey).empty())
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        (*outKey).clear();
        (*outJsThis) = nullptr;
        (*outJsFunc) = nullptr;
    }

    return found;
}

static void removeSchedule(se::Object* jsFunc, se::Object* jsThis)
{
    auto funcObjKeyMapIter = __jsthis_schedulekey_map.find(jsThis);
    if (funcObjKeyMapIter != __jsthis_schedulekey_map.end())
    {
        jsThis->detachChild(jsFunc);
        funcObjKeyMapIter->second.erase(jsFunc);
        if (funcObjKeyMapIter->second.empty())
            __jsthis_schedulekey_map.erase(funcObjKeyMapIter);

        jsFunc->release();
        jsThis->release();
    }
}

static void removeScheduleByKey(const std::string& key)
{
    se::Object* jsFunc = nullptr;
    se::Object* jsThis = nullptr;

    for (const auto& e : __jsthis_schedulekey_map)
    {
        for (const auto& e2 : e.second)
        {
            if (e2.second == key)
            {
                jsThis = e.first;
                jsFunc = e2.first;
                break;
            }
        }

        if (jsThis != nullptr && jsFunc != nullptr)
            break;
    }

    removeSchedule(jsFunc, jsThis);
}

static void insertSchedule(se::Object* jsFunc, se::Object* jsThis, const std::string& key)
{
    auto& funcKeyMap = __jsthis_schedulekey_map[jsThis];
    funcKeyMap.emplace(jsFunc, key);
    jsFunc->addRef();
    jsThis->addRef();
}

class UnscheduleNotifier
{
public:
    UnscheduleNotifier(Node* node, const std::string& key)
    : _node(node)
    , _key(key)
    {
        _node->retain();
    }
    ~UnscheduleNotifier()
    {
        printf("~UnscheduleNotifier, node: %p, key: %s\n", _node, _key.c_str());

        Node* node = _node;
        std::string key = _key;
        Director::getInstance()->getScheduler()->performFunctionInCocosThread([node, key](){
            removeScheduleByKey(key);
            if (node->isScheduled(key))
                node->unschedule(key);

            node->release();
        });
    }

private:
    Node* _node;
    std::string _key;
};

void Node_schedule(se::Object* thisObject, const se::ValueArray& args, int argc, void* nativeThisObject)
{
    printf("--------------------------\ntarget count: %d\n", (int)__jsthis_schedulekey_map.size());
    for (const auto& e1 : __jsthis_schedulekey_map)
    {
        printf("target: %p, functions: %d\n", e1.first, (int)e1.second.size());
    }
    printf("-------------------------- \n");
    static uint32_t __idx = 0;
    Node* thiz = (Node*)nativeThisObject;
    se::Value jsThis(thisObject);
    se::Value jsFunc(args[0]);
    jsThis.toObject()->attachChild(jsFunc.toObject());
    
    se::Object* foundThisObj = nullptr;
    se::Object* foundFuncObj = nullptr;
    std::string key;
    
    bool found = isScheduleExist(jsFunc.toObject(), jsThis.toObject(), &foundFuncObj, &foundThisObj, &key);
    if (found && !key.empty())
    {
        removeSchedule(foundFuncObj, foundThisObj);
        thiz->unschedule(key);
    }
    
    float interval = 0.0f;
    unsigned int repeat = CC_REPEAT_FOREVER;
    float delay = 0.0f;
    key = StringUtils::format("__node_schedule_key:%u", __idx++);
    
    if (argc >= 2)
        interval = args[1].toNumber();
    
    if (argc >= 3)
        repeat = args[2].toNumber();
    
    if (argc >= 4)
        delay = args[3].toNumber();
    
    insertSchedule(jsFunc.toObject(), jsThis.toObject(), key);
    std::shared_ptr<UnscheduleNotifier> unscheduleNotifier = std::make_shared<UnscheduleNotifier>(thiz, key);
    
    thiz->schedule([jsThis, jsFunc, unscheduleNotifier](float dt){
        se::Object* thisObj = jsThis.toObject();
        se::Object* funcObj = jsFunc.toObject();
        
        se::ValueArray args;
        args.push_back(se::Value((double)dt));
        funcObj->call(args, thisObj);
        
    }, interval, repeat, delay, key);
}

SE_FUNCTION_VOID(Node_schedule, se::DONT_NEED_THIS);


void Node_unschedule(se::Object* thisObject, const se::ValueArray& args, int argc, void* nativeThisObject)
{
    Node* thiz = (Node*)nativeThisObject;
    se::Value jsThis(thisObject);
    se::Value jsFunc(args[0]);
    se::Object* foundThisObj = nullptr;
    se::Object* foundFuncObj = nullptr;
    std::string key;
    
    bool found = isScheduleExist(jsFunc.toObject(), jsThis.toObject(), &foundFuncObj, &foundThisObj, &key);
    
    if (found && !key.empty())
    {
        removeSchedule(foundFuncObj, foundThisObj);
        thiz->unschedule(key);
    }
    else
    {
        printf("Node_unschedule not found\n");
    }
}

SE_FUNCTION_VOID(Node_unschedule, se::DONT_NEED_THIS);

void Node_set_x(se::Object* thisObject, void* nativeThisObject, const se::Value& data)
{
    Node* thiz = (Node*)nativeThisObject;
    printf("cc.Node set_x native obj: %p\n", thiz);
    float x = data.toNumber();
    thiz->setPositionX(x);
}

SE_SET_PROPERTY(Node_set_x, se::DONT_NEED_THIS)

se::Value Node_get_x(se::Object* thisObject, void* nativeThisObject)
{
    Node* thiz = (Node*)nativeThisObject;
    return se::Value(thiz->getPositionX());
}

SE_GET_PROPERTY(Node_get_x, se::DONT_NEED_THIS)

void Node_set_y(se::Object* thisObject, void* nativeThisObject, const se::Value& data)
{
    Node* thiz = (Node*)nativeThisObject;
    printf("cc.Node set_y native obj: %p\n", thiz);
    float y = data.toNumber();
    thiz->setPositionY(y);
}
SE_SET_PROPERTY(Node_set_y, se::DONT_NEED_THIS)

se::Value Node_get_y(se::Object* thisObject, void* nativeThisObject)
{
     Node* thiz = (Node*)nativeThisObject;
    return se::Value(thiz->getPositionY());
}

SE_GET_PROPERTY(Node_get_y, se::DONT_NEED_THIS)

bool jsb_register_Node()
{
    auto cls = se::Class::create("Node", __ccObj, nullptr, SE_FUN_WRAPPER(Node_constructor));
    cls->defineStaticFunction("create", SE_FUN_WRAPPER(Node_create));

    cls->defineProperty("x", SE_PROPERTY_WRAPPER(Node_get_x), SE_PROPERTY_WRAPPER(Node_set_x));
    cls->defineProperty("y", SE_PROPERTY_WRAPPER(Node_get_y), SE_PROPERTY_WRAPPER(Node_set_y));

    cls->defineFunction("ctor", SE_FUN_WRAPPER(Node_ctor));
    cls->defineFunction("onEnter", SE_FUN_WRAPPER(Node_onEnter));
    cls->defineFunction("onExit", SE_FUN_WRAPPER(Node_onExit));
    cls->defineFunction("onEnterTransitionDidFinish", SE_FUN_WRAPPER(Node_onEnterTransitionDidFinish));
    cls->defineFunction("onExitTransitionDidStart", SE_FUN_WRAPPER(Node_onExitTransitionDidStart));
    cls->defineFunction("cleanup", SE_FUN_WRAPPER(Node_cleanup));
    cls->defineFunction("schedule", SE_FUN_WRAPPER(Node_schedule));
    cls->defineFunction("unschedule", SE_FUN_WRAPPER(Node_unschedule));

    cls->defineFunction("addChild", SE_FUN_WRAPPER(Node_addChild));
    cls->defineFinalizedFunction(Node_finalized);

    cls->install();

    __jsb_Node_proto = cls->getProto();
    __jsb_Node_class = cls;

    return true;
}
