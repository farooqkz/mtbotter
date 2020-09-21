# Creating a Minetest bot with MtBotter

In this article, I suppose you are using C++. For other languages see other languages
section in README.md

See `examples/` in project's root for some example bots.

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
Don't forget to call `step()` regularly in your loop so
that your bot can receive events and its commands will
be sent to server.

## Public methods which you can use in your bot
### MtBotter(...)

```cpp
MtBotter(const char* botname,
         const std::string &password,
         const std::string &address_name,
         const std::string hostname,
         const unsigned short port,
         bool ipv6)
```

 - `botname`, `password` and `port` explain themselves.
 - `ipv6` indicates if you are going to connect to an IPv6
server.
 - `hostname` is the (IP) address of the server you want to 
connect to.
 - `address_name` can be whatever you like but you'd better
make it the same as hostname.

### ~MtBotter()

Disconnects from server and deletes the created instances.

### bool connect()

Connects to server.

### void start()

Runs the bot. Should be called after `connect()`.

### void turnRight(float deg = 15)

Turns bot's body to right.

### void turnLeft(float deg = 15)

Turns bot's body to left.

### void turnHeadUp(float deg = 5)

Turns bot's head up.

### void turnHeadDown(float deg = 5)

Turns bot's head down.

### float getHeadingV(), float getHeadingH()

In order, they return current vertical heading(pitch)
and current horizontal heading(yaw) of bot.

### void move(unsigned flags)

Move the bot or jump and sneak with this. You should use one or more of these flags:

```
FORWARD, BACKWARD, LEFT, RIGHT, JUMP, SNEAK
```

Examples:

```cpp
move(FORWARD); // moves forward
move(FORWARD | JUMP); // move forward and jump at the same time
```

### short getPosX(), short getPosY(), short getPosZ()

They return the current position of the bot in the world.

### bool punch()

Punchs the pointed node or object. Returns `false` if failed.

### bool place(bool noplace = false)

Place the node which is wielded in bot's inventory.
If `noplace` is true, right clicks on the pointed node instead
of placing a new node.
Returns `false` if failed.

### bool dig()

Digs the pointed node and returns `true` if successful.

### bool activate()

Activates(rightclick in air) and returns true if successful.

### void sendChat(std::wstring message)

Sends the message to chat.

### step(float dtime)

Steps the client and receives the event. `dtime` can be from 0 to 2.
This should be called in order to have a working bot so that
commands(move, dig, place, ...) will be sent to server and
events will be recieved.

### std::list\<SomeObject\> getNearestObjects(float max\_d)

Returns a list of objects in `max_d` radius.
`SomeObject` is defined as below:

```cpp
struct SomeObject {
    unsigned short id;
    std::unordered_map<std::string, int> groups;
    bool immortal, localplayer;
}
```

### bool getNearestObject(float max\_d, SomeObject &obj)

Assigns `obj` to the nearest object which is not bot itself
and returns `true` if successful.

### unsigned short getHP()

Returns current HP of the bot.

### unsigned short getBreath()

Returns the current Oxygen level of the bot.

### bool isDead()

Returns `true` if the bot is dead.
Because MtBotter respawns the bot when it dies, you may rarely
(or never?) get `true` from this method.

### std::list<std::string> getPlayerNames()

Returns a list of players who are online in the server.

### void setWieldIndex(unsigned short index)

Sets the current selected item's index to `index`.

### unsigned short getWieldIndex()

Returns the current selected/wielded item index.

### SomeItemStack getWieldedItem()

Returns the current selected/wielded item in bot's inventory.
SomeItemStack is defined like this:

```cpp
struct SomeItemStack {
    std::string name;
    unsigned short count, wear;
}
```

### PThing getPThing()

Gets the current pointed thing. PThing is defined as below:

```cpp
struct PThing {
    PThingType type;
    short node_undersurface[3]; // X, Y, Z
    short node_abovesurface[3];
}
```

`type` is one of `PTHING_NOTHING`, `PTHING_NODE`, `PTHING_OBJECT`.

### SomeNode getNode(short x, short y, short z)

Returns the node at given position.

SomeNode is defined like this:

```cpp
struct SomeNode {
    std::string name; // empty if unknown
    bool is_ground_content, walkable, pointable, diggable;
    bool climbable, buildable_to, rightclickable;
    unsigned long dmg_p_sec; // damage per second
}
```

## Protected virtual methods which you should override
You should override all of these even if you don't need to.
You can keep the one you don't need empty
### void onRemoveNode(short pos[])

pos -> X, Y, Z
### void onAddNode(short pos[])
### void onChatMessage(unsigned char type, std::wstring sender, std::wstring message)
### void onConnect()
### void onDisconnect(std::string reason)

Currently unused(won't be called at all)

### void onTime(unsigned short t)
### void onInventoryUpdate()
### void onPlayerMove(float pos[])

pos -> X, Y, Z

### void run()

The main code of bot is here. Don't forget to use `step()`
regularly here. When the end of this method is reached, bot stops and with `delete`
you can disconnect it.
