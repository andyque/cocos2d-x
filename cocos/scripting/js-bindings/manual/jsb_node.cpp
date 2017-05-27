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

void Node_finalized(se::State& state)
{
    auto nativeThisObject = state.nativeThisObject();
    if (nativeThisObject)
    {
        Node* thiz = (Node*) nativeThisObject;
        printf("Node_finalized %p ...\n", thiz->getUserData());
        SAFE_RELEASE(thiz);
    }
}

SE_FINALIZE_FUNC(Node_finalized)

bool Node_constructor(se::State& state)
{
    printf("Node_constructor ...\n");
    Node* obj = new Node();
    state.thisObject()->setPrivateData(obj);
    return true;
}

SE_CTOR(Node_constructor, __jsb_Node_class, _SE(Node_finalized))

bool Node_ctor(se::State& state)
{
    printf("Node_ctor ...\n");
    Node* obj = new Node();
    state.thisObject()->setPrivateData(obj);
    return true;
}
SE_CTOR(Node_ctor, __jsb_Node_class, _SE(Node_finalized))

bool Node_create(se::State& state)
{
    Node* node = Node::create();
    node->retain();
    auto obj = se::Object::createObjectWithClass(__jsb_Node_class, false);
    obj->setPrivateData(node);
    state.setRetVal(se::Value(obj));
    
    return true;
}

SE_FUNCTION(Node_create)

bool Node_onEnter(se::State& state)
{
    ScriptingCore::getInstance()->setCalledFromScript(true);
    Node* thiz = (Node*)state.nativeThisObject();
    thiz->onEnter();
    return true;
}

SE_FUNCTION(Node_onEnter);

bool Node_onExit(se::State& state)
{
    ScriptingCore::getInstance()->setCalledFromScript(true);
    Node* thiz = (Node*)state.nativeThisObject();
    thiz->onExit();
    return true;
}

SE_FUNCTION(Node_onExit);

bool Node_onEnterTransitionDidFinish(se::State& state)
{
    ScriptingCore::getInstance()->setCalledFromScript(true);
    Node* thiz = (Node*)state.nativeThisObject();
    thiz->onEnterTransitionDidFinish();
    return true;
}

SE_FUNCTION(Node_onEnterTransitionDidFinish);

bool Node_onExitTransitionDidStart(se::State& state)
{
    ScriptingCore::getInstance()->setCalledFromScript(true);
    Node* thiz = (Node*) state.nativeThisObject();
    thiz->onExitTransitionDidStart();
    return true;
}

SE_FUNCTION(Node_onExitTransitionDidStart);


bool Node_cleanup(se::State& state)
{
    ScriptingCore::getInstance()->setCalledFromScript(true);
    Node* thiz = (Node*)state.nativeThisObject();
    thiz->cleanup();
    return true;
}

SE_FUNCTION(Node_cleanup);

bool Node_addChild(se::State& state)
{
    Node* thiz = (Node*)state.nativeThisObject();
    Node* child = (Node*)state.args()[0].toObject()->getPrivateData();
    thiz->addChild(child);
    return true;
}

SE_FUNCTION(Node_addChild);



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

bool Node_schedule(se::State& state)
{
    printf("--------------------------\ntarget count: %d\n", (int)__jsthis_schedulekey_map.size());
    for (const auto& e1 : __jsthis_schedulekey_map)
    {
        printf("target: %p, functions: %d\n", e1.first, (int)e1.second.size());
    }
    printf("-------------------------- \n");
    static uint32_t __idx = 0;
    Node* thiz = (Node*)state.nativeThisObject();
    se::Value jsThis(state.thisObject());
    auto& args = state.args();
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
    auto argc = state.argc();
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
    return true;
}

SE_FUNCTION(Node_schedule);


bool Node_unschedule(se::State& state)
{
    auto& args = state.args();
    Node* thiz = (Node*)state.nativeThisObject();
    se::Value jsThis(state.thisObject());
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
    return true;
}

SE_FUNCTION(Node_unschedule);

bool Node_set_x(se::State& state)
{
    Node* thiz = (Node*)state.nativeThisObject();
    printf("cc.Node set_x native obj: %p\n", thiz);
    float x = state.rval().toNumber();
    thiz->setPositionX(x);
    return true;
}

SE_SET_PROPERTY(Node_set_x)

bool Node_get_x(se::State& state)
{
    Node* thiz = (Node*)state.nativeThisObject();
    state.setRetVal(se::Value(thiz->getPositionX()));
    return true;
}

SE_GET_PROPERTY(Node_get_x)

bool Node_set_y(se::State& state)
{
    Node* thiz = (Node*)state.nativeThisObject();
    printf("cc.Node set_y native obj: %p\n", thiz);
    float y = state.rval().toNumber();
    thiz->setPositionY(y);
    return true;
}
SE_SET_PROPERTY(Node_set_y)

bool Node_get_y(se::State& state)
{
    Node* thiz = (Node*)state.nativeThisObject();
    state.setRetVal(se::Value(thiz->getPositionY()));
    return true;
}

SE_GET_PROPERTY(Node_get_y)

bool jsb_register_Node()
{
    auto cls = se::Class::create("Node", __ccObj, nullptr, _SE(Node_constructor));
    cls->defineStaticFunction("create", _SE(Node_create));

    cls->defineProperty("x", _SE(Node_get_x), _SE(Node_set_x));
    cls->defineProperty("y", _SE(Node_get_y), _SE(Node_set_y));

    cls->defineFunction("ctor", _SE(Node_ctor));
    cls->defineFunction("onEnter", _SE(Node_onEnter));
    cls->defineFunction("onExit", _SE(Node_onExit));
    cls->defineFunction("onEnterTransitionDidFinish", _SE(Node_onEnterTransitionDidFinish));
    cls->defineFunction("onExitTransitionDidStart", _SE(Node_onExitTransitionDidStart));
    cls->defineFunction("cleanup", _SE(Node_cleanup));
    cls->defineFunction("schedule", _SE(Node_schedule));
    cls->defineFunction("unschedule", _SE(Node_unschedule));

    cls->defineFunction("addChild", _SE(Node_addChild));
    cls->defineFinalizedFunction(_SE(Node_finalized));

    cls->install();

    __jsb_Node_proto = cls->getProto();
    __jsb_Node_class = cls;

    return true;
}
