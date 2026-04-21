#include "AI/ScriptDevAI/include/sc_common.h"

#include <vector>
#include <functional>
#include <cstdint>
#include <Globals/ObjectMgr.h>

struct EventController
{
    struct Event
    {
        uint64_t delay;
        std::function<void()> action;
    };

    uint64_t elapsed;
    size_t currentIndex;
    std::vector<Event> events;
    bool done = false;

    EventController()
    {
        elapsed = 0;
        currentIndex = 0;
    }

    void AddEvent(uint64_t delay, std::function<void()> action)
    {
        events.push_back({delay, action});

        if (delay == 0) {
            Update(0);
        }
    }

    void AddDelay(uint64_t delay)
    {
        if (delay == 0)
        {
            return;
        }

        events.push_back({delay, nullptr});
    }

    void Update(const uint32 diff)
    {
        if (currentIndex >= events.size())
        {
            if (!done)
            {
                done = true;
            }

            return;
        }

        const Event& currentEvent = events[currentIndex];
        elapsed += diff;

        if (elapsed >= currentEvent.delay)
        {
            if (currentEvent.action) {
                currentEvent.action();
            }
            elapsed -= currentEvent.delay;
            currentIndex++;
        }
    }
};

struct PetyaAI : public ScriptedAI
{
    EventController* events = nullptr;

    PetyaAI(Creature* pCreature) : ScriptedAI(pCreature) {}

    ~PetyaAI()
    {
        if (events)
        {
            delete events;
        }
    }

    void StartEvent(Player* player)
    {
        if (events && !events->done)
        {
            return;
        }

        if (events)
        {
            delete events;
        }
        events = new EventController();

        ObjectGuid playerGuid = player->GetObjectGuid();

        auto flagsFirst = std::vector<uint32> {
            9000555,
            9000557,
            9000559
        };

        auto flagsBack = std::vector<uint32> {
            9000537,
            9000538,
            9000547,
            9000548,
            9000549,
            9000550,
            9000551,
            9000552,
            9000553,
            9000554,
        };

        auto flagPairsBridge1 = std::vector<std::pair<uint32, uint32>> {
            {9000526, 9000535},
            {9000527, 9000534},
            {9000528, 9000533},
            {9000529, 9000532},
            {9000530, 9000531},
        };
        auto flagPairsBridge2 = std::vector<std::pair<uint32, uint32>> {
            {9000521, 9000520},
            {9000522, 9000519},
            {9000523, 9000518},
            {9000524, 9000517},
            {9000525, 9000516},
        };
        auto flagPairsEntrance = std::vector<std::pair<uint32, uint32>> {
            {9000539, 9000540},
        };
        auto originalOrientation = m_creature->GetOrientation();

        // Play speech + music:
        events->AddEvent(0, [this, playerGuid]() {
            if (Player* player = m_creature->GetMap()->GetPlayer(playerGuid))
            {
                player->PlayMusic(20000);
            }
        });

        // Speech emote:
        events->AddEvent(2000, [this]() {
            m_creature->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_TALK);
        });
        events->AddEvent(38500, [this]() {
            m_creature->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
        });

        // Remove chains, levitate:
        events->AddEvent(1000, [this]() {
            // Remove chains:
            DeleteObjects({9000579, 9000578, 9000577, 9000574});

            // Levitate:
            m_creature->CastSpell(m_creature, 1706, TRIGGERED_OLD_TRIGGERED);
            m_creature->SetLevitate(true);
            m_creature->SetHover(true);


            // Explosion:
            m_creature->CastSpell(m_creature, 46402, TRIGGERED_OLD_TRIGGERED);
        });

        events->AddEvent(1500, [this, flagsFirst]() {
            float x = m_creature->GetPositionX();
            float y = m_creature->GetPositionY();
            float z = m_creature->GetPositionZ();

            m_creature->GetMotionMaster()->MoveJump(x, y, z + 10.0f, 0.75f, 0.5f, 1, true);
        });

        events->AddEvent(3000, [this, flagsFirst]() {
            DeleteObjects(flagsFirst);
        });

        events->AddEvent(4000, [this, originalOrientation]() {
            m_creature->GetMotionMaster()->Clear();
            m_creature->SetLevitate(true);
            
            float z = m_creature->GetPositionZ();

            m_creature->GetMotionMaster()->MoveJump(-9073.0, 425.0f, z, 3.0f, 0.5f, 1, true);
        });


        events->AddEvent(7000, [this, flagsBack]() {
            DeleteObjects(flagsBack);
        });

        events->AddDelay(6750);
        for (auto flagPairs : flagPairsBridge1) {
            events->AddEvent(1250, [this, flagPairs]() {
                DeleteObjects({ flagPairs.first, flagPairs.second });
            });
        }

        events->AddDelay(4000);
        for (auto flagPairs : flagPairsBridge2)
        {
            events->AddEvent(1000, [this, flagPairs]() {
                DeleteObjects({flagPairs.first, flagPairs.second});
            });
        }

        events->AddEvent(2000, [this]() {
            DeleteObjects({ 9000539, 9000540 });
        });

        events->AddEvent(1500, [this]() {
            m_creature->AddObjectToRemoveList();
        });
        
    }

    void DeleteObjects(const std::vector<uint32>& guids)
    {
        auto map = m_creature->GetMap();
        if (!map)
        {
            return;
        }

        for (uint32 guid : guids)
        {
            if (GameObjectData const* go_data = sObjectMgr.GetGOData(guid))
            {
                auto obj = map->GetGameObject(ObjectGuid(HIGHGUID_GAMEOBJECT, go_data->id, guid));

                if (obj)
                {
                    // TODO: Should add delete from db after done with testing
                    obj->Delete();
                }
            }
        }
    }

    void StopEvent()
    {
        if (events)
        {
            delete events;
        }
        events = nullptr;
    }

    void UpdateAI(const uint32 diff) override
    {
        if (!events || events->done) {
            return;
        }

        events->Update(diff);
    }
};

bool GossipHello_petya(Player* pPlayer, Creature* pCreature)
{
    pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Test", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

    pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetObjectGuid());
    return true;
}

bool GossipSelect_petya(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction)
{
    pPlayer->CLOSE_GOSSIP_MENU();
    
    if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
    {
        if (PetyaAI* pAI = dynamic_cast<PetyaAI*>(pCreature->AI()))
        {
            pAI->StartEvent(pPlayer);
        }
    }

    return true;
}

void AddSC_petya()
{
    Script* script = new Script;
    script->Name = "petya";
    script->GetAI = [](Creature* pCreature) {
        return (UnitAI*)new PetyaAI(pCreature);
    };
    script->pGossipHello = &GossipHello_petya;
    script->pGossipSelect = &GossipSelect_petya;
    script->RegisterSelf();
}