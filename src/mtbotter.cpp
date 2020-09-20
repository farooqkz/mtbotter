#include <iostream>
#include "mtbotter.h"
using namespace std;
using namespace con;


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
    m_hit = 0.0f;
}
list<string> MtBotter::getPlayerNames() {
    return m_client->getEnv().getPlayerNames();
}

unsigned short MtBotter::getHP() {
    return m_client->getHP();
}

PThing MtBotter::getPThing() {
    PointedThing pt = getPointedThing();
    PThing pt_;
    v3s16 vs0, vs1;
    vs0 = pt.node_undersurface;
    vs1 = pt.node_abovesurface;

    pt_.type = (PThingType) pt.type;
    pt_.node_undersurface[0] = vs0.X;
    pt_.node_undersurface[1] = vs0.Y;
    pt_.node_undersurface[2] = vs0.Z;
    pt_.node_abovesurface[0] = vs1.X;
    pt_.node_abovesurface[1] = vs1.Y;
    pt_.node_abovesurface[2] = vs1.Z;
    return pt_;
}

SomeNode MtBotter::getNode(short x, short y, short z) {
    ClientMap &map = m_client->getEnv().getClientMap();
    bool valid;
    MapNode node = map.getNode(v3s16(x, y, z), &valid);
    SomeNode result;
    ContentFeatures cf;
    cf = m_nodemgr->get(node);
    result.name = cf.name;
    result.is_ground_content = cf.is_ground_content;
    result.walkable = cf.walkable;
    result.pointable = cf.pointable;
    result.diggable = cf.diggable;
    result.climbable = cf.climbable;
    result.buildable_to = cf.buildable_to;
    result.rightclickable = cf.rightclickable;
    result.dmg_p_sec = cf.damage_per_second;
    return result;
}

list<SomeObject> MtBotter::getNearestObjects(float max_d) {
    vector<DistanceSortedActiveObject> objs;
    v3f player_pos = v3f();
    player_pos.X = getPosX();
    player_pos.Y = getPosY();
    player_pos.Z = getPosZ();
    list<SomeObject> result;

    m_client->getEnv().getActiveObjects(player_pos,max_d,objs);

    list<DistanceSortedActiveObject> objs_list;
    for (auto obj : objs) {
        objs_list.push_front(obj);
    }
    objs_list.sort();
    SomeObject obj2add;
    GenericCAO *gobj;
    u16 id;
    aabb3f a;
    bool ab;
    for (auto obj : objs_list) {
        id = obj.obj->getId();
        gobj = m_client->getEnv().getGenericCAO(id);
        ab = gobj->getSelectionBox(&a);
        if (ab) {
            irr::core::vector3d<float> edges[8];
            a.getEdges(edges);
            obj2add.width = abs((edges[0] - edges[4]).X);
            obj2add.height = abs((edges[0] - edges[1]).Y);
        } else {
            obj2add.height = obj2add.width = 0;
        }
        obj2add.localplayer = gobj->isLocalPlayer();
        obj2add.groups = gobj->getGroups();
        obj2add.immortal = gobj->isImmortal();
        obj2add.posX = gobj->getPosition().X;
        obj2add.posY = gobj->getPosition().Y;
        obj2add.posZ = gobj->getPosition().Z;
        obj2add.id = id;

        result.push_front(obj2add);
    }
    return result;
}

unsigned short MtBotter::getBreath() {
    return m_client->getEnv().getLocalPlayer()->getBreath();
}

bool MtBotter::isDead() {
    return m_client->getEnv().getLocalPlayer()->isDead();
}

bool MtBotter::getNearestObject(float max_d, SomeObject &obj) {
    list<SomeObject> objs = getNearestObjects(max_d);
    if (objs.size() < 2) {
        return false;
    }
    unsigned i = 1;
    for (auto o : objs) {
        if (i++ == 2) {
            obj = o;
            return true;
        }
    }
    return false;
}


void MtBotter::setWieldIndex(unsigned short index) {
    m_client->getEnv().getLocalPlayer()->setWieldIndex(index);
}

unsigned short MtBotter::getWieldIndex() {
    return m_client->getEnv().getLocalPlayer()->getWieldIndex();
}

SomeItemStack MtBotter::getWieldedItem() {
    ItemStack result, selected, hand;
    LocalPlayer *player = m_client->getEnv().getLocalPlayer();
    result = player->getWieldedItem(&selected, &hand);
    SomeItemStack sis;
    sis.name = result.name;
    sis.count = result.count;
    sis.wear = result.wear;
    return sis;
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
    onConnect();
    return true;
}

PointedThing MtBotter::getPointedThing() {
    LocalPlayer *player = m_client->getEnv().getLocalPlayer();
    
    // https://stackoverflow.com/questions/10569659/camera-pitch-yaw-to-direction-vector
    float yaw = getHeadingH();
    yaw = yaw * M_PI / 180; // in radians
    float pitch = getHeadingV();
    pitch = pitch * M_PI / 180; // in radians
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

bool MtBotter::punch() {
    if (!m_client->checkPrivilege("interact")) {
        return false;
    }
    PointedThing pt = getPointedThing();
    switch (pt.type) {
        case POINTEDTHING_NOTHING:
            return false;
        case POINTEDTHING_NODE:
            m_client->interact(INTERACT_START_DIGGING, pt);
            return true;
        case POINTEDTHING_OBJECT:
            ClientActiveObject *selected;
            selected = m_client->getEnv().getActiveObject(pt.object_id);
            ItemStack tool_item, selected_item, hand_item;
            LocalPlayer *player = m_client->getEnv().getLocalPlayer();
            tool_item = player->getWieldedItem(&selected_item, &hand_item);
            v3f player_pos = player->getPosition();
            v3f obj_pos = selected->getPosition();
            v3f dir = (obj_pos - player_pos).normalize();
            bool disable_send = selected->directReportPunch(dir, &tool_item, 0);

            if (!disable_send)
                m_client->interact(INTERACT_START_DIGGING, pt);
            return true;
    }
    return false;
}

bool MtBotter::activate() {
    if (!m_client->checkPrivilege("interact")) {
        return false;
    }
    PointedThing pt;
    pt.type = POINTEDTHING_NOTHING;
    m_client->interact(INTERACT_ACTIVATE, pt);
    return true;
}

bool MtBotter::place(bool noplace) {
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
    auto &selected_def = selected_item.getDefinition(m_itemdef);
    
    MapNode node;
    bool is_valid_pos;
    v3s16 nodepos = pt.node_undersurface;
    v3s16 p = pt.node_abovesurface;
    node = map.getNode(nodepos, &is_valid_pos);

    if (!is_valid_pos)
        return false;
    
    MapNode p_n = map.getNode(p, &is_valid_pos);
    if (!is_valid_pos) return false;

    if (m_nodemgr->get(node).rightclickable) {
        cout << m_nodemgr->get(node).name << endl;
        if (noplace) {
            m_client->interact(INTERACT_PLACE, pt);
            return true;
        } else {
            return false;
        }
    }
    
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

    string prediction = selected_def.node_placement_prediction;
    content_t id;
    bool found = m_nodemgr->getId(prediction, id);
    if (!found) {
        m_client->interact(INTERACT_PLACE, pt);
        return false;
    }
    const ContentFeatures &predicted_f = m_nodemgr->get(id);
    u8 param2 = 0;

	if (predicted_f.param_type_2 == CPT2_WALLMOUNTED ||
			predicted_f.param_type_2 == CPT2_COLORED_WALLMOUNTED) {
		v3s16 dir = nodepos - pt.node_abovesurface;

		if (abs(dir.Y) > MYMAX(abs(dir.X), abs(dir.Z))) {
			param2 = dir.Y < 0 ? 1 : 0;
		} else if (abs(dir.X) > abs(dir.Z)) {
			param2 = dir.X < 0 ? 3 : 2;
		} else {
			param2 = dir.Z < 0 ? 5 : 4;
		}
	}

	if (predicted_f.param_type_2 == CPT2_FACEDIR ||
			predicted_f.param_type_2 == CPT2_COLORED_FACEDIR) {
		v3s16 dir = nodepos - floatToInt(m_client->getEnv().getLocalPlayer()->getPosition(), BS);

		if (abs(dir.X) > abs(dir.Z)) {
			param2 = dir.X < 0 ? 3 : 1;
		} else {
			param2 = dir.Z < 0 ? 2 : 0;
		}
	}

    assert(param2 <= 5);

    //Check attachment if node is in group attached_node
	if (((ItemGroupList) predicted_f.groups)["attached_node"] != 0) {
		static v3s16 wallmounted_dirs[8] = {
			v3s16(0, 1, 0),
			v3s16(0, -1, 0),
			v3s16(1, 0, 0),
			v3s16(-1, 0, 0),
			v3s16(0, 0, 1),
			v3s16(0, 0, -1),
		};
		v3s16 pp;

		if (predicted_f.param_type_2 == CPT2_WALLMOUNTED ||
				predicted_f.param_type_2 == CPT2_COLORED_WALLMOUNTED)
			pp = p + wallmounted_dirs[param2];
		else
			pp = p + v3s16(0, -1, 0);

		if (!m_nodemgr->get(map.getNode(pp)).walkable) {
			// Report to server
			m_client->interact(INTERACT_PLACE, pt);
			return false;
		}
	}

	// Apply color
	if ((predicted_f.param_type_2 == CPT2_COLOR
			|| predicted_f.param_type_2 == CPT2_COLORED_FACEDIR
			|| predicted_f.param_type_2 == CPT2_COLORED_WALLMOUNTED)) {
		const std::string &indexstr = selected_item.metadata.getString(
			"palette_index", 0);
		if (!indexstr.empty()) {
			s32 index = mystoi(indexstr);
			if (predicted_f.param_type_2 == CPT2_COLOR) {
				param2 = index;
			} else if (predicted_f.param_type_2 == CPT2_COLORED_WALLMOUNTED) {
				// param2 = pure palette index + other
				param2 = (index & 0xf8) | (param2 & 0x07);
			} else if (predicted_f.param_type_2 == CPT2_COLORED_FACEDIR) {
				// param2 = pure palette index + other
				param2 = (index & 0xe0) | (param2 & 0x1f);
			}
		}
	}
    
    MapNode n(id, 0, param2);
    m_client->addNode(p, n);
    m_client->interact(INTERACT_PLACE, pt);
    return true;
}

bool MtBotter::place() {
    return place(false);
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
    NetworkPacket pkt;
    v3s16 pos;
    v3f posf;
    short pos_arr[3];
    float posf_arr[3];
    unsigned char type, ver;
    unsigned short l1, l2;
    wstring sender, message;
    while (m_client->getEvent(pkt)) {
        switch (pkt.getCommand()) {
            case TOCLIENT_ADDNODE:
                pkt >> pos;
                pos_arr[0] = pos.X;
                pos_arr[1] = pos.Y;
                pos_arr[2] = pos.Z;
                onAddNode(pos_arr);
                break;
            case TOCLIENT_REMOVENODE:
                pkt >> pos;
                pos_arr[0] = pos.X;
                pos_arr[1] = pos.Y;
                pos_arr[2] = pos.Z;
                onRemoveNode(pos_arr);
                break;
            case TOCLIENT_TIME_OF_DAY:
                unsigned short t;
                pkt >> t;
                onTime(t);
                break;
            case TOCLIENT_CHAT_MESSAGE:
                pkt >> ver >>
                        type >>
                        l1 >>
                        sender >>
                        l2 >>
                        message;
                onChatMessage(type, sender, message);
                break;
            case TOCLIENT_MOVE_PLAYER:
                pkt >> posf;
                posf_arr[0] = posf.X; 
                posf_arr[1] = posf.Y; 
                posf_arr[2] = posf.Z; 
                onPlayerMove(posf_arr);
                break;
        }
    }
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
    return m_client->getEnv().getLocalPlayer()->getStandingNodePos().Y + 1;
}

short MtBotter::getPosZ() {
    return m_client->getEnv().getLocalPlayer()->getStandingNodePos().Z;
}

void MtBotter::turnLeft(float deg) {
    PlayerControl control;
    control.yaw = fmod(getHeadingH() + deg, 360);
    m_client->setPlayerControl(control);
}

void MtBotter::turnRight() {
    turnRight(15.0);
}

void MtBotter::turnRight(float deg) {
    PlayerControl control;
    control.yaw = fmod(getHeadingH() - deg, 360);
    m_client->setPlayerControl(control);
}

void MtBotter::turnLeft() {
    turnLeft(15.0);
}

void MtBotter::turnHeadUp(float deg) {
    PlayerControl control;
    control.pitch = getHeadingV() + deg;
    control.yaw = getHeadingH();
    m_client->setPlayerControl(control);
}

void MtBotter::turnHeadUp() {
    turnHeadUp(5.0);
}

void MtBotter::turnHeadDown(float deg) {
    PlayerControl control;
    control.pitch = getHeadingV() - deg;
    control.yaw = getHeadingH();
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

