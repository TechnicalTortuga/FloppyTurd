# FloppyTurd Asset Structure

## Current Asset Organization (src/Assets/)

### Directory Structure
```
src/Assets/
├── enemies/
│   ├── BirdHurt.png
│   ├── BirdIdle.png
│   ├── RatCopterHurt.png
│   ├── RatCopterIdle.png
│   ├── Ratking.png
│   ├── RatkingAimBackArmOnly.png
│   ├── RatkingAimTorsoOnly.png
│   ├── RatkingAimTossArmOnly.png
│   ├── RatkingDeath.png
│   ├── RatkingHurt.png
│   ├── RatkingWalk.png
│   ├── SnowManChad.png
│   ├── SnowManChill.png
│   ├── SnowManGreen.png
│   ├── SnowManIdle.png
│   ├── SnowManThrow.png
│   ├── SnowmanIdle.gif
│   ├── ToiletPaperFlap.png
│   ├── ToiletPaperHit.png
│   └── toiletpaperprojectile.png
├── environment/
│   ├── BossFloor.png
│   ├── BossWalls.png
│   ├── BottomPipeWide.png
│   ├── BottomPipeWideBlue.png
│   ├── BottomToilet.png
│   ├── BottomToiletGold-export.png
│   ├── BottomToiletGold.png
│   ├── BottomToiletSnow.png
│   ├── Cacti.png
│   ├── Level1BackLayerBackground.png
│   ├── Level1Clouds.png
│   ├── Level1FrontLayerBackground.png
│   ├── Level1MidLayerBackground.png
│   ├── Level3BackLayerBackground.png
│   ├── Level3FrontLayerBackground.png
│   ├── Level3MidLayerBackground.png
│   ├── Outhouse.png
│   ├── OuthouseToilet.png
│   ├── SewerBackgroundRunningWater.png
│   ├── SnowLevelBackTrees.png
│   ├── SnowLevelBackground.png
│   ├── SnowLevelFrontTrees.png
│   ├── SnowLevelMountains.png
│   ├── SnowLevelTundra.png
│   ├── Snowfall.png
│   ├── TopPipeWide.png
│   ├── TopPipeWideBlue.png
│   ├── TopToilet.png
│   └── TopToiletSnow.png
├── fonts/
│   ├── Whacky_Joe.fnt
│   └── Whacky_Joe.png
├── hats/ (77 files)
│   ├── Beret.png
│   ├── PinwheelHat.png
│   ├── RamsesHat.png
│   ├── SamuraiHelmet.png
│   ├── SpartanHelmet.png
│   ├── ballcap.png
│   ├── cowboyhat.png
│   ├── crown.png
│   ├── dooraghat.png
│   ├── flowerhat.png
│   ├── poophat.png
│   ├── shellhat.png
│   ├── strawhat.png
│   ├── tophat.png
│   ├── ushanka.png
│   └── [62 more hat variations with character states]
├── mainmenu/ (12 files)
│   ├── CastleLevelPainting.png
│   ├── DesertLevelPainting.png
│   ├── EmptyPainting.png
│   ├── F.png
│   ├── FloppyLogo.png
│   ├── LockedPainting.png
│   ├── MainMenu.png
│   ├── MainMenuMobile.png
│   ├── ParkLevelPainting.png
│   ├── RatKingPainting.png
│   ├── SewerLevelPainting.png
│   └── SnowLevelPainting.png
├── music/ (29 files)
│   ├── BossMusic.mp3
│   ├── DeathJingle.mp3
│   ├── GameMusic.mp3
│   ├── GameMusicLevel1-20.mp3 (20 level tracks)
│   ├── MainMenuMusic.mp3
│   ├── PauseMenuMusic.mp3
│   ├── SettingsMenuMusic.mp3
│   ├── ShopMenuMusic.mp3
│   ├── VictoryJingle.mp3
│   └── WinMusic.mp3
├── objects/ (26 files)
│   ├── BlueCoin.png
│   ├── BrickWall.png
│   ├── CabinPainting.gif
│   ├── CabinPainting.png
│   ├── CactiA-E.png (5 cactus variants)
│   ├── GoldCoin.png
│   ├── RedCoin.png
│   ├── Janitor.png
│   ├── JanitorSurprise.png
│   ├── JanitorSweep.png
│   ├── PooHeart variants (4 files)
│   ├── Painting files (3 files)
│   ├── Snowball.png
│   └── SpikeBall variants (2 files)
├── sounds/ (19 files)
│   ├── BirdCaw.mp3
│   ├── BirdFlap.mp3
│   ├── BirdHurt.mp3
│   ├── Bounce.mp3
│   ├── ButtonClick.mp3
│   ├── Coin.mp3
│   ├── Death.mp3
│   ├── Fart.mp3
│   ├── Flush.mp3
│   ├── Jump.mp3
│   ├── MenuButtonClick.mp3
│   ├── PauseMenuOpen.mp3
│   ├── Plop.mp3
│   ├── PowerUp.mp3
│   ├── RatCopterHurt.mp3
│   ├── RatCopterIdle.mp3
│   ├── Splash.mp3
│   ├── Squish.mp3
│   └── Whoosh.mp3
├── turd/ (15 files)
│   ├── BigTurdHurt.png
│   ├── BigTurdIdle.png
│   ├── BigTurdJump.png
│   ├── BigTurdShoot.png
│   ├── Floppy Poop Large.png
│   ├── Floppy Poop Mid.png
│   ├── Floppy Poop.png
│   ├── TeenageTurdHurt.png
│   ├── TeenageTurdIdle.png
│   ├── TeenageTurdJump.png
│   ├── TeenageTurdShoot.png
│   ├── TurdletHurt.png
│   ├── TurdletIdle.png
│   ├── TurdletJump.png
│   └── TurdletShoot.png
├── ui/
│   └── ui_spritesheet.png
├── vfx/ (2 files)
│   ├── blast_big.png
│   └── blast_small.png
└── poophat.ico
```

## Asset File Paths for Asset Manager

### Complete File Path List
```
Assets/enemies/BirdHurt.png
Assets/enemies/BirdIdle.png
Assets/enemies/RatCopterHurt.png
Assets/enemies/RatCopterIdle.png
Assets/enemies/Ratking.png
Assets/enemies/RatkingAimBackArmOnly.png
Assets/enemies/RatkingAimTorsoOnly.png
Assets/enemies/RatkingAimTossArmOnly.png
Assets/enemies/RatkingDeath.png
Assets/enemies/RatkingHurt.png
Assets/enemies/RatkingWalk.png
Assets/enemies/SnowManChad.png
Assets/enemies/SnowManChill.png
Assets/enemies/SnowManGreen.png
Assets/enemies/SnowManIdle.png
Assets/enemies/SnowManThrow.png
Assets/enemies/SnowmanIdle.gif
Assets/enemies/ToiletPaperFlap.png
Assets/enemies/ToiletPaperHit.png
Assets/enemies/toiletpaperprojectile.png
Assets/environment/BossFloor.png
Assets/environment/BossWalls.png
Assets/environment/BottomPipeWide.png
Assets/environment/BottomPipeWideBlue.png
Assets/environment/BottomToilet.png
Assets/environment/BottomToiletGold-export.png
Assets/environment/BottomToiletGold.png
Assets/environment/BottomToiletSnow.png
Assets/environment/Cacti.png
Assets/environment/Level1BackLayerBackground.png
Assets/environment/Level1Clouds.png
Assets/environment/Level1FrontLayerBackground.png
Assets/environment/Level1MidLayerBackground.png
Assets/environment/Level3BackLayerBackground.png
Assets/environment/Level3FrontLayerBackground.png
Assets/environment/Level3MidLayerBackground.png
Assets/environment/Outhouse.png
Assets/environment/OuthouseToilet.png
Assets/environment/SewerBackgroundRunningWater.png
Assets/environment/SnowLevelBackTrees.png
Assets/environment/SnowLevelBackground.png
Assets/environment/SnowLevelFrontTrees.png
Assets/environment/SnowLevelMountains.png
Assets/environment/SnowLevelTundra.png
Assets/environment/Snowfall.png
Assets/environment/TopPipeWide.png
Assets/environment/TopPipeWideBlue.png
Assets/environment/TopToilet.png
Assets/environment/TopToiletSnow.png
Assets/fonts/Whacky_Joe.fnt
Assets/fonts/Whacky_Joe.png
Assets/music/BossMusic.mp3
Assets/music/DeathJingle.mp3
Assets/music/GameMusic.mp3
Assets/music/GameMusicLevel1.mp3
Assets/music/GameMusicLevel2.mp3
Assets/music/GameMusicLevel3.mp3
Assets/music/GameMusicLevel4.mp3
Assets/music/GameMusicLevel5.mp3
Assets/music/GameMusicLevel6.mp3
Assets/music/GameMusicLevel7.mp3
Assets/music/GameMusicLevel8.mp3
Assets/music/GameMusicLevel9.mp3
Assets/music/GameMusicLevel10.mp3
Assets/music/GameMusicLevel11.mp3
Assets/music/GameMusicLevel12.mp3
Assets/music/GameMusicLevel13.mp3
Assets/music/GameMusicLevel14.mp3
Assets/music/GameMusicLevel15.mp3
Assets/music/GameMusicLevel16.mp3
Assets/music/GameMusicLevel17.mp3
Assets/music/GameMusicLevel18.mp3
Assets/music/GameMusicLevel19.mp3
Assets/music/GameMusicLevel20.mp3
Assets/music/MainMenuMusic.mp3
Assets/music/PauseMenuMusic.mp3
Assets/music/SettingsMenuMusic.mp3
Assets/music/ShopMenuMusic.mp3
Assets/music/VictoryJingle.mp3
Assets/music/WinMusic.mp3
Assets/sounds/BirdCaw.mp3
Assets/sounds/BirdFlap.mp3
Assets/sounds/BirdHurt.mp3
Assets/sounds/Bounce.mp3
Assets/sounds/ButtonClick.mp3
Assets/sounds/Coin.mp3
Assets/sounds/Death.mp3
Assets/sounds/Fart.mp3
Assets/sounds/Flush.mp3
Assets/sounds/Jump.mp3
Assets/sounds/MenuButtonClick.mp3
Assets/sounds/PauseMenuOpen.mp3
Assets/sounds/Plop.mp3
Assets/sounds/PowerUp.mp3
Assets/sounds/RatCopterHurt.mp3
Assets/sounds/RatCopterIdle.mp3
Assets/sounds/Splash.mp3
Assets/sounds/Squish.mp3
Assets/sounds/Whoosh.mp3
Assets/ui/ui_spritesheet.png
Assets/poophat.ico
```

## Asset Categories by Type

### Graphics (Images/Sprites)
- **Enemies**: 21 files (PNG/GIF)
- **Environment**: 29 files (PNG)
- **Fonts**: 2 files (FNT/PNG)
- **UI**: 1 file (PNG)
- **Icons**: 1 file (ICO)

### Audio
- **Music**: 29 files (MP3)
- **Sound Effects**: 19 files (MP3)

### Empty Directories (Potential for Future Assets)
- `hats/`
- `mainmenu/`
- `objects/`
- `turd/`
- `vfx/`
