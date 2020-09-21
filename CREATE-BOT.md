# Creating a Minetest bot with MtBotter

In this article, I suppose you are using C++. For other languages see other languages
section in README.md

You need to include `mtbotter.h` in your project and create
a subclass of `MtBotter` class whose definition goes as below:

```cpp
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
```

You should override all virtual methods(even if you don't need
to use them) and you can use public methods of `MtBotter`
class. `run()` is called upon starting the bot. You should
put a loop there and do whatever the bot should do.
