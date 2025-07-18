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
#pragma once

#include <string>
#include <unordered_set>
#include <cstdint>
#include "serialize.h"

#include "localevent.h"

namespace GameCheats
{
    extern std::unordered_set<uint8_t> cheatingColors;
    extern std::unordered_set<uint8_t> winningColors;

    void enableCheats( bool cheatsEnabled );
    bool areCheatsEnabled();
    bool isPlayersCheating();
    bool redrawHeroesDialog();
    void onKeyPressed( const fheroes2::Key key, const int32_t modifier );
    void gameCheatsReset();
}

// serialization of cheatingColors so it can be added to save games and loaded from load games
inline OStreamBase & operator<<(OStreamBase & stream, const std::unordered_set<uint8_t> & cheatingColors) {
    uint32_t size = static_cast<uint32_t>(cheatingColors.size());
    stream << size;
    for (uint8_t c : cheatingColors)
        stream << c;
    return stream;
}

// serialization of cheatingColors so it can be added to save games and loaded from load games
inline IStreamBase & operator>>(IStreamBase & stream, std::unordered_set<uint8_t> & cheatingColors) {
    uint32_t size = 0;
    stream >> size;
    cheatingColors.clear();
    for (uint32_t i = 0; i < size; ++i) {
        uint8_t c;
        stream >> c;
        cheatingColors.insert(c);
    }
    return stream;
}