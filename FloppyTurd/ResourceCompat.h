#pragma once
#include "ResourceManager.h"
#include "RaylibCompat.h"

// Compatibility layer to transition from old Resources.h to ResourceManager
// This allows us to gradually update the codebase without breaking everything at once

// Define logging levels for Raylib compatibility
#ifndef LOG_INFO
#define LOG_INFO     1
#endif
#ifndef LOG_WARNING
#define LOG_WARNING  2
#endif
#ifndef LOG_ERROR
#define LOG_ERROR    3
#endif
#ifndef LOG_DEBUG
#define LOG_DEBUG    4
#endif

namespace Resources {
    // Helper function to get ResourceManager instance
    inline ResourceManager& RM() {
        return ResourceManager::GetInstance();
    }

    // Core textures (most commonly used)
    inline Texture2D GetScoreBoard() { return RM().GetTexture("scoreboard"); }
    inline Texture2D GetTurdHeart() { return RM().GetTexture("turd_heart"); }
    inline Texture2D GetCoinBag() { return RM().GetTexture("coin_bag"); }

    // CRITICAL MISSING FUNCTIONS - Adding these to fix Playing.cpp crash
    inline Sound GetGotScore() { return RM().GetSound("got_score"); }
    inline Texture2D GetArrowLeft() { return RM().GetTexture("arrow_left"); }
    inline Texture2D GetArrowRight() { return RM().GetTexture("arrow_right"); }
    inline Texture2D GetArrowLeftHover() { return RM().GetTexture("arrow_left_hover"); }
    inline Texture2D GetArrowRightHover() { return RM().GetTexture("arrow_right_hover"); }
    inline Texture2D GetBlueButton() { return RM().GetTexture("blue_button"); }
    inline Texture2D GetBlueButtonHover() { return RM().GetTexture("blue_button_hover"); }

    // Heart variants - CRITICAL for health system UI
    inline Texture2D GetTurdHeartSmall() { return RM().GetTexture("turd_heart_small"); }
    inline Texture2D GetTurdHeart0Half() { return RM().GetTexture("turd_heart_0_half"); }
    inline Texture2D GetTurdHeart0HalfHollow() { return RM().GetTexture("turd_heart_0_half_hollow"); }
    inline Texture2D GetTurdHeart0ThirdHollow() { return RM().GetTexture("turd_heart_0_third_hollow"); }
    inline Texture2D GetTurdHeart0ThirdHollow1Third() { return RM().GetTexture("turd_heart_0_third_hollow_1_third"); }
    inline Texture2D GetTurdHeart0HalfHollow1Half() { return RM().GetTexture("turd_heart_0_half_hollow_1_half"); }
    inline Texture2D GetTurdHeart1Half() { return RM().GetTexture("turd_heart_1_half"); }
    inline Texture2D GetTurdHeart1HalfHollow() { return RM().GetTexture("turd_heart_1_half_hollow"); }
    inline Texture2D GetTurdHeart1Third() { return RM().GetTexture("turd_heart_1_third"); }
    inline Texture2D GetTurdHeart1ThirdHollow() { return RM().GetTexture("turd_heart_1_third_hollow"); }
    inline Texture2D GetTurdHeart1ThirdHollow1Third() { return RM().GetTexture("turd_heart_1_third_hollow_1_third"); }
    inline Texture2D GetTurdHeart2Thirds() { return RM().GetTexture("turd_heart_2_thirds"); }
    inline Texture2D GetTurdHeart2ThirdsHollow() { return RM().GetTexture("turd_heart_2_thirds_hollow"); }
    inline Texture2D GetTurdHeart2ThirdsHollow2Thirds() { return RM().GetTexture("turd_heart_2_thirds_hollow_2_thirds"); }

    // Player sprites
    inline Texture2D GetTurdletIdle() { return RM().GetTexture("turdlet_idle"); }
    inline Texture2D GetTurdletJump() { return RM().GetTexture("turdlet_jump"); }
    inline Texture2D GetTurdletShoot() { return RM().GetTexture("turdlet_shoot"); }
    inline Texture2D GetTurdletHurt() { return RM().GetTexture("turdlet_hurt"); }

    // Main menu textures
    inline Texture2D GetMainMenuBackground() { return RM().GetTexture("main_menu_bg"); }
    inline Texture2D GetFloppyLogo() { return RM().GetTexture("floppy_logo"); }
    inline Texture2D GetFinLogo() { return RM().GetTexture("fin_logo"); }

    // Main Menu Buttons
    inline Texture2D GetPlayButton() { return RM().GetTexture("play_button"); }
    inline Texture2D GetPlayButtonFocused() { return RM().GetTexture("play_button_focused"); }
    inline Texture2D GetPlayButtonSelected() { return RM().GetTexture("play_button_selected"); }
    inline Texture2D GetQuickPlayButton() { return RM().GetTexture("quickplay_button"); }
    inline Texture2D GetQuickPlayButtonFocused() { return RM().GetTexture("quickplay_button_focused"); }
    inline Texture2D GetQuickPlayButtonSelected() { return RM().GetTexture("quickplay_button_selected"); }
    inline Texture2D GetOptionsButton() { return RM().GetTexture("options_button"); }
    inline Texture2D GetOptionsButtonFocused() { return RM().GetTexture("options_button_focused"); }
    inline Texture2D GetOptionsButtonSelected() { return RM().GetTexture("options_button_selected"); }

    // Level paintings
    inline Texture2D GetParkPainting() { return RM().GetTexture("park_painting"); }
    inline Texture2D GetSewerPainting() { return RM().GetTexture("sewer_painting"); }
    inline Texture2D GetDesertPainting() { return RM().GetTexture("desert_painting"); }
    inline Texture2D GetSnowPainting() { return RM().GetTexture("snow_painting"); }
    inline Texture2D GetCastlePainting() { return RM().GetTexture("castle_painting"); }
    inline Texture2D GetRatKingPainting() { return RM().GetTexture("ratking_painting"); }
    inline Texture2D GetEmptyPainting() { return RM().GetTexture("empty_painting"); }
    inline Texture2D GetLockedPainting() { return RM().GetTexture("locked_painting"); }

    // Core sounds
    inline Sound GetHurt() { return RM().GetSound("hurt"); }
    inline Sound GetGotCoin() { return RM().GetSound("got_coin"); }
    inline Sound GetGotHealth() { return RM().GetSound("got_health"); }
    inline Sound GetGotHealthBig() { return RM().GetSound("got_health_big"); }

    // Enemy kill sounds
    inline Sound GetRatCopterKill() { return RM().GetSound("ratcopter_kill"); }
    inline Sound GetToiletPaperKill() { return RM().GetSound("toilet_paper_kill"); }
    inline Sound GetRatKingKill() { return RM().GetSound("ratking_kill"); }

    // Fart sounds
    inline Sound GetFart1() { return RM().GetSound("fart1"); }
    inline Sound GetFart2() { return RM().GetSound("fart2"); }
    inline Sound GetFart3() { return RM().GetSound("fart3"); }
    inline Sound GetFart4() { return RM().GetSound("fart4"); }
    inline Sound GetFart5() { return RM().GetSound("fart5"); }
    inline Sound GetFart6() { return RM().GetSound("fart6"); }
    inline Sound GetFart7() { return RM().GetSound("fart7"); }
    inline Sound GetFart8() { return RM().GetSound("fart8"); }
    inline Sound GetFart9() { return RM().GetSound("fart9"); }
    inline Sound GetFart10() { return RM().GetSound("fart10"); }
    inline Sound GetFart11() { return RM().GetSound("fart11"); }

    // Level assets  
    inline Texture2D GetTopToilet() { return RM().GetTexture("top_toilet"); }
    inline Texture2D GetBottomToilet() { return RM().GetTexture("bottom_toilet"); }
    inline Texture2D GetTopToiletSnow() { return RM().GetTexture("top_toilet_snow"); }
    inline Texture2D GetBottomToiletSnow() { return RM().GetTexture("bottom_toilet_snow"); }
    inline Texture2D GetTopToiletGold() { return RM().GetTexture("top_toilet_gold"); }
    inline Texture2D GetBottomToiletGold() { return RM().GetTexture("bottom_toilet_gold"); }
    
    // Pipes
    inline Texture2D GetTopPipeOrange() { return RM().GetTexture("top_pipe_orange"); }
    inline Texture2D GetTopPipeBlue() { return RM().GetTexture("top_pipe_blue"); }
    inline Texture2D GetBottomPipeOrange() { return RM().GetTexture("bottom_pipe_orange"); }
    inline Texture2D GetBottomPipeBlue() { return RM().GetTexture("bottom_pipe_blue"); }

    // Enemies
    inline Texture2D GetSnowManRed() { return RM().GetTexture("snowman_red"); }
    inline Texture2D GetSnowManRedThrow() { return RM().GetTexture("snowman_red_throw"); }
    inline Texture2D GetSnowManBlue() { return RM().GetTexture("snowman_blue"); }
    inline Texture2D GetSnowManGreen() { return RM().GetTexture("snowman_green"); }
    inline Texture2D GetSnowManChad() { return RM().GetTexture("snowman_chad"); }
    inline Texture2D GetBirdIdle() { return RM().GetTexture("bird_idle"); }
    inline Texture2D GetBirdHurt() { return RM().GetTexture("bird_hurt"); }

    // Boss
    inline Texture2D GetRatKingIdle() { return RM().GetTexture("ratking_idle"); }
    inline Texture2D GetBlastSmall() { return RM().GetTexture("blast_small"); }
    inline Texture2D GetBlastBig() { return RM().GetTexture("blast_big"); }

    // Environment  
    inline Texture2D GetOuthouseSolo() { return RM().GetTexture("outhouse_solo"); }
    inline Texture2D GetBrickWallTexture() { return RM().GetTexture("brick_wall"); }
    inline Texture2D GetSpikeBallTexture() { return RM().GetTexture("spike_ball"); }
    inline Texture2D GetSpikeBallBase() { return RM().GetTexture("spike_ball_base"); }
    
    // Pickups
    inline Texture2D GetPooHeart() { return RM().GetTexture("poo_heart"); }
    inline Texture2D GetPooHeartBig() { return RM().GetTexture("poo_heart_big"); }
    inline Texture2D GetPooHeartInvisible() { return RM().GetTexture("poo_heart_invisible"); }
    
    // Legacy path for PooHeartInvisible (missing from original definitions)
    inline const char* PooHeartInvisible = "resources/objects/PooHeartInvisible.png";
    
    // Hats
    inline Texture2D GetPoopHat() { return RM().GetTexture("poophat"); }
    
    // Credits
    inline Texture2D GetCreditsBackgroundTexture() { return RM().GetTexture("credits_background"); }

    // Music - Core level music
    inline Music GetLevelOneMusic() { return RM().GetMusic("level1_music"); }
    inline Music GetLevelOneSlow() { return RM().GetMusic("level1_slow"); }
    inline Music GetLevelOneFast() { return RM().GetMusic("level1_fast"); }
    inline Music GetLevelTwoMusic() { return RM().GetMusic("level2_music"); }
    inline Music GetLevelTwoSlow() { return RM().GetMusic("level2_slow"); }
    inline Music GetLevelTwoFast() { return RM().GetMusic("level2_fast"); }
    inline Music GetLevelThreeMusic() { return RM().GetMusic("level3_music"); }
    inline Music GetLevelThreeSlow() { return RM().GetMusic("level3_slow"); }
    inline Music GetLevelThreeFast() { return RM().GetMusic("level3_fast"); }
    inline Music GetLevelFourMusic() { return RM().GetMusic("level4_music"); }
    inline Music GetLevelFourSlow() { return RM().GetMusic("level4_slow"); }
    inline Music GetLevelFourFast() { return RM().GetMusic("level4_fast"); }
    inline Music GetLevelFiveMusic() { return RM().GetMusic("level5_music"); }
    inline Music GetMainMenuMusic() { return RM().GetMusic("main_menu_music"); }
    inline Music GetCreditsMusic() { return RM().GetMusic("credits_music"); }
    
    // Level Backgrounds - Critical for gameplay
    inline Texture2D GetParkBackLayer() { return RM().GetTexture("park_back"); }
    inline Texture2D GetParkMidLayer() { return RM().GetTexture("park_mid"); }
    inline Texture2D GetParkFrontLayer() { return RM().GetTexture("park_front"); }
    inline Texture2D GetDesertBackLayer() { return RM().GetTexture("desert_back"); }
    inline Texture2D GetDesertMidLayer() { return RM().GetTexture("desert_mid"); }
    inline Texture2D GetDesertFrontLayer() { return RM().GetTexture("desert_front"); }
    inline Texture2D GetDesertCactiLayer() { return RM().GetTexture("desert_cacti_layer"); }
    inline Texture2D GetSnowBackground() { return RM().GetTexture("snow_background"); }
    inline Texture2D GetSnowMountains() { return RM().GetTexture("snow_mountains"); }
    inline Texture2D GetCastleWall() { return RM().GetTexture("castle_wall"); }
    inline Texture2D GetCastleBars() { return RM().GetTexture("castle_bars"); }
    
    // Sewer Level - Critical pipes
    inline Texture2D GetSewerWallA() { return RM().GetTexture("sewer_wall_a"); }
    inline Texture2D GetSewerWallB() { return RM().GetTexture("sewer_wall_b"); }
    inline Texture2D GetSewerWallC() { return RM().GetTexture("sewer_wall_c"); }
    inline Texture2D GetSewerWallD() { return RM().GetTexture("sewer_wall_d"); }
    
    // Castle Level - Decorations
    inline Texture2D GetTorchPillar() { return RM().GetTexture("torch_pillar"); }
    inline Texture2D GetChandelier() { return RM().GetTexture("chandelier"); }
    inline Texture2D GetCurtains() { return RM().GetTexture("curtains"); }
    
    // Enemies - Important for gameplay
    inline Texture2D GetRatCopterIdle() { return RM().GetTexture("ratcopter_idle"); }
    inline Texture2D GetRatCopterHurt() { return RM().GetTexture("ratcopter_hurt"); }
    
    // Cactus variants
    inline Texture2D GetCactiA() { return RM().GetTexture("cacti_a"); }
    inline Texture2D GetCactiB() { return RM().GetTexture("cacti_b"); }
    inline Texture2D GetCactiC() { return RM().GetTexture("cacti_c"); }
    inline Texture2D GetCactiD() { return RM().GetTexture("cacti_d"); }
    inline Texture2D GetCactiE() { return RM().GetTexture("cacti_e"); }
          
      // UI Elements - Important for interface

    // **CRITICAL SPRITE TEXTURE HELPER**
    // Maps file paths to ResourceManager texture IDs for Sprite class compatibility
    inline Texture2D GetTextureByPath(const std::string& filePath) {
        // Map common file paths to resource IDs
        if (filePath == "resources/turd/TurdletIdle.png") return RM().GetTexture("turdlet_idle");
        if (filePath == "resources/turd/TurdletJump.png") return RM().GetTexture("turdlet_jump");
        if (filePath == "resources/turd/TurdletShoot.png") return RM().GetTexture("turdlet_shoot");
        if (filePath == "resources/turd/TurdletHurt.png") return RM().GetTexture("turdlet_hurt");
        if (filePath == "resources/turd/BigTurdIdle.png") return RM().GetTexture("big_turd_idle");
        if (filePath == "resources/turd/BigTurdJump.png") return RM().GetTexture("big_turd_jump");
        if (filePath == "resources/turd/BigTurdShoot.png") return RM().GetTexture("big_turd_shoot");
        if (filePath == "resources/turd/BigTurdHurt.png") return RM().GetTexture("big_turd_hurt");
        if (filePath == "resources/turd/TeenageTurdIdle.png") return RM().GetTexture("teenage_turd_idle");
        if (filePath == "resources/turd/TeenageTurdJump.png") return RM().GetTexture("teenage_turd_jump");
        if (filePath == "resources/turd/TeenageTurdShoot.png") return RM().GetTexture("teenage_turd_shoot");
        if (filePath == "resources/turd/TeenageTurdHurt.png") return RM().GetTexture("teenage_turd_hurt");
        if (filePath == "resources/turd/Floppy Poop.png") return RM().GetTexture("poop_small");
        if (filePath == "resources/turd/Floppy Poop Mid.png") return RM().GetTexture("poop_mid");
        if (filePath == "resources/turd/Floppy Poop Large.png") return RM().GetTexture("poop_large");
        
        // Janitor sprites
        if (filePath == "resources/objects/Janitor.png") return RM().GetTexture("janitor_idle");
        if (filePath == "resources/objects/JanitorSweep.png") return RM().GetTexture("janitor_sweep");
        if (filePath == "resources/objects/JanitorSurprise.png") return RM().GetTexture("janitor_surprise");
        
        // Cactus variants - CRITICAL for desert level (these were causing warnings)
        if (filePath == "resources/objects/CactiA.png") return RM().GetTexture("cacti_a");
        if (filePath == "resources/objects/CactiB.png") return RM().GetTexture("cacti_b");
        if (filePath == "resources/objects/CactiC.png") return RM().GetTexture("cacti_c");
        if (filePath == "resources/objects/CactiD.png") return RM().GetTexture("cacti_d");
        if (filePath == "resources/objects/CactiE.png") return RM().GetTexture("cacti_e");
        if (filePath == "resources/objects/CactiBush.png") return RM().GetTexture("cacti_bush");
        
        // Desert environment - CRITICAL (these were causing warnings)
        if (filePath == "resources/environment/dancingcactismall.png") return RM().GetTexture("dancing_cacti_small");
        if (filePath == "resources/environment/dancingcacti.png") return RM().GetTexture("dancing_cacti");
        if (filePath == "resources/environment/dancingcacticowboy.png") return RM().GetTexture("dancing_cacti_cowboy");
        
        // Castle environment - CRITICAL (these were causing warnings)
        if (filePath == "resources/environment/curtains.png") return RM().GetTexture("curtains");
        if (filePath == "resources/environment/castlelevelfloortorch.png") return RM().GetTexture("floor_torch");
        if (filePath == "resources/environment/castlelevelchandelier.png") return RM().GetTexture("chandelier");
        if (filePath == "resources/environment/TorchPillar.png") return RM().GetTexture("torch_pillar");
        
        // Boss environment - CRITICAL (these were causing warnings)
        if (filePath == "resources/environment/ratkingbackground.png") return RM().GetTexture("boss_background");
        if (filePath == "resources/environment/darkclouds.png") return RM().GetTexture("boss_dark_clouds");
        if (filePath == "resources/environment/BossFloor.png") return RM().GetTexture("boss_floor");
        if (filePath == "resources/environment/screenCurtains.png") return RM().GetTexture("boss_curtains");
        if (filePath == "resources/environment/BossWalls.png") return RM().GetTexture("boss_walls");
        if (filePath == "resources/environment/bosspillar.png") return RM().GetTexture("boss_pillar");
        
        // Castle paintings - CRITICAL (these were causing warnings)
        if (filePath == "resources/objects/CabinPainting.png") return RM().GetTexture("painting_a");
        if (filePath == "resources/objects/RabbitKnightPainting.png") return RM().GetTexture("painting_b");
        if (filePath == "resources/objects/RatBeachPainting.png") return RM().GetTexture("painting_c");
        if (filePath == "resources/objects/RiverWalkPainting.png") return RM().GetTexture("painting_d");
        
        // Boss sprites - CRITICAL (these were causing warnings)
        if (filePath == "resources/enemies/RatkingAimTorsoOnly.png") return RM().GetTexture("ratking_aim_torso");
        if (filePath == "resources/enemies/RatkingAimTossArmOnly.png") return RM().GetTexture("ratking_aim_front_arm");
        if (filePath == "resources/enemies/RatkingAimBackArmOnly.png") return RM().GetTexture("ratking_aim_back_arm");
        
        // Hat sprites - CRITICAL FIXED! Now using ResourceManager instead of direct LoadTexture calls
        // Cowboy Hat sprites
        if (filePath == "resources/hats/cowboyhatturdletjump.png") return RM().GetTexture("cowboy_hat_turdlet_jump");
        if (filePath == "resources/hats/cowboyhatturdletshoot.png") return RM().GetTexture("cowboy_hat_turdlet_shoot");
        if (filePath == "resources/hats/cowboyhatbigturdjump.png") return RM().GetTexture("cowboy_hat_big_jump");
        if (filePath == "resources/hats/cowboyhatbigturdshoot.png") return RM().GetTexture("cowboy_hat_big_shoot");
        
        // Flower Hat sprites
        if (filePath == "resources/hats/flowerhatturdletjump.png") return RM().GetTexture("flower_hat_turdlet_jump");
        if (filePath == "resources/hats/flowerhatturdletshoot.png") return RM().GetTexture("flower_hat_turdlet_shoot");
        if (filePath == "resources/hats/flowerhatbigturdjump.png") return RM().GetTexture("flower_hat_big_jump");
        if (filePath == "resources/hats/flowerhatbigturdshoot.png") return RM().GetTexture("flower_hat_big_shoot");
        
        // Doorag Hat sprites  
        if (filePath == "resources/hats/dooragturdletjump.png") return RM().GetTexture("doorag_hat_turdlet_jump");
        if (filePath == "resources/hats/dooragturdletshoot.png") return RM().GetTexture("doorag_hat_turdlet_shoot");
        if (filePath == "resources/hats/dooragbigturdjump.png") return RM().GetTexture("doorag_hat_big_jump");
        if (filePath == "resources/hats/dooragbigturdshoot.png") return RM().GetTexture("doorag_hat_big_shoot");
        
        // Ball Cap sprites
        if (filePath == "resources/hats/ballcapturdletjump.png") return RM().GetTexture("ballcap_hat_turdlet_jump");
        if (filePath == "resources/hats/ballcapturdletshoot.png") return RM().GetTexture("ballcap_hat_turdlet_shoot");
        if (filePath == "resources/hats/ballcapbigturdjump.png") return RM().GetTexture("ballcap_hat_big_jump");
        if (filePath == "resources/hats/ballcapbigturdshoot.png") return RM().GetTexture("ballcap_hat_big_shoot");
        
        // Pinwheel Hat sprites
        if (filePath == "resources/hats/pinwheelturdletjump.png") return RM().GetTexture("pinwheel_hat_turdlet_jump");
        if (filePath == "resources/hats/pinwheelturdletshoot.png") return RM().GetTexture("pinwheel_hat_turdlet_shoot");
        if (filePath == "resources/hats/pinwheelbigturdjump.png") return RM().GetTexture("pinwheel_hat_big_jump");
        if (filePath == "resources/hats/pinwheelbigturdshoot.png") return RM().GetTexture("pinwheel_hat_big_shoot");
        
        // Straw Hat sprites
        if (filePath == "resources/hats/strawhatturdletjump.png") return RM().GetTexture("straw_hat_turdlet_jump");
        if (filePath == "resources/hats/strawhatturdletshoot.png") return RM().GetTexture("straw_hat_turdlet_shoot");
        if (filePath == "resources/hats/strawhatbigturdjump.png") return RM().GetTexture("straw_hat_big_jump");
        if (filePath == "resources/hats/strawhatbigturdshoot.png") return RM().GetTexture("straw_hat_big_shoot");
        
        // Samurai Hat sprites
        if (filePath == "resources/hats/samuraiturdletjump.png") return RM().GetTexture("samurai_hat_turdlet_jump");
        if (filePath == "resources/hats/samuraiturdletshoot.png") return RM().GetTexture("samurai_hat_turdlet_shoot");
        if (filePath == "resources/hats/samuraibigturdjump.png") return RM().GetTexture("samurai_hat_big_jump");
        if (filePath == "resources/hats/samuraibigturdshoot.png") return RM().GetTexture("samurai_hat_big_shoot");
        
        // Top Hat sprites
        if (filePath == "resources/hats/tophatturdletjump.png") return RM().GetTexture("top_hat_turdlet_jump");
        if (filePath == "resources/hats/tophatturdletshoot.png") return RM().GetTexture("top_hat_turdlet_shoot");
        if (filePath == "resources/hats/tophatbigturdjump.png") return RM().GetTexture("top_hat_big_jump");
        if (filePath == "resources/hats/tophatbigturdshoot.png") return RM().GetTexture("top_hat_big_shoot");
        
        // Ushanka sprites
        if (filePath == "resources/hats/ushankaturdletjump.png") return RM().GetTexture("ushanka_hat_turdlet_jump");
        if (filePath == "resources/hats/ushankaturdletshoot.png") return RM().GetTexture("ushanka_hat_turdlet_shoot");
        if (filePath == "resources/hats/ushankabigturdjump.png") return RM().GetTexture("ushanka_hat_big_jump");
        if (filePath == "resources/hats/ushankabigturdshoot.png") return RM().GetTexture("ushanka_hat_big_shoot");
        
        // Beret sprites
        if (filePath == "resources/hats/berethatturdletjump.png") return RM().GetTexture("beret_hat_turdlet_jump");
        if (filePath == "resources/hats/berethatturdletshoot.png") return RM().GetTexture("beret_hat_turdlet_shoot");
        if (filePath == "resources/hats/berethatbigturdjump.png") return RM().GetTexture("beret_hat_big_jump");
        if (filePath == "resources/hats/berethatbigturdshoot.png") return RM().GetTexture("beret_hat_big_shoot");
        
        // Crown sprites
        if (filePath == "resources/hats/crownhatturdletjump.png") return RM().GetTexture("crown_hat_turdlet_jump");
        if (filePath == "resources/hats/crownhatturdletshoot.png") return RM().GetTexture("crown_hat_turdlet_shoot");
        if (filePath == "resources/hats/crownhatbigturdjump.png") return RM().GetTexture("crown_hat_big_jump");
        if (filePath == "resources/hats/crownhatbigturdshoot.png") return RM().GetTexture("crown_hat_big_shoot");
        
        // Poop Hat sprites
        if (filePath == "resources/hats/poophatturdletjump.png") return RM().GetTexture("poop_hat_turdlet_jump");
        if (filePath == "resources/hats/poophatturdletshoot.png") return RM().GetTexture("poop_hat_turdlet_shoot");
        if (filePath == "resources/hats/poophatbigturdjump.png") return RM().GetTexture("poop_hat_big_jump");
        if (filePath == "resources/hats/poophatbigturdshoot.png") return RM().GetTexture("poop_hat_big_shoot");
        
        // Ramses Hat sprites
        if (filePath == "resources/hats/ramsesturdletjump.png") return RM().GetTexture("ramses_hat_turdlet_jump");
        if (filePath == "resources/hats/ramsesturdletshoot.png") return RM().GetTexture("ramses_hat_turdlet_shoot");
        if (filePath == "resources/hats/ramsesbigturdjump.png") return RM().GetTexture("ramses_hat_big_jump");
        if (filePath == "resources/hats/ramsesbigturdshoot.png") return RM().GetTexture("ramses_hat_big_shoot");
        
        // Spartan Hat sprites
        if (filePath == "resources/hats/spartanhatturdletjump.png") return RM().GetTexture("spartan_hat_turdlet_jump");
        if (filePath == "resources/hats/spartanhatturdletshoot.png") return RM().GetTexture("spartan_hat_turdlet_shoot");
        if (filePath == "resources/hats/spartanhatbigturdjump.png") return RM().GetTexture("spartan_hat_big_jump");
        if (filePath == "resources/hats/spartanhatbigturdshoot.png") return RM().GetTexture("spartan_hat_big_shoot");
        
        // Shell Hat sprites
        if (filePath == "resources/hats/shellhatturdletjump.png") return RM().GetTexture("shell_hat_turdlet_jump");
        if (filePath == "resources/hats/shellhatturdletshoot.png") return RM().GetTexture("shell_hat_turdlet_shoot");
        if (filePath == "resources/hats/shellhatbigturdjump.png") return RM().GetTexture("shell_hat_big_jump");
        if (filePath == "resources/hats/shellhatbigturdshoot.png") return RM().GetTexture("shell_hat_big_shoot");
        
        // Enemy sprites
        if (filePath == "resources/enemies/BirdIdle.png") return RM().GetTexture("bird_idle");
        if (filePath == "resources/enemies/BirdHurt.png") return RM().GetTexture("bird_hurt");
        if (filePath == "resources/enemies/SnowManIdle.png") return RM().GetTexture("snowman_red");
        if (filePath == "resources/enemies/SnowManThrow.png") return RM().GetTexture("snowman_red_throw");
        if (filePath == "resources/enemies/SnowManChill.png") return RM().GetTexture("snowman_blue");
        if (filePath == "resources/enemies/SnowManGreen.png") return RM().GetTexture("snowman_green");
        if (filePath == "resources/enemies/SnowManChad.png") return RM().GetTexture("snowman_chad");
        if (filePath == "resources/enemies/RatCopterIdle.png") return RM().GetTexture("ratcopter_idle");
        if (filePath == "resources/enemies/RatCopterHurt.png") return RM().GetTexture("ratcopter_hurt");
        
        // Boss sprites
        if (filePath == "resources/enemies/Ratking.png") return RM().GetTexture("ratking_idle");
        if (filePath == "resources/enemies/RatkingWalk.png") return RM().GetTexture("ratking_walk");
        if (filePath == "resources/enemies/RatkingHurt.png") return RM().GetTexture("ratking_hurt");
        if (filePath == "resources/enemies/RatkingDeath.png") return RM().GetTexture("ratking_death");
        
        // Pickups
        if (filePath == "resources/objects/GoldCoin.png") return RM().GetTexture("gold_coin");
        if (filePath == "resources/objects/BlueCoin.png") return RM().GetTexture("blue_coin");
        if (filePath == "resources/objects/RedCoin.png") return RM().GetTexture("red_coin");
        if (filePath == "resources/objects/PooHeart.png") return RM().GetTexture("poo_heart");
        if (filePath == "resources/objects/PooHeartBig.png") return RM().GetTexture("poo_heart_big");
        if (filePath == "resources/objects/PooHeartRainbowBeam.png") return RM().GetTexture("poo_heart_invisible");
        
        // Legacy duplicate mappings for backward compatibility
        if (filePath == "resources/hats/FlowerHatTurdlet.png") return LoadTexture("resources/hats/flowerhatturdletjump.png");
        if (filePath == "resources/hats/FlowerHatTurdletShoot.png") return LoadTexture("resources/hats/flowerhatturdletshoot.png");
        if (filePath == "resources/hats/FlowerHatBigTurd.png") return LoadTexture("resources/hats/flowerhatbigturdjump.png");
        if (filePath == "resources/hats/FlowerHatBigTurdShoot.png") return LoadTexture("resources/hats/flowerhatbigturdshoot.png");
        
        if (filePath == "resources/hats/DooragTurdlet.png") return LoadTexture("resources/hats/dooragturdletjump.png");
        if (filePath == "resources/hats/DooragTurdletShoot.png") return LoadTexture("resources/hats/dooragturdletshoot.png");
        if (filePath == "resources/hats/DooragBigTurd.png") return LoadTexture("resources/hats/dooragbigturdjump.png");
        if (filePath == "resources/hats/DooragBigTurdShoot.png") return LoadTexture("resources/hats/dooragbigturdshoot.png");
        
        // Hat icon mappings (for menu display)
        if (filePath == "resources/hats/Flower.png") return LoadTexture("resources/hats/flowerhat.png");
        if (filePath == "resources/hats/Doorag.png") return LoadTexture("resources/hats/dooraghat.png");
        if (filePath == "resources/hats/SamuraiHat.png") return LoadTexture("resources/hats/SamuraiHelmet.png");
        
        // Environment variants (alternative file names)
        if (filePath == "resources/environment/CactiA.png") return RM().GetTexture("cacti_a");
        if (filePath == "resources/environment/CactiB.png") return RM().GetTexture("cacti_b");
        if (filePath == "resources/environment/CactiC.png") return RM().GetTexture("cacti_c");
        if (filePath == "resources/environment/CactiD.png") return RM().GetTexture("cacti_d");
        if (filePath == "resources/environment/CactiE.png") return RM().GetTexture("cacti_e");
        if (filePath == "resources/environment/CactiBush.png") return RM().GetTexture("cacti_bush");
        
        // Environment - Castle level decorations
        if (filePath == "resources/environment/PaintingA.png") return RM().GetTexture("painting_a");
        if (filePath == "resources/environment/PaintingB.png") return RM().GetTexture("painting_b");
        if (filePath == "resources/environment/PaintingC.png") return RM().GetTexture("painting_c");
        if (filePath == "resources/environment/PaintingD.png") return RM().GetTexture("painting_d");
        if (filePath == "resources/environment/FloorTorch.png") return RM().GetTexture("floor_torch");
        if (filePath == "resources/environment/Chandelier.png") return RM().GetTexture("chandelier");
        
        // Boss level environment
        if (filePath == "resources/environment/BossBackground.png") return RM().GetTexture("boss_background");
        if (filePath == "resources/environment/BossDarkClouds.png") return RM().GetTexture("boss_dark_clouds");
        if (filePath == "resources/environment/BossCurtains.png") return RM().GetTexture("boss_curtains");
        if (filePath == "resources/environment/BossPillar.png") return RM().GetTexture("boss_pillar");
        
        // Enemy sprites - Boss alternative names
        if (filePath == "resources/enemies/RatKingIdle.png") return RM().GetTexture("ratking_idle");
        if (filePath == "resources/enemies/RatKingAimTorso.png") return RM().GetTexture("ratking_aim_torso");
        if (filePath == "resources/enemies/RatKingAimFrontArm.png") return RM().GetTexture("ratking_aim_front_arm");
        if (filePath == "resources/enemies/RatKingAimBackArm.png") return RM().GetTexture("ratking_aim_back_arm");
        
        // Fallback: try loading directly if not found in ResourceManager
        TraceLog(LOG_WARNING, "GetTextureByPath: Unknown path %s, attempting direct load", filePath.c_str());
        return LoadTexture(filePath.c_str());
    }

    // Legacy string constants temporarily disabled to avoid conflicts
    // Will re-enable these as we migrate away from Resources.h
    
    // Sounds (legacy)
    inline const char* Click = "resources/sounds/confirm.ogg";
    inline const char* Hurt = "resources/sounds/hurt.mp3";
    inline const char* GotCoin = "resources/sounds/pickup.ogg";
    inline const char* GotHealth = "resources/sounds/SmallHealthPickup.wav";
    inline const char* GotHealthBig = "resources/sounds/BigHealthPickup.wav";

    // Player sprites (legacy paths) - CRITICAL for Player class InitSprites()
    inline const char* TurdletIdle = "resources/turd/TurdletIdle.png";
    inline const char* TurdletJump = "resources/turd/TurdletJump.png";
    inline const char* TurdletShoot = "resources/turd/TurdletShoot.png";
    inline const char* TurdletHurt = "resources/turd/TurdletHurt.png";
    inline const char* BigTurdIdle = "resources/turd/BigTurdIdle.png";
    inline const char* BigTurdJump = "resources/turd/BigTurdJump.png";
    inline const char* BigTurdShoot = "resources/turd/BigTurdShoot.png";
    inline const char* BigTurdHurt = "resources/turd/BigTurdHurt.png";
    inline const char* TeenageTurdIdle = "resources/turd/TeenageTurdIdle.png";
    inline const char* TeenageTurdJump = "resources/turd/TeenageTurdJump.png";
    inline const char* TeenageTurdShoot = "resources/turd/TeenageTurdShoot.png";
    inline const char* TeenageTurdHurt = "resources/turd/TeenageTurdHurt.png";
    
    // Player projectiles - CRITICAL for Player class Shoot()
    inline const char* PoopSmall = "resources/turd/Floppy Poop.png";
    inline const char* PoopMid = "resources/turd/Floppy Poop Mid.png";
    inline const char* PoopLarge = "resources/turd/Floppy Poop Large.png";

    // Legacy path definitions - keeping original Resources.h compatibility
    // These work alongside ResourceManager functions
    
    // Hat icon constants - CRITICAL for Playing class InitializeHats()
    inline const char* CowboyHat = "resources/hats/cowboyhat.png";
    inline const char* Flower = "resources/hats/flowerhat.png";
    inline const char* Doorag = "resources/hats/dooraghat.png";
    inline const char* Ballcap = "resources/hats/ballcap.png";
    inline const char* PinwheelHat = "resources/hats/PinwheelHat.png";
    inline const char* StrawHat = "resources/hats/strawhat.png";
    inline const char* SamuraiHat = "resources/hats/SamuraiHelmet.png";
    inline const char* TopHat = "resources/hats/tophat.png";
    inline const char* Ushanka = "resources/hats/ushanka.png";
    inline const char* Beret = "resources/hats/Beret.png";
    inline const char* Crown = "resources/hats/Crown.png";
    inline const char* PoopHat = "resources/hats/poophat.png";
    inline const char* RamsesHat = "resources/hats/RamsesHat.png";
    inline const char* SpartanHelmet = "resources/hats/SpartanHelmet.png";
    inline const char* Shell = "resources/hats/shellhat.png";
    
    // Hat variant constants - CRITICAL for Playing class Hat constructors
    // Cowboy Hat variants
    inline const char* CowboyHatTurdlet = "resources/hats/cowboyhatturdletjump.png";
    inline const char* CowboyHatTurdletShoot = "resources/hats/cowboyhatturdletshoot.png";
    inline const char* CowboyHatBigTurd = "resources/hats/cowboyhatbigturdjump.png";
    inline const char* CowboyHatBigTurdShoot = "resources/hats/cowboyhatbigturdshoot.png";
    
    // Flower Hat variants
    inline const char* FlowerHatTurdlet = "resources/hats/flowerhatturdletjump.png";
    inline const char* FlowerHatTurdletShoot = "resources/hats/flowerhatturdletshoot.png";
    inline const char* FlowerHatBigTurd = "resources/hats/flowerhatbigturdjump.png";
    inline const char* FlowerHatBigTurdShoot = "resources/hats/flowerhatbigturdshoot.png";
    
    // Doorag variants
    inline const char* DooragTurdlet = "resources/hats/dooragturdletjump.png";
    inline const char* DooragTurdletShoot = "resources/hats/dooragturdletshoot.png";
    inline const char* DooragBigTurd = "resources/hats/dooragbigturdjump.png";
    inline const char* DooragBigTurdShoot = "resources/hats/dooragbigturdshoot.png";
    
    // Ballcap variants
    inline const char* BallCapTurdlet = "resources/hats/ballcapturdletjump.png";
    inline const char* BallCapTurdletShoot = "resources/hats/ballcapturdletshoot.png";
    inline const char* BallCapBigTurd = "resources/hats/ballcapbigturdjump.png";
    inline const char* BallCapBigTurdShoot = "resources/hats/ballcapbigturdshoot.png";
    
    // Pinwheel variants
    inline const char* PinwheelTurdlet = "resources/hats/pinwheelturdletjump.png";
    inline const char* PinwheelTurdletShoot = "resources/hats/pinwheelturdletshoot.png";
    inline const char* PinwheelBigTurd = "resources/hats/pinwheelbigturdjump.png";
    inline const char* PinwheelBigTurdShoot = "resources/hats/pinwheelbigturdshoot.png";
    
    // Straw Hat variants
    inline const char* StrawHatTurdlet = "resources/hats/strawhatturdletjump.png";
    inline const char* StrawHatTurdletShoot = "resources/hats/strawhatturdletshoot.png";
    inline const char* StrawHatBigTurd = "resources/hats/strawhatbigturdjump.png";
    inline const char* StrawHatBigTurdShoot = "resources/hats/strawhatbigturdshoot.png";
    
    // Samurai Hat variants
    inline const char* SamuraiHatTurdlet = "resources/hats/samuraiturdletjump.png";
    inline const char* SamuraiHatTurdletShoot = "resources/hats/samuraiturdletshoot.png";
    inline const char* SamuraiHatBigTurd = "resources/hats/samuraibigturdjump.png";
    inline const char* SamuraiHatBigTurdShoot = "resources/hats/samuraibigturdshoot.png";
    
    // Top Hat variants
    inline const char* TopHatTurdlet = "resources/hats/tophatturdletjump.png";
    inline const char* TopHatTurdletShoot = "resources/hats/tophatturdletshoot.png";
    inline const char* TopHatBigTurd = "resources/hats/tophatbigturdjump.png";
    inline const char* TopHatBigTurdShoot = "resources/hats/tophatbigturdshoot.png";
    
    // Ushanka variants
    inline const char* UshankaTurdlet = "resources/hats/ushankaturdletjump.png";
    inline const char* UshankaTurdletShoot = "resources/hats/ushankaturdletshoot.png";
    inline const char* UshankaBigTurd = "resources/hats/ushankabigturdjump.png";
    inline const char* UshankaBigTurdShoot = "resources/hats/ushankabigturdshoot.png";
    
    // Beret variants
    inline const char* BeretTurdlet = "resources/hats/berethatturdletjump.png";
    inline const char* BeretTurdletShoot = "resources/hats/berethatturdletshoot.png";
    inline const char* BeretBigTurd = "resources/hats/berethatbigturdjump.png";
    inline const char* BeretBigTurdShoot = "resources/hats/berethatbigturdshoot.png";
    
    // Crown variants
    inline const char* CrownHatTurdlet = "resources/hats/crownhatturdletjump.png";
    inline const char* CrownHatTurdletShoot = "resources/hats/crownhatturdletshoot.png";
    inline const char* CrownHatBigTurd = "resources/hats/crownhatbigturdjump.png";
    inline const char* CrownHatBigTurdShoot = "resources/hats/crownhatbigturdshoot.png";
    
    // Poop Hat variants
    inline const char* PoophatTurdlet = "resources/hats/poophatturdletjump.png";
    inline const char* PoophatTurdletShoot = "resources/hats/poophatturdletshoot.png";
    inline const char* PoophatBigTurd = "resources/hats/poophatbigturdjump.png";
    inline const char* PoophatBigTurdShoot = "resources/hats/poophatbigturdshoot.png";
    
    // Ramses Hat variants
    inline const char* RamsesHatTurdlet = "resources/hats/ramsesturdletjump.png";
    inline const char* RamsesHatTurdletShoot = "resources/hats/ramsesturdletshoot.png";
    inline const char* RamsesHatBigTurd = "resources/hats/ramsesbigturdjump.png";
    inline const char* RamsesHatBigTurdShoot = "resources/hats/ramsesbigturdshoot.png";
    
    // Spartan Hat variants
    inline const char* SpartanHatTurdlet = "resources/hats/spartanhatturdletjump.png";
    inline const char* SpartanHatTurdletShoot = "resources/hats/spartanhatturdletshoot.png";
    inline const char* SpartanHatBigTurd = "resources/hats/spartanhatbigturdjump.png";
    inline const char* SpartanHatBigTurdShoot = "resources/hats/spartanhatbigturdshoot.png";
    
    // Shell Hat variants
    inline const char* ShellHatTurdlet = "resources/hats/shellhatturdletjump.png";
    inline const char* ShellHatTurdletShoot = "resources/hats/shellhatturdletshoot.png";
    inline const char* ShellHatBigTurd = "resources/hats/shellhatbigturdjump.png";
    inline const char* ShellHatBigTurdShoot = "resources/hats/shellhatbigturdshoot.png";
    
    // Snowman enemies (missing from original Resources.h copy)
    inline const char* SnowManRed = "resources/enemies/SnowManIdle.png";
    inline const char* SnowManRedThrow = "resources/enemies/SnowManThrow.png";
    inline const char* SnowManBlue = "resources/enemies/SnowManChill.png";
    inline const char* SnowManGreen = "resources/enemies/SnowManGreen.png";
    inline const char* SnowManChad = "resources/enemies/SnowManChad.png";
    
    // VFX (missing from original Resources.h copy)
    inline const char* BlastSmall = "resources/vfx/blast_small.png";
    inline const char* BlastBig = "resources/vfx/blast_big.png";
    
    // Credits (missing from original Resources.h copy)
    inline const char* CreditsBackgroundTexture = "resources/ui/FloppyTurdCreditsBackground.png";
    
    // Desert Level (missing from original Resources.h copy)
    inline const char* DesertBackgroundCactiLayer = "resources/environment/Cacti.png";
    inline const char* BrickWallTexture = "resources/objects/BrickWall.png";
    
    // Game Over - CRITICAL for Playing class
    inline const char* GameOverMusic = "resources/music/GameOver.mp3";

    // UI Heart variants - CRITICAL for Playing class
    inline const char* TurdHeartSmall = "resources/ui/TurdHeartSmall.png";
    inline const char* TurdHeart0Half = "resources/ui/TurdHeart0Half.png";
    inline const char* TurdHeart0HalfHollow = "resources/ui/TurdHeart0HalfHollow.png";
    inline const char* TurdHeart0ThirdHollow = "resources/ui/TurdHeart0ThirdHollow.png";
    inline const char* TurdHeart1Half = "resources/ui/TurdHeart1Half.png";
    inline const char* TurdHeart1Third = "resources/ui/TurdHeart1Third.png";
    inline const char* TurdHeart2Thirds = "resources/ui/TurdHeart2Thirds.png";

    // Main Menu assets - CRITICAL for MainMenu class
    inline const char* MainMenuBackground = "resources/mainmenu/MainMenu.png";
    inline const char* MainMenuMusic = "resources/mainmenu/FloppyTurdMenu.mp3";
    inline const char* MainMenuMusicAlt = "resources/mainmenu/FloppyTurdMenu Fart Variant.mp3";
    inline const char* FloppyLogo = "resources/mainmenu/FloppyLogo.png";
    inline const char* FinLogo = "resources/mainmenu/F.png";
    inline const char* EmptyPainting = "resources/mainmenu/EmptyPainting.png";
    inline const char* LockedPainting = "resources/mainmenu/LockedPainting.png";
    inline const char* ParkLevelPainting = "resources/mainmenu/ParkLevelPainting.png";
    inline const char* SewerLevelPainting = "resources/mainmenu/SewerLevelPainting.png";
    inline const char* DesertLevelPainting = "resources/mainmenu/DesertLevelPainting.png";
    inline const char* SnowLevelPainting = "resources/mainmenu/SnowLevelPainting.png";
    inline const char* CastleLevelPainting = "resources/mainmenu/CastleLevelPainting.png";
    inline const char* RatKingPainting = "resources/mainmenu/RatKingPainting.png";

    // Level 1 (Park) backgrounds - CRITICAL for ParkLevel class
    inline const char* BackgroundBackLayer = "resources/environment/Level1BackLayerBackground.png";
    inline const char* BackgroundMidLayer = "resources/environment/Level1MidLayerBackground.png";
    inline const char* BackgroundFrontLayer = "resources/environment/Level1FrontLayerBackground.png";
    inline const char* Clouds = "resources/environment/level1Clouds.png";
    inline const char* TopToilet = "resources/environment/TopToilet.png";
    inline const char* BottomToilet = "resources/environment/BottomToilet.png";

    // Level music - CRITICAL for all levels
    inline const char* LevelOne = "resources/music/Level1.mp3";
    inline const char* LevelOneSlow = "resources/music/Level1Slow.ogg";
    inline const char* LevelOneFast = "resources/music/Level1Fast.ogg";
    inline const char* LevelTwo = "resources/music/Level2.mp3";
    inline const char* LevelTwoSlow = "resources/music/Level2Slow.ogg";
    inline const char* LevelTwoFast = "resources/music/Level2Fast.ogg";
    inline const char* LevelThree = "resources/music/Level3.mp3";
    inline const char* LevelThreeSlow = "resources/music/Level3Slow.ogg";
    inline const char* LevelThreeFast = "resources/music/Level3Fast.ogg";
    inline const char* LevelFour = "resources/music/Level4.mp3";
    inline const char* LevelFourSlow = "resources/music/Level4Slow.ogg";
    inline const char* LevelFourFast = "resources/music/Level4Fast.ogg";
    inline const char* LevelFive = "resources/music/Level5.ogg";
    inline const char* SnowLevelMusic = "resources/music/SnowLevel.mp3";
    inline const char* CreditsMusic = "resources/music/EndTheme.ogg";

    // Level 2 (Sewer) assets - CRITICAL for SewerLevel class
    inline const char* BackWallVarA = "resources/environment/sewerwidevarA.png";
    inline const char* BackWallVarB = "resources/environment/sewerwidevarB.png";
    inline const char* BackWallVarC = "resources/environment/sewerwidevarC.png";
    inline const char* BackWallVarD = "resources/environment/sewerwidevarD.png";
    inline const char* TopPipeOrange = "resources/environment/TopPipeWide.png";
    inline const char* TopPipeBlue = "resources/environment/TopPipeWideBlue.png";
    inline const char* BottomPipeOrange = "resources/environment/BottomPipeWide.png";
    inline const char* BottomPipeBlue = "resources/environment/BottomPipeWideBlue.png";
    inline const char* Janitor = "resources/objects/Janitor.png";
    inline const char* JanitorSweep = "resources/objects/JanitorSweep.png";
    inline const char* JanitorSurprise = "resources/objects/JanitorSurprise.png";

    // Level 3 (Desert) assets - CRITICAL for DesertLevel class
    inline const char* DesertBackgroundBackLayer = "resources/environment/Level3BackLayerBackground.png";
    inline const char* DesertBackgroundMidLayer = "resources/environment/Level3MidLayerBackground.png";
    inline const char* DesertBackgroundFrontLayer = "resources/environment/Level3FrontLayerBackground.png";
    inline const char* DancingCacti = "resources/environment/dancingcacti.png";
    inline const char* DancingCactiSmall = "resources/environment/dancingcactismall.png";
    inline const char* DancingCactiCowboy = "resources/environment/dancingcacticowboy.png";
    inline const char* OuthouseSolo = "resources/environment/Outhouse.png";
    inline const char* OuthouseToilet = "resources/environment/OuthouseToilet.png";

    // Level 4 (Snow) assets - CRITICAL for SnowLevel class
    inline const char* SnowBackground = "resources/environment/SnowLevelBackground.png";
    inline const char* SnowMountains = "resources/environment/SnowLevelMountains.png";
    inline const char* SnowBackTrees = "resources/environment/SnowLevelBackTrees.png";
    inline const char* SnowTundra = "resources/environment/SnowLevelTundra.png";
    inline const char* SnowFrontTrees = "resources/environment/SnowLevelFrontTrees.png";
    inline const char* Snowfall = "resources/environment/snow_tile.png";
    inline const char* TopToiletSnow = "resources/environment/TopToiletSnow.png";
    inline const char* BottomToiletSnow = "resources/environment/BottomToiletSnow.png";

    // Level 5 (Castle) assets - CRITICAL for CastleLevel class
    inline const char* CastleBackgroundWall = "resources/environment/castlelevelbackgroundwall.png";
    inline const char* CastleBackgroundBars = "resources/environment/castlelevelfloorceiling.png";
    inline const char* TopToiletGold = "resources/environment/TopToiletGold.png";
    inline const char* BottomToiletGold = "resources/environment/BottomToiletGold.png";
    inline const char* TorchPillar = "resources/environment/TorchPillar.png";
    inline const char* Chandelier = "resources/environment/castlelevelchandelier.png";

    // Pickups - CRITICAL for gameplay
    inline const char* GoldCoin = "resources/objects/GoldCoin.png";
    inline const char* BlueCoin = "resources/objects/BlueCoin.png";
    inline const char* RedCoin = "resources/objects/RedCoin.png";
    inline const char* PooHeart = "resources/objects/PooHeart.png";
    inline const char* PooHeartBig = "resources/objects/PooHeartBig.png";

    // Enemies - CRITICAL for gameplay
    inline const char* BirdIdle = "resources/enemies/BirdIdle.png";
    inline const char* BirdHurt = "resources/enemies/BirdHurt.png";
    inline const char* RatCopterIdle = "resources/enemies/RatCopterIdle.png";
    inline const char* RatCopterHurt = "resources/enemies/RatCopterHurt.png";
    inline const char* RatKingIdle = "resources/enemies/Ratking.png";
    inline const char* RatKingWalk = "resources/enemies/RatkingWalk.png";
    inline const char* RatKingHurt = "resources/enemies/RatkingHurt.png";
    inline const char* RatKingDeath = "resources/enemies/RatkingDeath.png";
    inline const char* RatKingAimTorso = "resources/enemies/RatkingAimTorsoOnly.png";
    inline const char* RatKingAimFrontArm = "resources/enemies/RatkingAimTossArmOnly.png";
    inline const char* RatKingAimBackArm = "resources/enemies/RatkingAimBackArmOnly.png";
    inline const char* ToiletPaperIdle = "resources/enemies/ToiletPaperFlap.png";
    inline const char* ToiletPaperHurt = "resources/enemies/ToiletPaperHit.png";
    inline const char* ToiletPaperProjectilePic = "resources/enemies/ToiletPaperProjectile.png";

    // Spike Ball - CRITICAL for SpikeBall class
    inline const char* SpikeBallTexture = "resources/objects/SpikeBall.png";
    inline const char* SpikeBallBase = "resources/objects/SpikeBallBase.png";

    // Fart sounds - for MainMenu class
    inline const char* fart1 = "resources/sounds/fart1.ogg";
    inline const char* fart2 = "resources/sounds/fart2.ogg";
    inline const char* fart3 = "resources/sounds/fart3.ogg";
    inline const char* fart4 = "resources/sounds/fart4.ogg";
    inline const char* fart5 = "resources/sounds/fart5.ogg";
    inline const char* fart6 = "resources/sounds/fart6.ogg";
    inline const char* fart7 = "resources/sounds/fart7.ogg";
    inline const char* fart8 = "resources/sounds/fart8.ogg";
    inline const char* fart9 = "resources/sounds/fart9.ogg";
    inline const char* fart10 = "resources/sounds/fart10.ogg";
    inline const char* fart11 = "resources/sounds/fart11.ogg";

    // Castle Level decorations - CRITICAL for CastleLevel class
    inline const char* PaintingA = "resources/objects/CabinPainting.png";
    inline const char* PaintingB = "resources/objects/RabbitKnightPainting.png";
    inline const char* PaintingC = "resources/objects/RatBeachPainting.png";
    inline const char* PaintingD = "resources/objects/RiverWalkPainting.png";
    inline const char* Curtains = "resources/environment/curtains.png";
    inline const char* FloorTorch = "resources/environment/castlelevelfloortorch.png";

    // Icon - CRITICAL for Game class
    inline const char* poophatIcon = "resources/poophat.ico";

    // Level 5 music variants - CRITICAL for CastleLevel class
    inline const char* LevelFiveSlow = "resources/music/Level5Slow.ogg";
    inline const char* LevelFiveFast = "resources/music/Level5Fast.ogg";

    // Boss Level assets - CRITICAL for BossLevel class
    inline const char* BossLevelSlow = "resources/music/BossThemeSlow.ogg";
    inline const char* BossLevelFast = "resources/music/BossThemeFast.ogg";
    inline const char* BossLowHealth = "resources/music/BossThemeLowHealth.ogg";
    inline const char* BossBeat = "resources/music/BossBeatv2.mp3";
    inline const char* BossBackground = "resources/environment/ratkingbackground.png";
    inline const char* BossDarkClouds = "resources/environment/darkclouds.png";
    inline const char* BossFloor = "resources/environment/BossFloor.png";
    inline const char* BossCurtains = "resources/environment/screenCurtains.png";
    inline const char* BossWalls = "resources/environment/BossWalls.png";
    inline const char* BossPillar = "resources/environment/bosspillar.png";

    // Cactus variants - CRITICAL for Cactus class
    inline const char* CactiA = "resources/objects/CactiA.png";
    inline const char* CactiB = "resources/objects/CactiB.png";
    inline const char* CactiC = "resources/objects/CactiC.png";
    inline const char* CactiD = "resources/objects/CactiD.png";
    inline const char* CactiE = "resources/objects/CactiE.png";
    inline const char* CactiBush = "resources/objects/CactiBush.png";
} 