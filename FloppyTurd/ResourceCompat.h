#pragma once
#include "ResourceManager.h"
#include "PlatformAPI.h"

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
    inline Texture2D GetMainMenuBackgroundMobile() { return RM().GetTexture("main_menu_bg_mobile"); }
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
    inline const char* PooHeartInvisible = "objects/PooHeartInvisible.png";
    
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
        if (filePath == "turd/TurdletIdle.png") return RM().GetTexture("turdlet_idle");
        if (filePath == "turd/TurdletJump.png") return RM().GetTexture("turdlet_jump");
        if (filePath == "turd/TurdletShoot.png") return RM().GetTexture("turdlet_shoot");
        if (filePath == "turd/TurdletHurt.png") return RM().GetTexture("turdlet_hurt");
        if (filePath == "turd/BigTurdIdle.png") return RM().GetTexture("big_turd_idle");
        if (filePath == "turd/BigTurdJump.png") return RM().GetTexture("big_turd_jump");
        if (filePath == "turd/BigTurdShoot.png") return RM().GetTexture("big_turd_shoot");
        if (filePath == "turd/BigTurdHurt.png") return RM().GetTexture("big_turd_hurt");
        if (filePath == "turd/TeenageTurdIdle.png") return RM().GetTexture("teenage_turd_idle");
        if (filePath == "turd/TeenageTurdJump.png") return RM().GetTexture("teenage_turd_jump");
        if (filePath == "turd/TeenageTurdShoot.png") return RM().GetTexture("teenage_turd_shoot");
        if (filePath == "turd/TeenageTurdHurt.png") return RM().GetTexture("teenage_turd_hurt");
        if (filePath == "turd/Floppy Poop.png") return RM().GetTexture("poop_small");
        if (filePath == "turd/Floppy Poop Mid.png") return RM().GetTexture("poop_mid");
        if (filePath == "turd/Floppy Poop Large.png") return RM().GetTexture("poop_large");
        
        // Janitor sprites
        if (filePath == "objects/Janitor.png") return RM().GetTexture("janitor_idle");
        if (filePath == "objects/JanitorSweep.png") return RM().GetTexture("janitor_sweep");
        if (filePath == "objects/JanitorSurprise.png") return RM().GetTexture("janitor_surprise");
        
        // Cactus variants - CRITICAL for desert level (these were causing warnings)
        if (filePath == "objects/CactiA.png") return RM().GetTexture("cacti_a");
        if (filePath == "objects/CactiB.png") return RM().GetTexture("cacti_b");
        if (filePath == "objects/CactiC.png") return RM().GetTexture("cacti_c");
        if (filePath == "objects/CactiD.png") return RM().GetTexture("cacti_d");
        if (filePath == "objects/CactiE.png") return RM().GetTexture("cacti_e");
        if (filePath == "objects/CactiBush.png") return RM().GetTexture("cacti_bush");
        
        // Desert environment - CRITICAL (these were causing warnings)
        if (filePath == "environment/dancingcactismall.png") return RM().GetTexture("dancing_cacti_small");
        if (filePath == "environment/dancingcacti.png") return RM().GetTexture("dancing_cacti");
        if (filePath == "environment/dancingcacticowboy.png") return RM().GetTexture("dancing_cacti_cowboy");
        
        // Castle environment - CRITICAL (these were causing warnings)
        if (filePath == "environment/curtains.png") return RM().GetTexture("curtains");
        if (filePath == "environment/castlelevelfloortorch.png") return RM().GetTexture("floor_torch");
        if (filePath == "environment/castlelevelchandelier.png") return RM().GetTexture("chandelier");
        if (filePath == "environment/TorchPillar.png") return RM().GetTexture("torch_pillar");
        
        // Boss environment - CRITICAL (these were causing warnings)
        if (filePath == "environment/ratkingbackground.png") return RM().GetTexture("boss_background");
        if (filePath == "environment/darkclouds.png") return RM().GetTexture("boss_dark_clouds");
        if (filePath == "environment/BossFloor.png") return RM().GetTexture("boss_floor");
        if (filePath == "environment/screenCurtains.png") return RM().GetTexture("boss_curtains");
        if (filePath == "environment/BossWalls.png") return RM().GetTexture("boss_walls");
        if (filePath == "environment/bosspillar.png") return RM().GetTexture("boss_pillar");
        
        // Castle paintings - CRITICAL (these were causing warnings)
        if (filePath == "objects/CabinPainting.png") return RM().GetTexture("painting_a");
        if (filePath == "objects/RabbitKnightPainting.png") return RM().GetTexture("painting_b");
        if (filePath == "objects/RatBeachPainting.png") return RM().GetTexture("painting_c");
        if (filePath == "objects/RiverWalkPainting.png") return RM().GetTexture("painting_d");
        
        // Boss sprites - CRITICAL (these were causing warnings)
        if (filePath == "enemies/RatkingAimTorsoOnly.png") return RM().GetTexture("ratking_aim_torso");
        if (filePath == "enemies/RatkingAimTossArmOnly.png") return RM().GetTexture("ratking_aim_front_arm");
        if (filePath == "enemies/RatkingAimBackArmOnly.png") return RM().GetTexture("ratking_aim_back_arm");
        
        // Hat sprites - CRITICAL FIXED! Now using ResourceManager instead of direct LoadTexture calls
        // Hat icons (CRITICAL - these were missing!)
        if (filePath == "hats/cowboyhat.png") return RM().GetTexture("cowboy_hat");
        if (filePath == "hats/flowerhat.png") return RM().GetTexture("flower_hat");
        if (filePath == "hats/dooraghat.png") return RM().GetTexture("doorag_hat");
        if (filePath == "hats/ballcap.png") return RM().GetTexture("ballcap_hat");
        if (filePath == "hats/PinwheelHat.png") return RM().GetTexture("pinwheel_hat");
        if (filePath == "hats/strawhat.png") return RM().GetTexture("straw_hat");
        if (filePath == "hats/SamuraiHelmet.png") return RM().GetTexture("samurai_hat");
        if (filePath == "hats/tophat.png") return RM().GetTexture("top_hat");
        if (filePath == "hats/ushanka.png") return RM().GetTexture("ushanka_hat");
        if (filePath == "hats/Beret.png") return RM().GetTexture("beret_hat");
        if (filePath == "hats/Crown.png") return RM().GetTexture("crown_hat");
        if (filePath == "hats/poophat.png") return RM().GetTexture("poop_hat");
        if (filePath == "hats/RamsesHat.png") return RM().GetTexture("ramses_hat");
        if (filePath == "hats/SpartanHelmet.png") return RM().GetTexture("spartan_hat");
        if (filePath == "hats/shellhat.png") return RM().GetTexture("shell_hat");
        
        // Cowboy Hat sprites
        if (filePath == "hats/cowboyhatturdletjump.png") return RM().GetTexture("cowboy_hat_turdlet_jump");
        if (filePath == "hats/cowboyhatturdletshoot.png") return RM().GetTexture("cowboy_hat_turdlet_shoot");
        if (filePath == "hats/cowboyhatbigturdjump.png") return RM().GetTexture("cowboy_hat_big_jump");
        if (filePath == "hats/cowboyhatbigturdshoot.png") return RM().GetTexture("cowboy_hat_big_shoot");
        
        // Flower Hat sprites
        if (filePath == "hats/flowerhatturdletjump.png") return RM().GetTexture("flower_hat_turdlet_jump");
        if (filePath == "hats/flowerhatturdletshoot.png") return RM().GetTexture("flower_hat_turdlet_shoot");
        if (filePath == "hats/flowerhatbigturdjump.png") return RM().GetTexture("flower_hat_big_jump");
        if (filePath == "hats/flowerhatbigturdshoot.png") return RM().GetTexture("flower_hat_big_shoot");
        
        // Doorag Hat sprites  
        if (filePath == "hats/dooragturdletjump.png") return RM().GetTexture("doorag_hat_turdlet_jump");
        if (filePath == "hats/dooragturdletshoot.png") return RM().GetTexture("doorag_hat_turdlet_shoot");
        if (filePath == "hats/dooragbigturdjump.png") return RM().GetTexture("doorag_hat_big_jump");
        if (filePath == "hats/dooragbigturdshoot.png") return RM().GetTexture("doorag_hat_big_shoot");
        
        // Ball Cap sprites
        if (filePath == "hats/ballcapturdletjump.png") return RM().GetTexture("ballcap_hat_turdlet_jump");
        if (filePath == "hats/ballcapturdletshoot.png") return RM().GetTexture("ballcap_hat_turdlet_shoot");
        if (filePath == "hats/ballcapbigturdjump.png") return RM().GetTexture("ballcap_hat_big_jump");
        if (filePath == "hats/ballcapbigturdshoot.png") return RM().GetTexture("ballcap_hat_big_shoot");
        
        // Pinwheel Hat sprites
        if (filePath == "hats/pinwheelturdletjump.png") return RM().GetTexture("pinwheel_hat_turdlet_jump");
        if (filePath == "hats/pinwheelturdletshoot.png") return RM().GetTexture("pinwheel_hat_turdlet_shoot");
        if (filePath == "hats/pinwheelbigturdjump.png") return RM().GetTexture("pinwheel_hat_big_jump");
        if (filePath == "hats/pinwheelbigturdshoot.png") return RM().GetTexture("pinwheel_hat_big_shoot");
        
        // Straw Hat sprites
        if (filePath == "hats/strawhatturdletjump.png") return RM().GetTexture("straw_hat_turdlet_jump");
        if (filePath == "hats/strawhatturdletshoot.png") return RM().GetTexture("straw_hat_turdlet_shoot");
        if (filePath == "hats/strawhatbigturdjump.png") return RM().GetTexture("straw_hat_big_jump");
        if (filePath == "hats/strawhatbigturdshoot.png") return RM().GetTexture("straw_hat_big_shoot");
        
        // Samurai Hat sprites
        if (filePath == "hats/samuraiturdletjump.png") return RM().GetTexture("samurai_hat_turdlet_jump");
        if (filePath == "hats/samuraiturdletshoot.png") return RM().GetTexture("samurai_hat_turdlet_shoot");
        if (filePath == "hats/samuraibigturdjump.png") return RM().GetTexture("samurai_hat_big_jump");
        if (filePath == "hats/samuraibigturdshoot.png") return RM().GetTexture("samurai_hat_big_shoot");
        
        // Top Hat sprites
        if (filePath == "hats/tophatturdletjump.png") return RM().GetTexture("top_hat_turdlet_jump");
        if (filePath == "hats/tophatturdletshoot.png") return RM().GetTexture("top_hat_turdlet_shoot");
        if (filePath == "hats/tophatbigturdjump.png") return RM().GetTexture("top_hat_big_jump");
        if (filePath == "hats/tophatbigturdshoot.png") return RM().GetTexture("top_hat_big_shoot");
        
        // Ushanka sprites
        if (filePath == "hats/ushankaturdletjump.png") return RM().GetTexture("ushanka_hat_turdlet_jump");
        if (filePath == "hats/ushankaturdletshoot.png") return RM().GetTexture("ushanka_hat_turdlet_shoot");
        if (filePath == "hats/ushankabigturdjump.png") return RM().GetTexture("ushanka_hat_big_jump");
        if (filePath == "hats/ushankabigturdshoot.png") return RM().GetTexture("ushanka_hat_big_shoot");
        
        // Beret sprites
        if (filePath == "hats/berethatturdletjump.png") return RM().GetTexture("beret_hat_turdlet_jump");
        if (filePath == "hats/berethatturdletshoot.png") return RM().GetTexture("beret_hat_turdlet_shoot");
        if (filePath == "hats/berethatbigturdjump.png") return RM().GetTexture("beret_hat_big_jump");
        if (filePath == "hats/berethatbigturdshoot.png") return RM().GetTexture("beret_hat_big_shoot");
        
        // Crown sprites
        if (filePath == "hats/crownhatturdletjump.png") return RM().GetTexture("crown_hat_turdlet_jump");
        if (filePath == "hats/crownhatturdletshoot.png") return RM().GetTexture("crown_hat_turdlet_shoot");
        if (filePath == "hats/crownhatbigturdjump.png") return RM().GetTexture("crown_hat_big_jump");
        if (filePath == "hats/crownhatbigturdshoot.png") return RM().GetTexture("crown_hat_big_shoot");
        
        // Poop Hat sprites
        if (filePath == "hats/poophatturdletjump.png") return RM().GetTexture("poop_hat_turdlet_jump");
        if (filePath == "hats/poophatturdletshoot.png") return RM().GetTexture("poop_hat_turdlet_shoot");
        if (filePath == "hats/poophatbigturdjump.png") return RM().GetTexture("poop_hat_big_jump");
        if (filePath == "hats/poophatbigturdshoot.png") return RM().GetTexture("poop_hat_big_shoot");
        
        // Ramses Hat sprites
        if (filePath == "hats/ramsesturdletjump.png") return RM().GetTexture("ramses_hat_turdlet_jump");
        if (filePath == "hats/ramsesturdletshoot.png") return RM().GetTexture("ramses_hat_turdlet_shoot");
        if (filePath == "hats/ramsesbigturdjump.png") return RM().GetTexture("ramses_hat_big_jump");
        if (filePath == "hats/ramsesbigturdshoot.png") return RM().GetTexture("ramses_hat_big_shoot");
        
        // Spartan Hat sprites
        if (filePath == "hats/spartanhatturdletjump.png") return RM().GetTexture("spartan_hat_turdlet_jump");
        if (filePath == "hats/spartanhatturdletshoot.png") return RM().GetTexture("spartan_hat_turdlet_shoot");
        if (filePath == "hats/spartanhatbigturdjump.png") return RM().GetTexture("spartan_hat_big_jump");
        if (filePath == "hats/spartanhatbigturdshoot.png") return RM().GetTexture("spartan_hat_big_shoot");
        
        // Shell Hat sprites
        if (filePath == "hats/shellhatturdletjump.png") return RM().GetTexture("shell_hat_turdlet_jump");
        if (filePath == "hats/shellhatturdletshoot.png") return RM().GetTexture("shell_hat_turdlet_shoot");
        if (filePath == "hats/shellhatbigturdjump.png") return RM().GetTexture("shell_hat_big_jump");
        if (filePath == "hats/shellhatbigturdshoot.png") return RM().GetTexture("shell_hat_big_shoot");
        
        // Enemy sprites
        if (filePath == "enemies/BirdIdle.png") return RM().GetTexture("bird_idle");
        if (filePath == "enemies/BirdHurt.png") return RM().GetTexture("bird_hurt");
        if (filePath == "enemies/SnowManIdle.png") return RM().GetTexture("snowman_red");
        if (filePath == "enemies/SnowManThrow.png") return RM().GetTexture("snowman_red_throw");
        if (filePath == "enemies/SnowManChill.png") return RM().GetTexture("snowman_blue");
        if (filePath == "enemies/SnowManGreen.png") return RM().GetTexture("snowman_green");
        if (filePath == "enemies/SnowManChad.png") return RM().GetTexture("snowman_chad");
        if (filePath == "enemies/RatCopterIdle.png") return RM().GetTexture("ratcopter_idle");
        if (filePath == "enemies/RatCopterHurt.png") return RM().GetTexture("ratcopter_hurt");
        
        // Boss sprites
        if (filePath == "enemies/Ratking.png") return RM().GetTexture("ratking_idle");
        if (filePath == "enemies/RatkingWalk.png") return RM().GetTexture("ratking_walk");
        if (filePath == "enemies/RatkingHurt.png") return RM().GetTexture("ratking_hurt");
        if (filePath == "enemies/RatkingDeath.png") return RM().GetTexture("ratking_death");
        
        // Pickups
        if (filePath == "objects/GoldCoin.png") return RM().GetTexture("gold_coin");
        if (filePath == "objects/BlueCoin.png") return RM().GetTexture("blue_coin");
        if (filePath == "objects/RedCoin.png") return RM().GetTexture("red_coin");
        if (filePath == "objects/PooHeart.png") return RM().GetTexture("poo_heart");
        if (filePath == "objects/PooHeartBig.png") return RM().GetTexture("poo_heart_big");
        if (filePath == "objects/PooHeartRainbowBeam.png") return RM().GetTexture("poo_heart_invisible");
        
        // Legacy duplicate mappings for backward compatibility
        if (filePath == "hats/FlowerHatTurdlet.png") return LoadTexture("hats/flowerhatturdletjump.png");
        if (filePath == "hats/FlowerHatTurdletShoot.png") return LoadTexture("hats/flowerhatturdletshoot.png");
        if (filePath == "hats/FlowerHatBigTurd.png") return LoadTexture("hats/flowerhatbigturdjump.png");
        if (filePath == "hats/FlowerHatBigTurdShoot.png") return LoadTexture("hats/flowerhatbigturdshoot.png");
        
        if (filePath == "hats/DooragTurdlet.png") return LoadTexture("hats/dooragturdletjump.png");
        if (filePath == "hats/DooragTurdletShoot.png") return LoadTexture("hats/dooragturdletshoot.png");
        if (filePath == "hats/DooragBigTurd.png") return LoadTexture("hats/dooragbigturdjump.png");
        if (filePath == "hats/DooragBigTurdShoot.png") return LoadTexture("hats/dooragbigturdshoot.png");
        
        // Hat icon mappings (for menu display)
        if (filePath == "hats/Flower.png") return LoadTexture("hats/flowerhat.png");
        if (filePath == "hats/Doorag.png") return LoadTexture("hats/dooraghat.png");
        if (filePath == "hats/SamuraiHat.png") return LoadTexture("hats/SamuraiHelmet.png");
        
        // Environment variants (alternative file names)
        if (filePath == "environment/CactiA.png") return RM().GetTexture("cacti_a");
        if (filePath == "environment/CactiB.png") return RM().GetTexture("cacti_b");
        if (filePath == "environment/CactiC.png") return RM().GetTexture("cacti_c");
        if (filePath == "environment/CactiD.png") return RM().GetTexture("cacti_d");
        if (filePath == "environment/CactiE.png") return RM().GetTexture("cacti_e");
        if (filePath == "environment/CactiBush.png") return RM().GetTexture("cacti_bush");
        
        // Environment - Castle level decorations
        if (filePath == "environment/PaintingA.png") return RM().GetTexture("painting_a");
        if (filePath == "environment/PaintingB.png") return RM().GetTexture("painting_b");
        if (filePath == "environment/PaintingC.png") return RM().GetTexture("painting_c");
        if (filePath == "environment/PaintingD.png") return RM().GetTexture("painting_d");
        if (filePath == "environment/FloorTorch.png") return RM().GetTexture("floor_torch");
        if (filePath == "environment/Chandelier.png") return RM().GetTexture("chandelier");
        
        // Boss level environment
        if (filePath == "environment/BossBackground.png") return RM().GetTexture("boss_background");
        if (filePath == "environment/BossDarkClouds.png") return RM().GetTexture("boss_dark_clouds");
        if (filePath == "environment/BossCurtains.png") return RM().GetTexture("boss_curtains");
        if (filePath == "environment/BossPillar.png") return RM().GetTexture("boss_pillar");
        
        // Enemy sprites - Boss alternative names
        if (filePath == "enemies/RatKingIdle.png") return RM().GetTexture("ratking_idle");
        if (filePath == "enemies/RatKingAimTorso.png") return RM().GetTexture("ratking_aim_torso");
        if (filePath == "enemies/RatKingAimFrontArm.png") return RM().GetTexture("ratking_aim_front_arm");
        if (filePath == "enemies/RatKingAimBackArm.png") return RM().GetTexture("ratking_aim_back_arm");
        
        // Fallback: try loading directly if not found in ResourceManager
        TraceLog(LOG_WARNING, "GetTextureByPath: Unknown path %s, attempting direct load", filePath.c_str());
        return LoadTexture(filePath.c_str());
    }

    // Legacy string constants temporarily disabled to avoid conflicts
    // Will re-enable these as we migrate away from Resources.h
    
    // Sounds (legacy)
    inline const char* Click = "sounds/confirm.ogg";
    inline const char* Hurt = "sounds/hurt.mp3";
    inline const char* GotCoin = "sounds/pickup.ogg";
    inline const char* GotHealth = "sounds/SmallHealthPickup.wav";
    inline const char* GotHealthBig = "sounds/BigHealthPickup.wav";

    // Player sprites (legacy paths) - CRITICAL for Player class InitSprites()
    inline const char* TurdletIdle = "turd/TurdletIdle.png";
    inline const char* TurdletJump = "turd/TurdletJump.png";
    inline const char* TurdletShoot = "turd/TurdletShoot.png";
    inline const char* TurdletHurt = "turd/TurdletHurt.png";
    inline const char* BigTurdIdle = "turd/BigTurdIdle.png";
    inline const char* BigTurdJump = "turd/BigTurdJump.png";
    inline const char* BigTurdShoot = "turd/BigTurdShoot.png";
    inline const char* BigTurdHurt = "turd/BigTurdHurt.png";
    inline const char* TeenageTurdIdle = "turd/TeenageTurdIdle.png";
    inline const char* TeenageTurdJump = "turd/TeenageTurdJump.png";
    inline const char* TeenageTurdShoot = "turd/TeenageTurdShoot.png";
    inline const char* TeenageTurdHurt = "turd/TeenageTurdHurt.png";
    
    // Player projectiles - CRITICAL for Player class Shoot()
    inline const char* PoopSmall = "turd/Floppy Poop.png";
    inline const char* PoopMid = "turd/Floppy Poop Mid.png";
    inline const char* PoopLarge = "turd/Floppy Poop Large.png";

    // Legacy path definitions - keeping original Resources.h compatibility
    // These work alongside ResourceManager functions
    
    // Hat icon constants - CRITICAL for Playing class InitializeHats()
    inline const char* CowboyHat = "hats/cowboyhat.png";
    inline const char* Flower = "hats/flowerhat.png";
    inline const char* Doorag = "hats/dooraghat.png";
    inline const char* Ballcap = "hats/ballcap.png";
    inline const char* PinwheelHat = "hats/PinwheelHat.png";
    inline const char* StrawHat = "hats/strawhat.png";
    inline const char* SamuraiHat = "hats/SamuraiHelmet.png";
    inline const char* TopHat = "hats/tophat.png";
    inline const char* Ushanka = "hats/ushanka.png";
    inline const char* Beret = "hats/Beret.png";
    inline const char* Crown = "hats/Crown.png";
    inline const char* PoopHat = "hats/poophat.png";
    inline const char* RamsesHat = "hats/RamsesHat.png";
    inline const char* SpartanHelmet = "hats/SpartanHelmet.png";
    inline const char* Shell = "hats/shellhat.png";
    
    // Hat variant constants - CRITICAL for Playing class Hat constructors
    // Cowboy Hat variants
    inline const char* CowboyHatTurdlet = "hats/cowboyhatturdletjump.png";
    inline const char* CowboyHatTurdletShoot = "hats/cowboyhatturdletshoot.png";
    inline const char* CowboyHatBigTurd = "hats/cowboyhatbigturdjump.png";
    inline const char* CowboyHatBigTurdShoot = "hats/cowboyhatbigturdshoot.png";
    
    // Flower Hat variants
    inline const char* FlowerHatTurdlet = "hats/flowerhatturdletjump.png";
    inline const char* FlowerHatTurdletShoot = "hats/flowerhatturdletshoot.png";
    inline const char* FlowerHatBigTurd = "hats/flowerhatbigturdjump.png";
    inline const char* FlowerHatBigTurdShoot = "hats/flowerhatbigturdshoot.png";
    
    // Doorag variants
    inline const char* DooragTurdlet = "hats/dooragturdletjump.png";
    inline const char* DooragTurdletShoot = "hats/dooragturdletshoot.png";
    inline const char* DooragBigTurd = "hats/dooragbigturdjump.png";
    inline const char* DooragBigTurdShoot = "hats/dooragbigturdshoot.png";
    
    // Ballcap variants
    inline const char* BallCapTurdlet = "hats/ballcapturdletjump.png";
    inline const char* BallCapTurdletShoot = "hats/ballcapturdletshoot.png";
    inline const char* BallCapBigTurd = "hats/ballcapbigturdjump.png";
    inline const char* BallCapBigTurdShoot = "hats/ballcapbigturdshoot.png";
    
    // Pinwheel variants
    inline const char* PinwheelTurdlet = "hats/pinwheelturdletjump.png";
    inline const char* PinwheelTurdletShoot = "hats/pinwheelturdletshoot.png";
    inline const char* PinwheelBigTurd = "hats/pinwheelbigturdjump.png";
    inline const char* PinwheelBigTurdShoot = "hats/pinwheelbigturdshoot.png";
    
    // Straw Hat variants
    inline const char* StrawHatTurdlet = "hats/strawhatturdletjump.png";
    inline const char* StrawHatTurdletShoot = "hats/strawhatturdletshoot.png";
    inline const char* StrawHatBigTurd = "hats/strawhatbigturdjump.png";
    inline const char* StrawHatBigTurdShoot = "hats/strawhatbigturdshoot.png";
    
    // Samurai Hat variants
    inline const char* SamuraiHatTurdlet = "hats/samuraiturdletjump.png";
    inline const char* SamuraiHatTurdletShoot = "hats/samuraiturdletshoot.png";
    inline const char* SamuraiHatBigTurd = "hats/samuraibigturdjump.png";
    inline const char* SamuraiHatBigTurdShoot = "hats/samuraibigturdshoot.png";
    
    // Top Hat variants
    inline const char* TopHatTurdlet = "hats/tophatturdletjump.png";
    inline const char* TopHatTurdletShoot = "hats/tophatturdletshoot.png";
    inline const char* TopHatBigTurd = "hats/tophatbigturdjump.png";
    inline const char* TopHatBigTurdShoot = "hats/tophatbigturdshoot.png";
    
    // Ushanka variants
    inline const char* UshankaTurdlet = "hats/ushankaturdletjump.png";
    inline const char* UshankaTurdletShoot = "hats/ushankaturdletshoot.png";
    inline const char* UshankaBigTurd = "hats/ushankabigturdjump.png";
    inline const char* UshankaBigTurdShoot = "hats/ushankabigturdshoot.png";
    
    // Beret variants
    inline const char* BeretTurdlet = "hats/berethatturdletjump.png";
    inline const char* BeretTurdletShoot = "hats/berethatturdletshoot.png";
    inline const char* BeretBigTurd = "hats/berethatbigturdjump.png";
    inline const char* BeretBigTurdShoot = "hats/berethatbigturdshoot.png";
    
    // Crown variants
    inline const char* CrownHatTurdlet = "hats/crownhatturdletjump.png";
    inline const char* CrownHatTurdletShoot = "hats/crownhatturdletshoot.png";
    inline const char* CrownHatBigTurd = "hats/crownhatbigturdjump.png";
    inline const char* CrownHatBigTurdShoot = "hats/crownhatbigturdshoot.png";
    
    // Poop Hat variants
    inline const char* PoophatTurdlet = "hats/poophatturdletjump.png";
    inline const char* PoophatTurdletShoot = "hats/poophatturdletshoot.png";
    inline const char* PoophatBigTurd = "hats/poophatbigturdjump.png";
    inline const char* PoophatBigTurdShoot = "hats/poophatbigturdshoot.png";
    
    // Ramses Hat variants
    inline const char* RamsesHatTurdlet = "hats/ramsesturdletjump.png";
    inline const char* RamsesHatTurdletShoot = "hats/ramsesturdletshoot.png";
    inline const char* RamsesHatBigTurd = "hats/ramsesbigturdjump.png";
    inline const char* RamsesHatBigTurdShoot = "hats/ramsesbigturdshoot.png";
    
    // Spartan Hat variants
    inline const char* SpartanHatTurdlet = "hats/spartanhatturdletjump.png";
    inline const char* SpartanHatTurdletShoot = "hats/spartanhatturdletshoot.png";
    inline const char* SpartanHatBigTurd = "hats/spartanhatbigturdjump.png";
    inline const char* SpartanHatBigTurdShoot = "hats/spartanhatbigturdshoot.png";
    
    // Shell Hat variants
    inline const char* ShellHatTurdlet = "hats/shellhatturdletjump.png";
    inline const char* ShellHatTurdletShoot = "hats/shellhatturdletshoot.png";
    inline const char* ShellHatBigTurd = "hats/shellhatbigturdjump.png";
    inline const char* ShellHatBigTurdShoot = "hats/shellhatbigturdshoot.png";
    
    // Snowman enemies (missing from original Resources.h copy)
    inline const char* SnowManRed = "enemies/SnowManIdle.png";
    inline const char* SnowManRedThrow = "enemies/SnowManThrow.png";
    inline const char* SnowManBlue = "enemies/SnowManChill.png";
    inline const char* SnowManGreen = "enemies/SnowManGreen.png";
    inline const char* SnowManChad = "enemies/SnowManChad.png";
    
    // VFX (missing from original Resources.h copy)
    inline const char* BlastSmall = "vfx/blast_small.png";
    inline const char* BlastBig = "vfx/blast_big.png";
    
    // Credits (missing from original Resources.h copy)
    inline const char* CreditsBackgroundTexture = "ui/FloppyTurdCreditsBackground.png";
    
    // Desert Level (missing from original Resources.h copy)
    inline const char* DesertBackgroundCactiLayer = "environment/Cacti.png";
    inline const char* BrickWallTexture = "objects/BrickWall.png";
    
    // Game Over - CRITICAL for Playing class
    inline const char* GameOverMusic = "music/GameOver.mp3";

    // UI Heart variants - CRITICAL for Playing class
    inline const char* TurdHeartSmall = "ui/TurdHeartSmall.png";
    inline const char* TurdHeart0Half = "ui/TurdHeart0Half.png";
    inline const char* TurdHeart0HalfHollow = "ui/TurdHeart0HalfHollow.png";
    inline const char* TurdHeart0ThirdHollow = "ui/TurdHeart0ThirdHollow.png";
    inline const char* TurdHeart1Half = "ui/TurdHeart1Half.png";
    inline const char* TurdHeart1Third = "ui/TurdHeart1Third.png";
    inline const char* TurdHeart2Thirds = "ui/TurdHeart2Thirds.png";

    // Main Menu assets - CRITICAL for MainMenu class
    inline const char* MainMenuBackground = "mainmenu/MainMenu.png";
    inline const char* MainMenuMusic = "mainmenu/FloppyTurdMenu.mp3";
    inline const char* MainMenuMusicAlt = "mainmenu/FloppyTurdMenu Fart Variant.mp3";
    inline const char* FloppyLogo = "mainmenu/FloppyLogo.png";
    inline const char* FinLogo = "mainmenu/F.png";
    inline const char* EmptyPainting = "mainmenu/EmptyPainting.png";
    inline const char* LockedPainting = "mainmenu/LockedPainting.png";
    inline const char* ParkLevelPainting = "mainmenu/ParkLevelPainting.png";
    inline const char* SewerLevelPainting = "mainmenu/SewerLevelPainting.png";
    inline const char* DesertLevelPainting = "mainmenu/DesertLevelPainting.png";
    inline const char* SnowLevelPainting = "mainmenu/SnowLevelPainting.png";
    inline const char* CastleLevelPainting = "mainmenu/CastleLevelPainting.png";
    inline const char* RatKingPainting = "mainmenu/RatKingPainting.png";

    // Level 1 (Park) backgrounds - CRITICAL for ParkLevel class
    inline const char* BackgroundBackLayer = "environment/Level1BackLayerBackground.png";
    inline const char* BackgroundMidLayer = "environment/Level1MidLayerBackground.png";
    inline const char* BackgroundFrontLayer = "environment/Level1FrontLayerBackground.png";
    inline const char* Clouds = "environment/level1Clouds.png";
    inline const char* TopToilet = "environment/TopToilet.png";
    inline const char* BottomToilet = "environment/BottomToilet.png";

    // Level music - CRITICAL for all levels
    inline const char* LevelOne = "music/Level1.mp3";
    inline const char* LevelOneSlow = "music/Level1Slow.ogg";
    inline const char* LevelOneFast = "music/Level1Fast.ogg";
    inline const char* LevelTwo = "music/Level2.mp3";
    inline const char* LevelTwoSlow = "music/Level2Slow.ogg";
    inline const char* LevelTwoFast = "music/Level2Fast.ogg";
    inline const char* LevelThree = "music/Level3.mp3";
    inline const char* LevelThreeSlow = "music/Level3Slow.ogg";
    inline const char* LevelThreeFast = "music/Level3Fast.ogg";
    inline const char* LevelFour = "music/Level4.mp3";
    inline const char* LevelFourSlow = "music/Level4Slow.ogg";
    inline const char* LevelFourFast = "music/Level4Fast.ogg";
    inline const char* LevelFive = "music/Level5.ogg";
    inline const char* SnowLevelMusic = "music/SnowLevel.mp3";
    inline const char* CreditsMusic = "music/EndTheme.ogg";

    // Level 2 (Sewer) assets - CRITICAL for SewerLevel class
    inline const char* BackWallVarA = "environment/sewerwidevarA.png";
    inline const char* BackWallVarB = "environment/sewerwidevarB.png";
    inline const char* BackWallVarC = "environment/sewerwidevarC.png";
    inline const char* BackWallVarD = "environment/sewerwidevarD.png";
    inline const char* TopPipeOrange = "environment/TopPipeWide.png";
    inline const char* TopPipeBlue = "environment/TopPipeWideBlue.png";
    inline const char* BottomPipeOrange = "environment/BottomPipeWide.png";
    inline const char* BottomPipeBlue = "environment/BottomPipeWideBlue.png";
    inline const char* Janitor = "objects/Janitor.png";
    inline const char* JanitorSweep = "objects/JanitorSweep.png";
    inline const char* JanitorSurprise = "objects/JanitorSurprise.png";

    // Level 3 (Desert) assets - CRITICAL for DesertLevel class
    inline const char* DesertBackgroundBackLayer = "environment/Level3BackLayerBackground.png";
    inline const char* DesertBackgroundMidLayer = "environment/Level3MidLayerBackground.png";
    inline const char* DesertBackgroundFrontLayer = "environment/Level3FrontLayerBackground.png";
    inline const char* DancingCacti = "environment/dancingcacti.png";
    inline const char* DancingCactiSmall = "environment/dancingcactismall.png";
    inline const char* DancingCactiCowboy = "environment/dancingcacticowboy.png";
    inline const char* OuthouseSolo = "environment/Outhouse.png";
    inline const char* OuthouseToilet = "environment/OuthouseToilet.png";

    // Level 4 (Snow) assets - CRITICAL for SnowLevel class
    inline const char* SnowBackground = "environment/SnowLevelBackground.png";
    inline const char* SnowMountains = "environment/SnowLevelMountains.png";
    inline const char* SnowBackTrees = "environment/SnowLevelBackTrees.png";
    inline const char* SnowTundra = "environment/SnowLevelTundra.png";
    inline const char* SnowFrontTrees = "environment/SnowLevelFrontTrees.png";
    inline const char* Snowfall = "environment/snow_tile.png";
    inline const char* TopToiletSnow = "environment/TopToiletSnow.png";
    inline const char* BottomToiletSnow = "environment/BottomToiletSnow.png";

    // Level 5 (Castle) assets - CRITICAL for CastleLevel class
    inline const char* CastleBackgroundWall = "environment/castlelevelbackgroundwall.png";
    inline const char* CastleBackgroundBars = "environment/castlelevelfloorceiling.png";
    inline const char* TopToiletGold = "environment/TopToiletGold.png";
    inline const char* BottomToiletGold = "environment/BottomToiletGold.png";
    inline const char* TorchPillar = "environment/TorchPillar.png";
    inline const char* Chandelier = "environment/castlelevelchandelier.png";

    // Pickups - CRITICAL for gameplay
    inline const char* GoldCoin = "objects/GoldCoin.png";
    inline const char* BlueCoin = "objects/BlueCoin.png";
    inline const char* RedCoin = "objects/RedCoin.png";
    inline const char* PooHeart = "objects/PooHeart.png";
    inline const char* PooHeartBig = "objects/PooHeartBig.png";

    // Enemies - CRITICAL for gameplay
    inline const char* BirdIdle = "enemies/BirdIdle.png";
    inline const char* BirdHurt = "enemies/BirdHurt.png";
    inline const char* RatCopterIdle = "enemies/RatCopterIdle.png";
    inline const char* RatCopterHurt = "enemies/RatCopterHurt.png";
    inline const char* RatKingIdle = "enemies/Ratking.png";
    inline const char* RatKingWalk = "enemies/RatkingWalk.png";
    inline const char* RatKingHurt = "enemies/RatkingHurt.png";
    inline const char* RatKingDeath = "enemies/RatkingDeath.png";
    inline const char* RatKingAimTorso = "enemies/RatkingAimTorsoOnly.png";
    inline const char* RatKingAimFrontArm = "enemies/RatkingAimTossArmOnly.png";
    inline const char* RatKingAimBackArm = "enemies/RatkingAimBackArmOnly.png";
    inline const char* ToiletPaperIdle = "enemies/ToiletPaperFlap.png";
    inline const char* ToiletPaperHurt = "enemies/ToiletPaperHit.png";
    inline const char* ToiletPaperProjectilePic = "enemies/ToiletPaperProjectile.png";

    // Spike Ball - CRITICAL for SpikeBall class
    inline const char* SpikeBallTexture = "objects/SpikeBall.png";
    inline const char* SpikeBallBase = "objects/SpikeBallBase.png";

    // Fart sounds - for MainMenu class
    inline const char* fart1 = "sounds/fart1.ogg";
    inline const char* fart2 = "sounds/fart2.ogg";
    inline const char* fart3 = "sounds/fart3.ogg";
    inline const char* fart4 = "sounds/fart4.ogg";
    inline const char* fart5 = "sounds/fart5.ogg";
    inline const char* fart6 = "sounds/fart6.ogg";
    inline const char* fart7 = "sounds/fart7.ogg";
    inline const char* fart8 = "sounds/fart8.ogg";
    inline const char* fart9 = "sounds/fart9.ogg";
    inline const char* fart10 = "sounds/fart10.ogg";
    inline const char* fart11 = "sounds/fart11.ogg";

    // Castle Level decorations - CRITICAL for CastleLevel class
    inline const char* PaintingA = "objects/CabinPainting.png";
    inline const char* PaintingB = "objects/RabbitKnightPainting.png";
    inline const char* PaintingC = "objects/RatBeachPainting.png";
    inline const char* PaintingD = "objects/RiverWalkPainting.png";
    inline const char* Curtains = "environment/curtains.png";
    inline const char* FloorTorch = "environment/castlelevelfloortorch.png";

    // Icon - CRITICAL for Game class
    inline const char* poophatIcon = "poophat.ico";

    // Level 5 music variants - CRITICAL for CastleLevel class
    inline const char* LevelFiveSlow = "music/Level5Slow.ogg";
    inline const char* LevelFiveFast = "music/Level5Fast.ogg";

    // Boss Level assets - CRITICAL for BossLevel class
    inline const char* BossLevelSlow = "music/BossThemeSlow.ogg";
    inline const char* BossLevelFast = "music/BossThemeFast.ogg";
    inline const char* BossLowHealth = "music/BossThemeLowHealth.ogg";
    inline const char* BossBeat = "music/BossBeatv2.mp3";
    inline const char* BossBackground = "environment/ratkingbackground.png";
    inline const char* BossDarkClouds = "environment/darkclouds.png";
    inline const char* BossFloor = "environment/BossFloor.png";
    inline const char* BossCurtains = "environment/screenCurtains.png";
    inline const char* BossWalls = "environment/BossWalls.png";
    inline const char* BossPillar = "environment/bosspillar.png";

    // Cactus variants - CRITICAL for Cactus class
    inline const char* CactiA = "objects/CactiA.png";
    inline const char* CactiB = "objects/CactiB.png";
    inline const char* CactiC = "objects/CactiC.png";
    inline const char* CactiD = "objects/CactiD.png";
    inline const char* CactiE = "objects/CactiE.png";
    inline const char* CactiBush = "objects/CactiBush.png";
} 