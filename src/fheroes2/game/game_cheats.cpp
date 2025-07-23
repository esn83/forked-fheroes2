/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2025                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "game_cheats.h"

#include <SDL2/SDL.h>

#include "army_troop.h"
#include "castle.h"
#include "game_interface.h"
#include "game_over.h"
#include "heroes.h"
#include "interface_base.h"
#include "interface_gamearea.h"
#include "kingdom.h"
#include "logging.h"
#include "monster.h"
#include "resource.h"
#include "settings.h"
#include "spell.h"
#include "spell_storage.h"
#include "world.h"

#include <unordered_set>

namespace GameCheats
{
    std::string buffer;

    // master switch for cheats
    // logic can be implemented around it later if it needs to be switched on/off
    // is on by default as the original homm2.
    bool cheatsEnabled = true;
    
    bool redrawHeroesDialogFlag = false;

    const size_t MAX_BUFFER = 32;

    const std::vector<std::string> cheatCodes = {
    "11111",    // Add resources
    "8675309",  // Reveal all fog
    "32167",    // Add 5 black dragons
    "44444",    // Give upgraded army
    "55555",    // Add random troop
    "66666",    // Max primary skills
    "77777",    // Max secondary skills
    "88888",    // Big movement bonus
    "99999"     // All spells + max spell points
    };

    std::unordered_set<uint8_t> cheatingColors; // list of cheaters colors
    std::unordered_set<uint8_t> winningColors; // list of human player colors

    void checkBuffer()
    {
    
        const Settings & conf = Settings::Get();
        const PlayerColor currentColor = conf.CurrentColor();
        Heroes * hero = Interface::GetFocusHeroes();
        Kingdom & kingdom = World::Get().GetKingdom(currentColor);

        // If cheats are not enabled, don't proceed with cheat codes
        if ( !areCheatsEnabled() )
        {
            return;
        }

        // Detect cheat use
        for ( const std::string& code : cheatCodes )
        {
            if ( buffer.find(code) != std::string::npos )
            {
                cheatingColors.insert(static_cast<uint8_t>(currentColor));
                break;
            }
        }

        // Execute known cheat codes
        struct CheatAction
        {
            const char* code;
            std::function<void()> action;
        };

        const std::vector<CheatAction> cheats = {
            { "11111", [&]() {
                DEBUG_LOG(DBG_GAME, DBG_INFO, "Cheat activated: resources");
                kingdom.AddFundsResource(Funds(0, 0, 0, 0, 0, 0, 50000));
                kingdom.AddFundsResource(Funds(Resource::WOOD, 50));
                kingdom.AddFundsResource(Funds(Resource::ORE, 50));
                kingdom.AddFundsResource(Funds(Resource::MERCURY, 50));
                kingdom.AddFundsResource(Funds(Resource::SULFUR, 50));
                kingdom.AddFundsResource(Funds(Resource::CRYSTAL, 50));
                kingdom.AddFundsResource(Funds(Resource::GEMS, 50));
                Interface::AdventureMap::Get().redraw(Interface::REDRAW_STATUS);
            }},
            { "8675309", [&]() {
                DEBUG_LOG(DBG_GAME, DBG_INFO, "Cheat activated: reveal all fog");
                PlayerColorsSet colors_set = Players::GetPlayerFriends(currentColor);
                for (size_t i = 0; i < world.getSize(); ++i) {
                    world.getTile(static_cast<int32_t>(i)).ClearFog(colors_set);
                }
                Interface::GameArea::updateMapFogDirections();
                Interface::AdventureMap::Get().redraw(Interface::REDRAW_RADAR);
            }},
            { "32167", [&]() {
                DEBUG_LOG(DBG_GAME, DBG_INFO, "Cheat activated: black dragons");
                if (hero) {
                    hero->GetArmy().JoinTroop(Monster::BLACK_DRAGON, 5, true);
                    Interface::AdventureMap::Get().redraw(Interface::REDRAW_STATUS);
                    redrawHeroesDialogFlag = true;
                }
            }},
            { "44444", [&]() {
                DEBUG_LOG(DBG_GAME, DBG_INFO, "Cheat activated: upgraded army");
                if (hero) {
                    const int race = hero->GetRace();
                    Army & army = hero->GetArmy();

                    // Base dwellings: tier 2–6
                    std::vector<uint32_t> dwellings = {
                        DWELLING_UPGRADE2,
                        DWELLING_UPGRADE3,
                        DWELLING_UPGRADE4,
                        DWELLING_UPGRADE5,
                        DWELLING_UPGRADE6
                    };

                    // Special case for Warlock: include tier 7 instead of 6
                    if (race == Race::WRLK) {
                        dwellings.pop_back(); // remove tier 6
                        dwellings.push_back(DWELLING_UPGRADE7); // add tier 7
                    }

                    std::unordered_set<uint32_t> allowedIDs;
                    for (const uint32_t dw : dwellings) {
                        Monster m(race, dw);
                        if (m.isValid())
                            allowedIDs.insert(m.GetID());
                    }

                    // Remove any non-upgraded troops
                    for (size_t i = 0; i < Army::maximumTroopCount; ++i) {
                        Troop* troop = army.GetTroop(i);
                        if (troop && troop->isValid()) {
                            if (allowedIDs.find(troop->GetMonster().GetID()) == allowedIDs.end()) {
                                troop->Reset();
                            }
                        }
                    }

                    // Add or top-up each upgraded troop with their actual weekly growth amount
                    for (const uint32_t dw : dwellings) {
                        Monster m(race, dw);
                        if (!m.isValid())
                            continue;

                        const uint32_t growth = m.GetGrown() + 2;  // Use actual weekly growth + 2 from well

                        bool found = false;
                        for (size_t i = 0; i < Army::maximumTroopCount; ++i) {
                            Troop* troop = army.GetTroop(i);
                            if (troop && troop->isValid() && troop->GetMonster().GetID() == m.GetID()) {
                                troop->SetCount(troop->GetCount() + growth);
                                found = true;
                                break;
                            }
                        }

                        if (!found) {
                            army.JoinTroop(m, growth, true);
                        }
                    }

                    Interface::AdventureMap::Get().redraw(Interface::REDRAW_STATUS);
                    redrawHeroesDialogFlag = true;
                }
            }},
            { "55555", [&]() {
                DEBUG_LOG(DBG_GAME, DBG_INFO, "Cheat activated: random troop");
                if (hero) {
                    Monster monster = Monster::Rand(Monster::LevelType::LEVEL_ANY);
                    hero->GetArmy().JoinTroop(monster, 5, true);
                    Interface::AdventureMap::Get().redraw(Interface::REDRAW_STATUS);
                    redrawHeroesDialogFlag = true;
                }
            }},
            { "66666", [&]() {
                DEBUG_LOG(DBG_GAME, DBG_INFO, "Cheat activated: max primary skills");
                if (hero) {
                    hero->setAttackBaseValue(99);
                    hero->setDefenseBaseValue(99);
                    hero->setPowerBaseValue(99);
                    hero->setKnowledgeBaseValue(99);
                    redrawHeroesDialogFlag = true;
                }
            }},
            { "77777", [&]() {
                /*{
                UNKNOWN = 0,
                PATHFINDING = 1,
                ARCHERY = 2,
                LOGISTICS = 3,
                SCOUTING = 4,
                DIPLOMACY = 5,
                NAVIGATION = 6,
                LEADERSHIP = 7,
                WISDOM = 8,
                MYSTICISM = 9,
                LUCK = 10,
                BALLISTICS = 11,
                EAGLE_EYE = 12,
                NECROMANCY = 13,
                ESTATES = 14
                };*/

                int default_secondary_skills[8] = {3, 1, 2, 5, 7, 10, 8, 11};
                int necro_secondary_skills[8] = {13, 1, 2, 5, 11, 10, 8, 3};
                DEBUG_LOG(DBG_GAME, DBG_INFO, "Cheat activated: max secondary skills");
                if (hero) {
                    // Clear all learned secondary skills manually
                    hero->ClearAllSecondarySkills();

                    const int race = hero->GetRace();
                    const int* selected_skills = (race == Race::NECR) ? necro_secondary_skills : default_secondary_skills;

                    // Add selected skills at EXPERT level
                    for (int i = 0; i < 8; ++i) {
                        hero->LearnSkill(Skill::Secondary(selected_skills[i], Skill::Level::EXPERT));
                    }

                    redrawHeroesDialogFlag = true;
                }
            }},
            { "88888", [&]() {
                DEBUG_LOG(DBG_GAME, DBG_INFO, "Cheat activated: big movement bonus");
                if (hero) {
                    hero->IncreaseMovePoints(hero->GetMaxMovePoints() * 50);
                }
            }},
            { "99999", [&]() {
                DEBUG_LOG(DBG_GAME, DBG_INFO, "Cheat activated: all spells");
                if (hero) {
                    if (!hero->HaveSpellBook()) {
                        Artifact magicBook(Artifact::MAGIC_BOOK);
                        hero->GetBagArtifacts().PushArtifact(magicBook);
                    }

                    SpellStorage storage;
                    for (int spellId : Spell::getAllSpellIdsSuitableForSpellBook()) {
                        storage.Append(Spell(spellId));
                    }
                    hero->AppendSpellsToBook(storage, true);
                    hero->SetSpellPoints(hero->GetMaxSpellPoints());
                    redrawHeroesDialogFlag = true;
                }
            }},
        };

        for (const auto& cheat : cheats)
        {
            if ( buffer.find(cheat.code) != std::string::npos )
            {
                cheat.action();
                buffer.clear();
                return; // Only one cheat per buffer processing
            }
        }
    }

    void enableCheats( bool enable )
    {
        cheatsEnabled = enable;
        if ( !enable )
            buffer.clear();
    }

    bool areCheatsEnabled()
    {
        return cheatsEnabled;
    }

    bool isPlayersCheating()
    {
        for (uint8_t color : winningColors)
        {
            if ( cheatingColors.count(color) )
                return true;
        }
        return false;
    }

    bool redrawHeroesDialog()
    {
        if ( redrawHeroesDialogFlag )
        {
            redrawHeroesDialogFlag = false;
            return true;
        }
        return false;
    }

    void gameCheatsReset()
    {
        buffer.clear();
        cheatingColors.clear();
        winningColors.clear();
    }

    void onKeyPressed(const fheroes2::Key key, const int32_t modifier)
    {
        if ( SDL_IsTextInputActive() )
        {
            return;
        }
        else
        {
            std::string tmp;
            if ( fheroes2::InsertKeySym(tmp, 0, key, modifier ) == 0 || tmp.empty())
            {
                return;
            }

            buffer += tmp;
            if ( buffer.size() > MAX_BUFFER )
            {
                buffer.erase(0, buffer.size() - MAX_BUFFER);
            }

            checkBuffer();
        }
    }
}