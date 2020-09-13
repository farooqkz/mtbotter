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
using namespace std;
using namespace con;

#define PROTOCOL_ID 0x4f457403

// These are for Mtbotter::move()
#define FORWARD     0b1
#define BACKWARD    0b10
#define LEFT        0b100
#define RIGHT       0b1000
#define JUMP        0b10000
#define SNEAK       0b100000


bool got_sigint = false;


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
    public:
        MtBotter(const char* botname,
                 const string &password,
                 const string &address_name,
                 const string hostname,
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
        //bool punch();
        bool place();
        bool dig();
        void sendChat(wstring message);
        PointedThing getPointedThing();
    protected:
        virtual void onRemoveNode(unsigned short pos[]) = 0;
        virtual void onAddNode(unsigned short pos[]) = 0;
        virtual void onChatMessage(wstring message) = 0;
        virtual void run() = 0;
        void step(float dtime);

};


MtBotter::MtBotter(const char* botname,
                   const string &password,
                   const string &address_name,
                   const string hostname,
                   const unsigned short port,
                   bool ipv6) {
    sockets_init();
    atexit(sockets_cleanup);
    srand(time(NULL));
    mysrand(time(NULL));
    m_address.setPort(port);
    m_address.Resolve(hostname.c_str());
    
    m_itemdef = createItemDefManager();
    m_nodemgr = createNodeDefManager();
    m_emgr = new EventManager();
    m_client = new Client(botname, password, address_name, m_mpc, nullptr, nullptr, m_itemdef,m_nodemgr,nullptr, m_emgr, false, nullptr);
    m_client->m_simple_singleplayer_mode = false;
}

bool MtBotter::connect() {
    m_client->connect(m_address, true);
    bool connect_ok = false;
    double dtime = 1.0;
    while (true) {
        m_client->step(dtime);
        if (m_client->getState() == LC_Init) {
            connect_ok = true;
            break;
        }
        if (m_client->accessDenied()) {
            break;
        }
    }
    if (!connect_ok)
        return false;

    while (true) {
        m_client->step(dtime);
        if (m_client->getState() < LC_Init) {
            return false;
        }
        if (m_client->mediaReceived() && m_client->itemdefReceived() && m_client->nodedefReceived())
            break;
        if (m_client->accessDenied()) {
            return false;
        }
    }
    m_client->afterContentReceived();
    while (!m_client->connectedToServer()) {
        m_client->step(1.0);
    }
    return true;
}

PointedThing MtBotter::getPointedThing() {
    LocalPlayer *player = m_client->getEnv().getLocalPlayer();
    
    // https://stackoverflow.com/questions/10569659/camera-pitch-yaw-to-direction-vector
    const float yaw = getHeadingH();
    const float pitch = getHeadingV();
    const float xzLen = cos(pitch);
    const v3f camera_dir = v3f(xzLen * sin(-yaw),
                               sin(pitch),
                               xzLen * cos(yaw));

    ItemStack selected_item, hand_item;
    const ItemStack &tool_item = player->getWieldedItem(&selected_item, &hand_item);

    const ItemDefinition &selected_def = selected_item.getDefinition(m_itemdef);
    f32 d = getToolRange(selected_def, hand_item.getDefinition(m_itemdef));
    
    core::line3d<f32> shootline;
    shootline.start = player->getEyePosition();
    shootline.end = shootline.start + camera_dir * BS * d;

    PointedThing pt;
    RaycastState s(shootline, true,
                   selected_def.liquids_pointable);
    m_client->getEnv().continueRaycast(&s, &pt);
    return pt;

}
bool MtBotter::dig() {
    if (!m_client->checkPrivilege("interact")) {
        return false;
    }
    PointedThing pt = getPointedThing();

    if (pt.type != POINTEDTHING_NODE)
        return false;

    v3s16 nodepos = pt.node_undersurface;
    LocalPlayer *player = m_client->getEnv().getLocalPlayer();
    ClientMap &map = m_client->getEnv().getClientMap();
    MapNode n = m_client->getEnv().getClientMap().getNode(nodepos);

    ItemStack selected_item, hand_item;
    const ItemStack &tool_item = player->getWieldedItem(&selected_item, &hand_item);
    DigParams params =
            getDigParams(m_nodemgr->get(n).groups,
                         &selected_item.getToolCapabilities(m_itemdef));

    if (!params.diggable) {
        params =
            getDigParams(m_nodemgr->get(n).groups,
                         &hand_item.getToolCapabilities(m_itemdef));
    }
    if (!params.diggable) {
        return false;
    }

    m_client->interact(INTERACT_START_DIGGING, pt);
    bool is_valid_pos;
    MapNode wasnode = map.getNode(nodepos, &is_valid_pos);
    if (is_valid_pos) {
        const ContentFeatures &f = m_client->ndef()->get(wasnode);
        if (f.node_dig_prediction == "air") {
            m_client->removeNode(nodepos);
        } else if (!f.node_dig_prediction.empty()) {
            content_t id;
            bool found = m_client->ndef()->getId(f.node_dig_prediction, id);
            if (found)
                m_client->addNode(nodepos, id, true);
        }
    }
    m_client->interact(INTERACT_DIGGING_COMPLETED, pt);
    
    return true;
}

bool MtBotter::place() {
    if (!m_client->checkPrivilege("interact")) {
        return false;
    }
    PointedThing pt = getPointedThing();
    
    if (pt.type != POINTEDTHING_NODE)
        return false;
    
    LocalPlayer *player = m_client->getEnv().getLocalPlayer();
    ItemStack selected_item, hand_item;
    const ItemStack &tool_item = player->getWieldedItem(&selected_item, &hand_item);
    
    ClientMap &map = m_client->getEnv().getClientMap();
    auto &def = selected_item.getDefinition(m_itemdef);
    
    MapNode node;
    bool is_valid_pos;
    v3s16 nodepos = pt.node_undersurface;
    v3s16 p = pt.node_abovesurface;
    node = map.getNode(nodepos, &is_valid_pos);

    if (!is_valid_pos)
        return false;
    
    if (m_nodemgr->get(node).buildable_to) {
        p = nodepos; 
        
    } else {
        node = map.getNode(p, &is_valid_pos);
        if (is_valid_pos && !m_nodemgr->get(node).buildable_to) {
            m_client->interact(INTERACT_PLACE, pt);
            return false;
        }
    }
    
    v3s16 standing_node = player->getStandingNodePos();

    if (standing_node + v3s16(0, 1, 0) == p) {
        return false;
    }
    if (standing_node + v3s16(0, 2, 0) == p) {
        return false;
    }

    m_client->interact(INTERACT_PLACE, pt);
    return true;
}

void MtBotter::start() {
    run();
}

void MtBotter::step(float dtime) {
    m_client->step(dtime);
    if (m_client->getHP() == 0) {
        m_client->sendRespawn();
    }
    m_itemdef->processQueue(m_client);
    // some event loop(s) here
}

MtBotter::~MtBotter() {
    delete m_client;
    delete m_nodemgr;
    delete m_emgr;
    delete m_itemdef;
}

short MtBotter::getPosX() {
    return m_client->getEnv().getLocalPlayer()->getStandingNodePos().X;
}

short MtBotter::getPosY() {
    return m_client->getEnv().getLocalPlayer()->getStandingNodePos().Y;
}

short MtBotter::getPosZ() {
    return m_client->getEnv().getLocalPlayer()->getStandingNodePos().Z;
}

void MtBotter::turnRight(float deg) {
    PlayerControl control;
    control.yaw = fmod(getHeadingH() + deg + 360, 360);
    m_client->setPlayerControl(control);
}

void MtBotter::turnRight() {
    turnRight(15.0);
}

void MtBotter::turnLeft(float deg) {
    PlayerControl control;
    control.yaw = fmod(getHeadingH() - deg + 360, 360);
    m_client->setPlayerControl(control);
}

void MtBotter::turnLeft() {
    turnLeft(15.0);
}

void MtBotter::turnHeadUp(float deg) {
    PlayerControl control;
    control.pitch = getHeadingV() + deg;
    m_client->setPlayerControl(control);
}

void MtBotter::turnHeadUp() {
    turnHeadUp(5.0);
}

void MtBotter::turnHeadDown(float deg) {
    PlayerControl control;
    control.pitch = getHeadingV() - deg;
    m_client->setPlayerControl(control);
}

void MtBotter::turnHeadDown() {
    turnHeadDown(5.0);
}

// Pitch
float MtBotter::getHeadingV() {
    return m_client->getEnv().getLocalPlayer()->getPitch();
}

// Yaw
float MtBotter::getHeadingH() {
    return m_client->getEnv().getLocalPlayer()->getYaw();
}

void MtBotter::move(unsigned flags) {
    if (flags == 0)
        return;

    PlayerControl control;
    if (flags & FORWARD)
        control.up = true;
    if (flags & BACKWARD)
        control.down = true;
    if (flags & LEFT)
        control.left = true;
    if (flags & RIGHT)
        control.right = true;
    if (flags & JUMP)
        control.jump = true;
    if (flags & SNEAK)
        control.sneak = true;
    control.pitch = getHeadingV();
    control.yaw = getHeadingH();
    m_client->setPlayerControl(control);
}
void signal_handler(int x) {
    got_sigint = true;
}

class MB : public MtBotter {
    public:
    MB():MtBotter("testbot",
                      string(""),
                      string("127.0.0.1"),
                      string("127.0.0.1"),
                      30000,
                      false){}
    private:
    void onRemoveNode(unsigned short pos[]) {

    }

    void onAddNode(unsigned short pos[]) {

    }

    void onChatMessage(wstring message) {

    }

    void run() {
        while (getHeadingH() != 0.0)
            turnLeft();
        while (getHeadingV() != 0.0) {
            if (getHeadingV() < 0.0) {
                turnHeadUp();
            } else {
                turnHeadDown();
            }
        }
        bool placed = false;
        while (!got_sigint) {
            step(2.0);
            placed = place();
        }
    }
};

int main() {
    struct sigaction action;
    action.sa_handler = signal_handler;
    sigaction(SIGINT, &action, NULL);
    MB *mb = new MB();
    cout << "Created an instance" << endl;
    mb->connect();
    cout << "Connected! Now running" << endl;
    mb->start();
    cout << "THE END" << endl;
    delete mb;
    cout << "Bye..." << endl;
    return 0;
    /*
    while(1) {
        if (got_sigint) {
            delete client;
            cout << "Bye..." << endl;
            return 0;
        }
        if (client->accessDenied()) {
            cout << "Access Denied because..." << endl
                 << client->accessDeniedReason() << endl;
            break;
        }
        if (g_gamecallback != nullptr && g_gamecallback->disconnect_requested) {
            cout << "Disconnect requested" << endl;
            break;
        }
        

        itemdef->processQueue(client);
        if (client->getHP() == 0) {
            client->sendRespawn();
        }
        client->step(dtime);
                        
    }
    return 0;
    */
}
