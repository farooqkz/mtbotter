#include <iostream>
#include <unistd.h>
#include <ctime>
#include <cstdlib>
#include <string>
#include <codecvt>
#include <csignal>
#include "map.h"
#include "client/client.h"
#include "client/clientmap.h"
#include "network/address.h"
#include "network/socket.h"
#include "nodedef.h"
#include "itemdef.h"
#include "event.h"
#include "client/event_manager.h"
#include "gui/mainmenumanager.h"
using namespace std;
using namespace con;

#define PROTOCOL_ID 0x4f457403

bool got_sigint = false;

class MER : public MapEventReceiver {
    public:
    void onMapEditEvent(const MapEditEvent &event) {
        cout << event.type << endl;
        for (auto mb : event.modified_blocks) {
            cout << mb.X << "," << mb.Y << "," << mb.Z << endl;
        }
    }
};

void signal_handler(int x) {
    got_sigint = true;
}

int main() {
    cout << "Hi :)" << endl;
    struct sigaction action;
    action.sa_handler = signal_handler;
    sigaction(SIGINT, &action, NULL);
    wstring ws = wstring_convert<codecvt_utf8<wchar_t>>().from_bytes("Hello! I am a useless bot.");
    string pass("");
    string addr("127.0.0.1");
    string pname("testing");
    MapDrawControl mpc;
    NodeDefManager *nodemgr = createNodeDefManager();
    IWritableItemDefManager *itemdef = createItemDefManager();
    EventManager *emgr = new EventManager();
    Address address(127, 0, 0, 1, 30000);
    sockets_init();
    atexit(sockets_cleanup);
    srand(time(NULL));
    mysrand(time(NULL));
    Client *client = new Client(pname.c_str(), pass, addr, mpc, nullptr, nullptr, itemdef,nodemgr,nullptr, emgr, false, nullptr);
    client->m_simple_singleplayer_mode = false;
    client->connect(address, true);
    cout << "Connected?" << endl << flush;
    bool connect_ok = false;
    f32 dtime = 1.0;
    while (1) {
        client->step(dtime);
        if (client->getState() == LC_Init) {
            connect_ok = true;
            break;
        }
        if (client->accessDenied()) {
            cout << "Access denied because " <<
                    client->accessDeniedReason() << endl << flush;
            break;
        }
    }
    cout << "o: " << connect_ok << endl << flush;
    if (!connect_ok) return 1;
    u64 c = 0;
    while (1) {
        client->step(dtime);
        if (client->getState() < LC_Init) {
            cout << ":(" << endl << flush;
            return 1;
        }
        if (client->mediaReceived() && client->itemdefReceived() && client->nodedefReceived())
            break;
        if (client->accessDenied()) {
            cout << "Access denied... " << client->accessDeniedReason() << endl;
            return 1;
        }
        if (!client->itemdefReceived()) {
            cout << c++ << " Item defs...";
        } else if (!client->nodedefReceived()) {
            cout << c++ << " Node defs...";
        } else {
            cout << c++ << " Media... " << client->mediaReceiveProgress()*100 << "%";
        }
    }
    client->afterContentReceived();
    while (!client->connectedToServer()) {
        client->step(dtime);
    }
    cout << "s: " << client->getState() << endl;
    bool first_time = false;
    MER *mer = new MER();
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
        if (!first_time) {
            client->sendChatMessage(ws);
            client->getEnv().getMap().addEventReceiver(mer);
            first_time = true;
        }
        LocalPlayer *lplayer = client->getEnv().getLocalPlayer();
        if (lplayer == nullptr) {
            cout << "No jumping yet" << endl;
            continue;
        }
        PlayerControl control;
        // Jump
        control.jump = true;
        client->setPlayerControl(control);
        //lplayer->keyPressed = (0x1) | (1U << 4);
        
        client->step(dtime);
        control.jump = false;
        control.up = true;
        client->setPlayerControl(control);

        client->step(dtime);
        control.up = false;
        control.down = true;
        client->setPlayerControl(control);

        client->step(dtime);
        control.down = false;
        control.LMB = true;
        client->setPlayerControl(control);

        wstring chat_msg;
        if (client->getChatMessage(chat_msg)) {
            cout << "Chat:";
            wcout << chat_msg << endl;
        }
    }
    return 0;
}
