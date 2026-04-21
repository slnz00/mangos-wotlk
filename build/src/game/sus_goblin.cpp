#include "AI/ScriptDevAI/include/sc_common.h"

#include "Entities/Player.h"
#include "Globals/ObjectAccessor.h"
#include "Groups/Group.h"

#include <set>

struct SusGoblinAI : public ScriptedAI
{
    std::set<ObjectGuid> m_playersWithMusic;

    SusGoblinAI(Creature* pCreature) : ScriptedAI(pCreature) {}

    void Reset() override
    {
        StopMusicForAllTrackedPlayers();
    }

    void PlayMusicForPlayer(Player* player)
    {
        if (!player)
            return;

        ObjectGuid const& guid = player->GetObjectGuid();

        // Avoid restarting the music for the same player.
        if (m_playersWithMusic.find(guid) != m_playersWithMusic.end())
            return;

        m_playersWithMusic.insert(guid);
        player->PlayMusic(30000);
    }

    void StopMusicForPlayer(Player* player)
    {
        if (!player)
            return;

        player->PlayMusic(0);
    }

    void StopMusicForGuid(ObjectGuid const& guid)
    {
        if (Player* player = ObjectAccessor::FindPlayer(guid))
            StopMusicForPlayer(player);

        m_playersWithMusic.erase(guid);
    }

    void StopMusicForAllTrackedPlayers()
    {
        for (ObjectGuid const& guid : m_playersWithMusic)
        {
            if (Player* player = ObjectAccessor::FindPlayer(guid))
                StopMusicForPlayer(player);
        }

        m_playersWithMusic.clear();
    }

    void KilledUnit(Unit* victim) override
    {
        // Instant stop if this creature directly kills a player.
        if (!victim || victim->GetTypeId() != TYPEID_PLAYER)
            return;

        Player* player = static_cast<Player*>(victim);
        StopMusicForGuid(player->GetObjectGuid());
    }

    void JustDied(Unit* /*killer*/) override
    {
        // Creature died: stop music for everyone who received it.
        StopMusicForAllTrackedPlayers();
    }

    void EnterEvadeMode() override
    {
        // Creature left combat / evaded / returned home:
        // stop music before base AI Reset() clears your state.
        StopMusicForAllTrackedPlayers();

        ScriptedAI::EnterEvadeMode();
    }

    void StopMusicForDeadTrackedPlayers()
    {
        for (std::set<ObjectGuid>::iterator itr = m_playersWithMusic.begin(); itr != m_playersWithMusic.end();)
        {
            Player* player = ObjectAccessor::FindPlayer(*itr);

            // Player logged out / not found. Cannot send stop packet, just forget them.
            if (!player)
            {
                m_playersWithMusic.erase(itr++);
                continue;
            }

            if (!player->IsAlive())
            {
                player->PlayMusic(0);
                m_playersWithMusic.erase(itr++);
                continue;
            }

            ++itr;
        }
    }

    void PlayMusicForAttackerAndGroup(Unit* attacker)
    {
        if (!attacker)
            return;

        Player* attackerPlayer = attacker->GetBeneficiaryPlayer();
        if (!attackerPlayer)
            return;

        Group* group = attackerPlayer->GetGroup();

        if (!group)
        {
            PlayMusicForPlayer(attackerPlayer);
            return;
        }

        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->getSource();
            if (!member)
                continue;

            PlayMusicForPlayer(member);
        }
    }

    void Aggro(Unit* who) override
    {
        PlayMusicForAttackerAndGroup(who);
    }

    void UpdateAI(const uint32 /*diff*/) override
    {
        StopMusicForDeadTrackedPlayers();

        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

void AddSC_sus_goblin()
{
    Script* script = new Script;
    script->Name = "sus_goblin";
    script->GetAI = [](Creature* pCreature) {
        return (UnitAI*)new SusGoblinAI(pCreature);
    };
    script->RegisterSelf();
}
