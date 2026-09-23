#include "main_levels.h"
#include "main.h"

MainLevelDefinition robtop_main_levels[] = {
    {
        .level_name = "Stereo Madness",
        .gmd_path = "romfs:/main_levels/StereoMadness.gmd",
        .song_path = "romfs:/songs/StereoMadness.mp3",
        .difficulty = MAIN_DIFF_EASY,
        .stars = 1,
        .song_data = {
            .title = "Stereo Madness", 
            .artist = "ForeverBound" 
        }
    },
    {
        .level_name = "Back On Track",
        .gmd_path = "romfs:/main_levels/BackOnTrack.gmd",
        .song_path = "romfs:/songs/BackOnTrack.mp3",
        .difficulty = MAIN_DIFF_EASY,
        .stars = 2,
        .song_data = {
            .title = "Back on Track", 
            .artist = "DJVI" 
        }
    },
    {
        .level_name = "Polargeist",
        .gmd_path = "romfs:/main_levels/Polargeist.gmd",
        .song_path = "romfs:/songs/Polargeist.mp3",
        .difficulty = MAIN_DIFF_NORMAL,
        .stars = 3,
        .song_data = {
            .title = "Polargeist", 
            .artist = "Step" 
        }
    },
    {
        .level_name = "Dry Out",
        .gmd_path = "romfs:/main_levels/DryOut.gmd",
        .song_path = "romfs:/songs/DryOut.mp3",
        .difficulty = MAIN_DIFF_NORMAL,
        .stars = 4,
        .song_data = {
            .title = "Dry Out", 
            .artist = "DJVI" 
        }
    },
    {
        .level_name = "Base After Base",
        .gmd_path = "romfs:/main_levels/BaseAfterBase.gmd",
        .song_path = "romfs:/songs/BaseAfterBase.mp3",
        .difficulty = MAIN_DIFF_HARD,
        .stars = 5,
        .song_data = {
            .title = "Base after Base", 
            .artist = "DJVI" 
        }
    },
    {
        .level_name = "Cant Let Go",
        .gmd_path = "romfs:/main_levels/CantLetGo.gmd",
        .song_path = "romfs:/songs/CantLetGo.mp3",
        .difficulty = MAIN_DIFF_HARD,
        .stars = 6,
        .song_data = {
            .title = "Cant let Go",
            .artist = "DJVI" 
        }
    },
    {
        .level_name = "Jumper",
        .gmd_path = "romfs:/main_levels/Jumper.gmd",
        .song_path = "romfs:/songs/Jumper.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 7,
        .song_data = {
            .title = "Jumper", 
            .artist = "Waterflame"
        }
    },
    {
        .level_name = "Time Machine",
        .gmd_path = "romfs:/main_levels/TimeMachine.gmd",
        .song_path = "romfs:/songs/TimeMachine.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 8,
        .song_data = {
            .title = "Time Machine", 
            .artist = "Waterflame"
        }
    },
    {
        .level_name = "Cycles",
        .gmd_path = "romfs:/main_levels/Cycles.gmd",
        .song_path = "romfs:/songs/Cycles.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 9,
        .song_data = {
            .title = "Cycles",  
            .artist = "DJVI" 
        }
    },
    {
        .level_name = "xStep",
        .gmd_path = "romfs:/main_levels/xStep.gmd",
        .song_path = "romfs:/songs/xStep.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 10,
        .song_data = {
            .title = "xStep", 
            .artist = "DJVI"
        }
    },
    {
        .level_name = "Clutterfunk",
        .gmd_path = "romfs:/main_levels/Clutterfunk.gmd",
        .song_path = "romfs:/songs/Clutterfunk.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 11,
        .song_data = {
            .title = "Clutterfunk", 
            .artist = "Waterflame"
        }
    },
    {
        .level_name = "Theory of Everything",
        .gmd_path = "romfs:/main_levels/TheoryofEverything.gmd",
        .song_path = "romfs:/songs/TheoryOfEverything.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .song_data = {
            .title = "Theory of Everything", 
            .artist = "DJ-Nate"
        }
    },
    {
        .level_name = "Electroman Adventures",
        .gmd_path = "romfs:/main_levels/Electroman.gmd",
        .song_path = "romfs:/songs/Electroman.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 10,
        .song_data = {
            .title = "Electroman Adventures", 
            .artist = "Waterflame"
        }
    },
    {
        .level_name = "Clubstep",
        .gmd_path = "romfs:/main_levels/Clubstep.gmd",
        .song_path = "romfs:/songs/Clubstep.mp3",
        .difficulty = MAIN_DIFF_DEMON,
        .stars = 14,
        .song_data = {
            .title = "Clubstep", 
            .artist = "DJ-Nate"
        }
    },
    {
        .level_name = "Electrodynamix",
        .gmd_path = "romfs:/main_levels/Electrodynamix.gmd",
        .song_path = "romfs:/songs/Electrodynamix.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .song_data = {
            .title = "Electrodynamix", 
            .artist = "DJ-Nate"
        }
    },
    {
        .level_name = "Hexagon Force",
        .gmd_path = "romfs:/main_levels/HexagonForce.gmd",
        .song_path = "romfs:/songs/HexagonForce.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .song_data = {
            .title = "Hexagon Force", 
            .artist = "Waterflame" 
        }
    },
    {
        .level_name = "Blast Processing",
        .gmd_path = "romfs:/main_levels/BlastProcessing.gmd",
        .song_path = "romfs:/songs/BlastProcessing.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 10,
        .song_data = {
            .title = "Blast Processing", 
            .artist = "Waterflame"
        }
    },
    {
        .level_name = "Theory of Everything 2",
        .gmd_path = "romfs:/main_levels/TheoryofEverything2.gmd",
        .song_path = "romfs:/songs/TheoryOfEverything2.mp3",
        .difficulty = MAIN_DIFF_DEMON,
        .stars = 14,
        .song_data = {
            .title = "Theory of Everything 2", 
            .artist = "DJ-Nate"
        }
    }
};


MainLevelDefinition gdps_main_levels[] = {
    {
        .level_name = "Stereo Madness",
        .gmd_path = "romfs:/main_levels/StereoMadness.gmd",
        .song_path = "romfs:/songs/StereoMadness.mp3",
        .difficulty = MAIN_DIFF_EASY,
        .stars = 1,
        .song_data = {
            .title = "Stereo Madness", 
            .artist = "ForeverBound" 
        }
    },
    {
        .level_name = "Back On Track",
        .gmd_path = "romfs:/main_levels/BackOnTrack.gmd",
        .song_path = "romfs:/songs/BackOnTrack.mp3",
        .difficulty = MAIN_DIFF_EASY,
        .stars = 2,
        .song_data = {
            .title = "Back on Track", 
            .artist = "DJVI" 
        }
    },
    {
        .level_name = "Polargeist",
        .gmd_path = "romfs:/main_levels/Polargeist.gmd",
        .song_path = "romfs:/songs/Polargeist.mp3",
        .difficulty = MAIN_DIFF_NORMAL,
        .stars = 3,
        .song_data = {
            .title = "Polargeist", 
            .artist = "Step" 
        }
    },
    {
        .level_name = "Dry Out",
        .gmd_path = "romfs:/main_levels/DryOut.gmd",
        .song_path = "romfs:/songs/DryOut.mp3",
        .difficulty = MAIN_DIFF_NORMAL,
        .stars = 4,
        .song_data = {
            .title = "Dry Out", 
            .artist = "DJVI" 
        }
    },
    {
        .level_name = "Base After Base",
        .gmd_path = "romfs:/main_levels/BaseAfterBase.gmd",
        .song_path = "romfs:/songs/BaseAfterBase.mp3",
        .difficulty = MAIN_DIFF_HARD,
        .stars = 5,
        .song_data = {
            .title = "Base after Base", 
            .artist = "DJVI" 
        }
    },
    {
        .level_name = "Cant Let Go",
        .gmd_path = "romfs:/main_levels/CantLetGo.gmd",
        .song_path = "romfs:/songs/CantLetGo.mp3",
        .difficulty = MAIN_DIFF_HARD,
        .stars = 6,
        .song_data = {
            .title = "Cant let Go",
            .artist = "DJVI" 
        }
    },
    {
        .level_name = "Jumper",
        .gmd_path = "romfs:/main_levels/Jumper.gmd",
        .song_path = "romfs:/songs/Jumper.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 7,
        .song_data = {
            .title = "Jumper", 
            .artist = "Waterflame"
        }
    },
    {
        .level_name = "Time Machine",
        .gmd_path = "romfs:/main_levels/TimeMachine.gmd",
        .song_path = "romfs:/songs/TimeMachine.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 8,
        .song_data = {
            .title = "Time Machine", 
            .artist = "Waterflame"
        }
    },
    {
        .level_name = "Cycles",
        .gmd_path = "romfs:/main_levels/Cycles.gmd",
        .song_path = "romfs:/songs/Cycles.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 9,
        .song_data = {
            .title = "Cycles",  
            .artist = "DJVI" 
        }
    },
    {
        .level_name = "xStep",
        .gmd_path = "romfs:/main_levels/xStep.gmd",
        .song_path = "romfs:/songs/xStep.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 10,
        .song_data = {
            .title = "xStep", 
            .artist = "DJVI"
        }
    },
    {
        .level_name = "Clutterfunk",
        .gmd_path = "romfs:/main_levels/Clutterfunk.gmd",
        .song_path = "romfs:/songs/Clutterfunk.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 11,
        .song_data = {
            .title = "Clutterfunk", 
            .artist = "Waterflame"
        }
    },
    {
        .level_name = "Theory of Everything",
        .gmd_path = "romfs:/main_levels/TheoryofEverything.gmd",
        .song_path = "romfs:/songs/TheoryOfEverything.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .song_data = {
            .title = "Theory of Everything", 
            .artist = "DJ-Nate"
        }
    },
    {
        .level_name = "Electroman Adventures",
        .gmd_path = "romfs:/main_levels/Electroman.gmd",
        .song_path = "romfs:/songs/Electroman.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 10,
        .song_data = {
            .title = "Electroman Adventures", 
            .artist = "Waterflame"
        }
    },
    {
        .level_name = "Clubstep",
        .gmd_path = "romfs:/main_levels/Clubstep.gmd",
        .song_path = "romfs:/songs/Clubstep.mp3",
        .difficulty = MAIN_DIFF_DEMON,
        .stars = 14,
        .song_data = {
            .title = "Clubstep", 
            .artist = "DJ-Nate"
        }
    },
    {
        .level_name = "Electrodynamix",
        .gmd_path = "romfs:/main_levels/Electrodynamix.gmd",
        .song_path = "romfs:/songs/Electrodynamix.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .song_data = {
            .title = "Electrodynamix", 
            .artist = "DJ-Nate"
        }
    },
    {
        .level_name = "Hexagon Force",
        .gmd_path = "romfs:/main_levels/HexagonForce.gmd",
        .song_path = "romfs:/songs/HexagonForce.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .song_data = {
            .title = "Hexagon Force", 
            .artist = "Waterflame" 
        }
    },
    {
        .level_name = "Blast Processing",
        .gmd_path = "romfs:/main_levels/BlastProcessing.gmd",
        .song_path = "romfs:/songs/BlastProcessing.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 10,
        .song_data = {
            .title = "Blast Processing", 
            .artist = "Waterflame"
        }
    },
    {
        .level_name = "Theory of Everything 2",
        .gmd_path = "romfs:/main_levels/TheoryofEverything2.gmd",
        .song_path = "romfs:/songs/TheoryOfEverything2.mp3",
        .difficulty = MAIN_DIFF_DEMON,
        .stars = 14,
        .song_data = {
            .title = "Theory of Everything 2", 
            .artist = "DJ-Nate"
        }
    },
    {
        .level_name = "Thumper",
        .gmd_path = "romfs:/main_levels/Thumper.gmd",
        .song_path = "romfs:/songs/Thumper.mp3",
        .difficulty = MAIN_DIFF_HARDER,
        .stars = 10,
        .song_data = {
            .title = "Thumper", 
            .artist = "Waterflame"
        }
    },
    {
        .level_name = "Clutterfunk 2",
        .gmd_path = "romfs:/main_levels/Clutterfunk2.gmd",
        .song_path = "romfs:/songs/Clutterfunk2.mp3",
        .difficulty = MAIN_DIFF_DEMON,
        .stars = 16,
        .song_data = {
            .title = "Clutterfunk 2", 
            .artist = "Waterflame"
        }
    },
    {
        .level_name = "Aura",
        .gmd_path = "romfs:/main_levels/Aura.gmd",
        .song_path = "romfs:/songs/Aura.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 11,
        .song_data = {
            .title = "Aura", 
            .artist = "Creo"
        }
    },
    {
        .level_name = "Jack Russel",
        .gmd_path = "romfs:/main_levels/JackRussel.gmd",
        .song_path = "romfs:/songs/JackRussel.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .song_data = {
            .title = "Jack Russel", 
            .artist = "Bossfight"
        }
    },
    {
        .level_name = "Streetwise",
        .gmd_path = "romfs:/main_levels/Streetwise.gmd",
        .song_path = "romfs:/songs/Streetwise.mp3",
        .difficulty = MAIN_DIFF_INSANE,
        .stars = 12,
        .song_data = {
            .title = "Streetwise", 
            .artist = "Waterflame"
        }
    },
};

const MainLevelPack *current_main_level_pack = NULL;

const MainLevelPack robtop_levels = {
    .levels = robtop_main_levels,
    .count = ARRAY_LEN(robtop_main_levels)
};

const MainLevelPack gdps_levels = {
    .levels = gdps_main_levels,
    .count = ARRAY_LEN(gdps_main_levels)
};