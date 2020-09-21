#include <iostream>
#include <cmath>
#include <list>
#include "mtbotter.h"
using namespace std;

bool got_sigint = false;

class MonsterKiller : public MtBotter {
    public:
    MonsterKiller():MtBotter("testbot",
                      string(""),
                      string("127.0.0.1"),
                      string("127.0.0.1"),
                      30000,
                      false){}
    private:
    void onRemoveNode(short pos[]) {}
    void onAddNode(short pos[]) {}
    void onChatMessage(unsigned char type, wstring sender, wstring message) {}
    void onConnect() {
        cout << "[] Connected!" << endl;
    }
    void onDisconnect(string reason) {}
    void onInventoryUpdate() {}
    void onPlayerMove(float pos[]) {}
    void onTime(unsigned short t){}

    void run() {
        unsigned i = 0;
        list<SomeObject> objs;
        SomeObject obj;
        bool got_wanted;
        while (!got_sigint) {
            step(2.0);
            got_wanted = false;
            if (!(i++ % 135)) {
                objs = getNearestObjects(100);
                for (auto someobj : objs) {
                    if (someobj.localplayer) {
                        continue;
                    }
                    auto fleshy=someobj.groups.find("fleshy");
                    auto immortal=someobj.immortal;
                    if (fleshy == someobj.groups.end())
                        continue;
                    if (fleshy->second > 0 && immortal) {
                        obj = someobj;
                        got_wanted = true;
                        break;
                    }
                }
                if (!got_wanted) {
                    step(0.5);
                    continue;
                }
                float diff_x = obj.posX - getPosX()/10;
                float diff_z = obj.posZ - getPosZ()/10;
                float radian, degree;
                radian = atan2(diff_x, diff_z);
                degree = 360 - radian * 180 / M_PI;
                turnLeft(degree - getHeadingH());
                step(1.0);
                punch();
            }
        }
    }
};

int main() {
    MonsterKiller *mk = new MonsterKiller();
    mk->connect();
    mk->start();
    return 0;
}
