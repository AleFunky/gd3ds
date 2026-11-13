#include "main_levels.hpp"
#include "main.hpp"

// level_id property defines the save entry id

namespace gd3ds
{

MainLevelDefinition robtopMainLevels[] = {
    {
        .levelID = 0,
        .levelName = "Stereo Madness",
        .gmdPath = "romfs:/main_levels/StereoMadness.gmd",
        .songPath = "romfs:/songs/StereoMadness.mp3",
        .difficulty = MAIN_DIFF_EASY,
        .stars = 1,
        .songData = {
            .title = "Stereo Madness", 
            .artist = "ForeverBound" 
        }
    },
    {
        .levelID = 1,
        .levelName = "Back On Track",
        .gmdPath = "romfs:/main_levels/BackOnTrack.gmd",
        .songPath = "romfs:/songs/BackOnTrack.mp3",
        .difficulty = MAIN_DIFF_EASY,
        .stars = 2,
        .songData = {
            .title = "Back on Track", 
            .artist = "DJVI" 
        }
    },
    {
        .levelID = 2,
        .levelName = "Polargeist",
        .gmdPath = "romfs:/main_levels/Polargeist.gmd",
        .songPath = "romfs:/songs/Polargeist.mp3",
        .difficulty = MAIN_DIFF_NORMAL,
        .stars = 3,
        .songData = {
            .title = "Polargeist", 
            .artist = "Step" 
        }
    },
    {
        .levelID = 3,
        .levelName = "Dry Out",
        .gmdPath = "romfs:/main_levels/DryOut.gmd",
        .songPath = "romfs:/songs/DryOut.mp3",
        .difficulty = MAIN_DIFF_NORMAL,
        .stars = 4,
        .songData = {
            .title = "Dry Out", 
            .artist = "DJVI" 
        }
    },
    {
        .levelID = 4,
        .levelName = "Base After Base",
        .gmdPath = "romfs:/main_levels/BaseAfterBase.gmd",
        .songPath = "romfs:/songs/BaseAfterBase.mp3",
        .difficulty = MAIN_DIFF_HARD,
        .stars = 5,
        .songData = {
            .title = "Base after Base", 
            .artist = "DJVI" 
        }
    },
    {
        .levelID = 5,
        .levelName = "Cant Let Go",
        .gmdPath = "romfs:/main_levels/CantLetGo.gmd",
        .songPath = "romfs:/songs/CantLetGo.mp3",
        .difficulty = MAIN_DIFF_HARD,
        .stars = 6,
        .songData = {
            .title = "Cant let Go",
            .artist = "DJVI" 
        }
    },
    {
        .levelID = 6,
        .levelName = "Jumper",
        .gmdPath = "romfs:/main_levels/Jumper.gmd",
        .songPath = "romfs:/songs/Jumper.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 7,
        .songData = {
            .title = "Jumper", 
            .artist = "Waterflame"
        }
    },
    {
        .levelID = 7,
        .levelName = "Time Machine",
        .gmdPath = "romfs:/main_levels/TimeMachine.gmd",
        .songPath = "romfs:/songs/TimeMachine.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 8,
        .songData = {
            .title = "Time Machine", 
            .artist = "Waterflame"
        }
    },
    {
        .levelID = 8,
        .levelName = "Cycles",
        .gmdPath = "romfs:/main_levels/Cycles.gmd",
        .songPath = "romfs:/songs/Cycles.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 9,
        .songData = {
            .title = "Cycles",  
            .artist = "DJVI" 
        }
    },
    {
        .levelID = 9,
        .levelName = "xStep",
        .gmdPath = "romfs:/main_levels/xStep.gmd",
        .songPath = "romfs:/songs/xStep.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 10,
        .songData = {
            .title = "xStep", 
            .artist = "DJVI"
        }
    },
    {
        .levelID = 10,
        .levelName = "Clutterfunk",
        .gmdPath = "romfs:/main_levels/Clutterfunk.gmd",
        .songPath = "romfs:/songs/Clutterfunk.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 11,
        .songData = {
            .title = "Clutterfunk", 
            .artist = "Waterflame"
        }
    },
    {
        .levelID = 11,
        .levelName = "Theory of Everything",
        .gmdPath = "romfs:/main_levels/TheoryofEverything.gmd",
        .songPath = "romfs:/songs/TheoryOfEverything.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .songData = {
            .title = "Theory of Everything", 
            .artist = "DJ-Nate"
        }
    },
    {
        .levelID = 12,
        .levelName = "Electroman Adventures",
        .gmdPath = "romfs:/main_levels/Electroman.gmd",
        .songPath = "romfs:/songs/Electroman.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 10,
        .songData = {
            .title = "Electroman Adventures", 
            .artist = "Waterflame"
        }
    },
    {
        .levelID = 13,
        .levelName = "Clubstep",
        .gmdPath = "romfs:/main_levels/Clubstep.gmd",
        .songPath = "romfs:/songs/Clubstep.mp3",
        .difficulty = MAIN_DIFF_DEMON,
        .stars = 14,
        .songData = {
            .title = "Clubstep", 
            .artist = "DJ-Nate"
        }
    },
    {
        .levelID = 14,
        .levelName = "Electrodynamix",
        .gmdPath = "romfs:/main_levels/Electrodynamix.gmd",
        .songPath = "romfs:/songs/Electrodynamix.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .songData = {
            .title = "Electrodynamix", 
            .artist = "DJ-Nate"
        }
    },
    {
        .levelID = 15,
        .levelName = "Hexagon Force",
        .gmdPath = "romfs:/main_levels/HexagonForce.gmd",
        .songPath = "romfs:/songs/HexagonForce.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .songData = {
            .title = "Hexagon Force", 
            .artist = "Waterflame" 
        }
    },
    {
        .levelID = 16,
        .levelName = "Blast Processing",
        .gmdPath = "romfs:/main_levels/BlastProcessing.gmd",
        .songPath = "romfs:/songs/BlastProcessing.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 10,
        .songData = {
            .title = "Blast Processing", 
            .artist = "Waterflame"
        }
    },
    {
        .levelID = 17,
        .levelName = "Theory of Everything 2",
        .gmdPath = "romfs:/main_levels/TheoryofEverything2.gmd",
        .songPath = "romfs:/songs/TheoryOfEverything2.mp3",
        .difficulty = MAIN_DIFF_DEMON,
        .stars = 14,
        .songData = {
            .title = "Theory of Everything 2", 
            .artist = "DJ-Nate"
        }
    },
    {
        .levelID = 18,
        .levelName = "Geometrical Dominator",
        .gmdPath = "romfs:/main_levels/GeometricalDominator.gmd",
        .songPath = "romfs:/songs/GeometricalDominator.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 10,
        .songData = {
            .title = "Geometrical Dominator",
            .artist = "Waterflame"
        }
    },
    {
        .levelID = 19,
        .levelName = "Deadlocked",
        .gmdPath = "romfs:/main_levels/Deadlocked.gmd",
        .songPath = "romfs:/songs/Deadlocked.mp3",
        .difficulty = MAIN_DIFF_DEMON,
        .stars = 15,
        .songData = {
            .title = "Deadlocked",
            .artist = "F-777"
        }
    },
    {
        .levelID = 20,
        .levelName = "Fingerdash",
        .gmdPath = "romfs:/main_levels/Fingerdash.gmd",
        .songPath = "romfs:/songs/Fingerdash.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .songData = {
            .title = "Fingerdash",
            .artist = "MDK"
        }
    }
};


MainLevelDefinition gdpsMainLevels[] = {
    {
        .levelID = 0,
        .levelName = "Stereo Madness",
        .gmdPath = "romfs:/main_levels/StereoMadness.gmd",
        .songPath = "romfs:/songs/StereoMadness.mp3",
        .difficulty = MAIN_DIFF_EASY,
        .stars = 1,
        .songData = {
            .title = "Stereo Madness", 
            .artist = "ForeverBound" 
        }
    },
    {
        .levelID = 1,
        .levelName = "Back On Track",
        .gmdPath = "romfs:/main_levels/BackOnTrack.gmd",
        .songPath = "romfs:/songs/BackOnTrack.mp3",
        .difficulty = MAIN_DIFF_EASY,
        .stars = 2,
        .songData = {
            .title = "Back on Track", 
            .artist = "DJVI" 
        }
    },
    {
        .levelID = 2,
        .levelName = "Polargeist",
        .gmdPath = "romfs:/main_levels/Polargeist.gmd",
        .songPath = "romfs:/songs/Polargeist.mp3",
        .difficulty = MAIN_DIFF_NORMAL,
        .stars = 3,
        .songData = {
            .title = "Polargeist", 
            .artist = "Step" 
        }
    },
    {
        .levelID = 3,
        .levelName = "Dry Out",
        .gmdPath = "romfs:/main_levels/DryOut.gmd",
        .songPath = "romfs:/songs/DryOut.mp3",
        .difficulty = MAIN_DIFF_NORMAL,
        .stars = 4,
        .songData = {
            .title = "Dry Out", 
            .artist = "DJVI" 
        }
    },
    {
        .levelID = 4,
        .levelName = "Base After Base",
        .gmdPath = "romfs:/main_levels/BaseAfterBase.gmd",
        .songPath = "romfs:/songs/BaseAfterBase.mp3",
        .difficulty = MAIN_DIFF_HARD,
        .stars = 5,
        .songData = {
            .title = "Base after Base", 
            .artist = "DJVI" 
        }
    },
    {
        .levelID = 5,
        .levelName = "Cant Let Go",
        .gmdPath = "romfs:/main_levels/CantLetGo.gmd",
        .songPath = "romfs:/songs/CantLetGo.mp3",
        .difficulty = MAIN_DIFF_HARD,
        .stars = 6,
        .songData = {
            .title = "Cant let Go",
            .artist = "DJVI" 
        }
    },
    {
        .levelID = 6,
        .levelName = "Jumper",
        .gmdPath = "romfs:/main_levels/Jumper.gmd",
        .songPath = "romfs:/songs/Jumper.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 7,
        .songData = {
            .title = "Jumper", 
            .artist = "Waterflame"
        }
    },
    {
        .levelID = 7,
        .levelName = "Time Machine",
        .gmdPath = "romfs:/main_levels/TimeMachine.gmd",
        .songPath = "romfs:/songs/TimeMachine.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 8,
        .songData = {
            .title = "Time Machine", 
            .artist = "Waterflame"
        }
    },
    {
        .levelID = 8,
        .levelName = "Cycles",
        .gmdPath = "romfs:/main_levels/Cycles.gmd",
        .songPath = "romfs:/songs/Cycles.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 9,
        .songData = {
            .title = "Cycles",  
            .artist = "DJVI" 
        }
    },
    {
        .levelID = 9,
        .levelName = "xStep",
        .gmdPath = "romfs:/main_levels/xStep.gmd",
        .songPath = "romfs:/songs/xStep.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 10,
        .songData = {
            .title = "xStep", 
            .artist = "DJVI"
        }
    },
    {
        .levelID = 10,
        .levelName = "Clutterfunk",
        .gmdPath = "romfs:/main_levels/Clutterfunk.gmd",
        .songPath = "romfs:/songs/Clutterfunk.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 11,
        .songData = {
            .title = "Clutterfunk", 
            .artist = "Waterflame"
        }
    },
    {
        .levelID = 11,
        .levelName = "Theory of Everything",
        .gmdPath = "romfs:/main_levels/TheoryofEverything.gmd",
        .songPath = "romfs:/songs/TheoryOfEverything.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .songData = {
            .title = "Theory of Everything", 
            .artist = "DJ-Nate"
        }
    },
    {
        .levelID = 12,
        .levelName = "Electroman Adventures",
        .gmdPath = "romfs:/main_levels/Electroman.gmd",
        .songPath = "romfs:/songs/Electroman.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 10,
        .songData = {
            .title = "Electroman Adventures", 
            .artist = "Waterflame"
        }
    },
    {
        .levelID = 13,
        .levelName = "Clubstep",
        .gmdPath = "romfs:/main_levels/Clubstep.gmd",
        .songPath = "romfs:/songs/Clubstep.mp3",
        .difficulty = MAIN_DIFF_DEMON,
        .stars = 14,
        .songData = {
            .title = "Clubstep", 
            .artist = "DJ-Nate"
        }
    },
    {
        .levelID = 14,
        .levelName = "Electrodynamix",
        .gmdPath = "romfs:/main_levels/Electrodynamix.gmd",
        .songPath = "romfs:/songs/Electrodynamix.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .songData = {
            .title = "Electrodynamix", 
            .artist = "DJ-Nate"
        }
    },
    {
        .levelID = 15,
        .levelName = "Hexagon Force",
        .gmdPath = "romfs:/main_levels/HexagonForce.gmd",
        .songPath = "romfs:/songs/HexagonForce.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .songData = {
            .title = "Hexagon Force", 
            .artist = "Waterflame" 
        }
    },
    {
        .levelID = 16,
        .levelName = "Blast Processing",
        .gmdPath = "romfs:/main_levels/BlastProcessing.gmd",
        .songPath = "romfs:/songs/BlastProcessing.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 10,
        .songData = {
            .title = "Blast Processing", 
            .artist = "Waterflame"
        }
    },
    {
        .levelID = 17,
        .levelName = "Theory of Everything 2",
        .gmdPath = "romfs:/main_levels/TheoryofEverything2.gmd",
        .songPath = "romfs:/songs/TheoryOfEverything2.mp3",
        .difficulty = MAIN_DIFF_DEMON,
        .stars = 14,
        .songData = {
            .title = "Theory of Everything 2", 
            .artist = "DJ-Nate"
        }
    },
    {
        .levelID = 18,
        .levelName = "Thumper",
        .gmdPath = "romfs:/main_levels/Thumper.gmd",
        .songPath = "romfs:/songs/Thumper.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 10,
        .songData = {
            .title = "Thumper", 
            .artist = "Waterflame"
        }
    },
    {
        .levelID = 19,
        .levelName = "Clutterfunk 2",
        .gmdPath = "romfs:/main_levels/Clutterfunk2.gmd",
        .songPath = "romfs:/songs/Clutterfunk2.mp3",
        .difficulty = MAIN_DIFF_DEMON,
        .stars = 16,
        .songData = {
            .title = "Clutterfunk 2", 
            .artist = "Waterflame"
        }
    },
    {
        .levelID = 20,
        .levelName = "Aura",
        .gmdPath = "romfs:/main_levels/Aura.gmd",
        .songPath = "romfs:/songs/Aura.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 11,
        .songData = {
            .title = "Aura", 
            .artist = "Creo"
        }
    },
    {
        .levelID = 21,
        .levelName = "Jack Russel",
        .gmdPath = "romfs:/main_levels/JackRussel.gmd",
        .songPath = "romfs:/songs/JackRussel.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .songData = {
            .title = "Jack Russel", 
            .artist = "Bossfight"
        }
    },
    {
        .levelID = 22,
        .levelName = "Streetwise",
        .gmdPath = "romfs:/main_levels/Streetwise.gmd",
        .songPath = "romfs:/songs/Streetwise.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .songData = {
            .title = "Streetwise", 
            .artist = "Waterflame"
        }
    },
};

const MainLevelPack *currentMainLevelPack = NULL;

extern const MainLevelPack robtopLevels = {
    .levels = robtopMainLevels,
    .count = ARRAY_LEN(robtopMainLevels)
};

extern const MainLevelPack gdpsLevels = {
    .levels = gdpsMainLevels,
    .count = ARRAY_LEN(gdpsMainLevels)
};

}