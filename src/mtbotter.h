#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstdlib>
#include <string>
#include <codecvt>
#include <csignal>
#include <cmath>
#include "map.h"
#include "client/client.h"
#include "tool.h"
#include "client/clientmap.h"
#include "network/address.h"
#include "network/socket.h"
#include "nodedef.h"
#include "itemdef.h"
#include "event.h"
#include "client/event_manager.h"
#include "gui/mainmenumanager.h"
#include "constants.h"
#include "client/camera.h"
#include "util/pointedthing.h"
#include <line3d.h>
#include "raycast.h"
#include "activeobject.h"
#include "client/clientobject.h"
#include <list>
#include "client/content_cao.h"
#include <unordered_map>
#include "network/networkpacket.h"
#include "network/networkprotocol.h"
#include "irr_aabb3d.h"


// These are for Mtbotter::move()
#define FORWARD     0b1
#define BACKWARD    0b10
#define LEFT        0b100
#define RIGHT       0b1000
#define JUMP        0b10000
#define SNEAK       0b100000

struct SomeObject {
    unsigned short id;
    float posX, posY, posZ;
    std::unordered_map<std::string, int> groups;
    bool immortal, localplayer;
    float height, width;
};

struct SomeItemStack {
    std::string name;
    unsigned short count, wear;
};

enum PThingType {
    PTHING_NOTHING,
    PTHING_NODE,
    PTHING_OBJECT
};

struct PThing {
    PThingType type;
    short node_undersurface[3];
    short node_abovesurface[3];
};

struct SomeNode {
    std::string name;
    bool is_ground_content, walkable, pointable, diggable;
    bool climbable, buildable_to, rightclickable;
    unsigned long dmg_p_sec;
};


class MtBotter {
    private:
        Client *m_client;
        MapDrawControl m_mpc;
        NodeDefManager *m_nodemgr;
        IWritableItemDefManager *m_itemdef;
        EventManager *m_emgr;
        Address m_address;
        float m_pitch;
        float m_yaw;
        float m_hit;
        PointedThing getPointedThing();
    public:
        MtBotter(const char* botname,
                 const std::string &password,
                 const std::string &address_name,
                 const std::string hostname,
                 const unsigned short port,
                 bool ipv6);
        ~MtBotter();
        bool connect();
        void start();
        void turnRight(float deg);
        void turnRight();
        void turnLeft(float deg);
        void turnLeft();
        void turnHeadUp(float deg);
        void turnHeadUp();
        void turnHeadDown(float deg);
        void turnHeadDown();
        float getHeadingV();
        float getHeadingH();
        void move(unsigned flags);
        short getPosX();
        short getPosY();
        short getPosZ();
        bool punch();
        bool place();
        bool place(bool noplace);
        bool dig();
        bool activate();
        void sendChat(std::wstring message);
        void step(float dtime);
        std::list<SomeObject> getNearestObjects(float max_d);
        bool getNearestObject(float max_d, SomeObject &obj);
        unsigned short getHP();
        unsigned short getBreath();
        bool isDead();
        std::list<std::string> getPlayerNames();
        void setWieldIndex(unsigned short index);
        unsigned short getWieldIndex();
        SomeItemStack getWieldedItem();
        PThing getPThing();
        SomeNode getNode(short x, short y, short z);
    protected:
        virtual void onRemoveNode(short pos[]) = 0;
        virtual void onAddNode(short pos[]) = 0;
        virtual void onChatMessage(unsigned char type, std::wstring sender, std::wstring message) = 0;
        virtual void onConnect() = 0;
        virtual void onDisconnect(std::string reason) = 0;
        virtual void onTime(unsigned short t) = 0;
        virtual void onInventoryUpdate() = 0;
        virtual void onPlayerMove(float pos[]) = 0;
        virtual void run() = 0;
};
