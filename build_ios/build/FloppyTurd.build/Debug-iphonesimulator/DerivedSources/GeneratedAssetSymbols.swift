import Foundation
#if canImport(DeveloperToolsSupport)
import DeveloperToolsSupport
#endif

#if SWIFT_PACKAGE
private let resourceBundle = Foundation.Bundle.module
#else
private class ResourceBundleClass {}
private let resourceBundle = Foundation.Bundle(for: ResourceBundleClass.self)
#endif

// MARK: - Color Symbols -

@available(iOS 17.0, macOS 14.0, tvOS 17.0, watchOS 10.0, *)
extension DeveloperToolsSupport.ColorResource {

}

// MARK: - Image Symbols -

@available(iOS 17.0, macOS 14.0, tvOS 17.0, watchOS 10.0, *)
extension DeveloperToolsSupport.ImageResource {

    /// The "Beret" asset catalog image resource.
    static let beret = DeveloperToolsSupport.ImageResource(name: "Beret", bundle: resourceBundle)

    /// The "BigTurdHurt" asset catalog image resource.
    static let bigTurdHurt = DeveloperToolsSupport.ImageResource(name: "BigTurdHurt", bundle: resourceBundle)

    /// The "BigTurdIdle" asset catalog image resource.
    static let bigTurdIdle = DeveloperToolsSupport.ImageResource(name: "BigTurdIdle", bundle: resourceBundle)

    /// The "BigTurdJump" asset catalog image resource.
    static let bigTurdJump = DeveloperToolsSupport.ImageResource(name: "BigTurdJump", bundle: resourceBundle)

    /// The "BigTurdShoot" asset catalog image resource.
    static let bigTurdShoot = DeveloperToolsSupport.ImageResource(name: "BigTurdShoot", bundle: resourceBundle)

    /// The "BirdHurt" asset catalog image resource.
    static let birdHurt = DeveloperToolsSupport.ImageResource(name: "BirdHurt", bundle: resourceBundle)

    /// The "BirdIdle" asset catalog image resource.
    static let birdIdle = DeveloperToolsSupport.ImageResource(name: "BirdIdle", bundle: resourceBundle)

    /// The "BlueCoin" asset catalog image resource.
    static let blueCoin = DeveloperToolsSupport.ImageResource(name: "BlueCoin", bundle: resourceBundle)

    /// The "BossBackground" asset catalog image resource.
    static let bossBackground = DeveloperToolsSupport.ImageResource(name: "BossBackground", bundle: resourceBundle)

    /// The "BossBarFrame" asset catalog image resource.
    static let bossBarFrame = DeveloperToolsSupport.ImageResource(name: "BossBarFrame", bundle: resourceBundle)

    /// The "BossBarHealth" asset catalog image resource.
    static let bossBarHealth = DeveloperToolsSupport.ImageResource(name: "BossBarHealth", bundle: resourceBundle)

    /// The "BossBarHurt" asset catalog image resource.
    static let bossBarHurt = DeveloperToolsSupport.ImageResource(name: "BossBarHurt", bundle: resourceBundle)

    /// The "BossFloor" asset catalog image resource.
    static let bossFloor = DeveloperToolsSupport.ImageResource(name: "BossFloor", bundle: resourceBundle)

    /// The "BossLevelBackgroundMobile" asset catalog image resource.
    static let bossLevelBackgroundMobile = DeveloperToolsSupport.ImageResource(name: "BossLevelBackgroundMobile", bundle: resourceBundle)

    /// The "BossLevelPillarMobile" asset catalog image resource.
    static let bossLevelPillarMobile = DeveloperToolsSupport.ImageResource(name: "BossLevelPillarMobile", bundle: resourceBundle)

    /// The "BossWalls" asset catalog image resource.
    static let bossWalls = DeveloperToolsSupport.ImageResource(name: "BossWalls", bundle: resourceBundle)

    /// The "BottomPipeWide" asset catalog image resource.
    static let bottomPipeWide = DeveloperToolsSupport.ImageResource(name: "BottomPipeWide", bundle: resourceBundle)

    /// The "BottomPipeWideBlue" asset catalog image resource.
    static let bottomPipeWideBlue = DeveloperToolsSupport.ImageResource(name: "BottomPipeWideBlue", bundle: resourceBundle)

    /// The "BottomToilet" asset catalog image resource.
    static let bottomToilet = DeveloperToolsSupport.ImageResource(name: "BottomToilet", bundle: resourceBundle)

    /// The "BottomToiletGold" asset catalog image resource.
    static let bottomToiletGold = DeveloperToolsSupport.ImageResource(name: "BottomToiletGold", bundle: resourceBundle)

    /// The "BottomToiletSnow" asset catalog image resource.
    static let bottomToiletSnow = DeveloperToolsSupport.ImageResource(name: "BottomToiletSnow", bundle: resourceBundle)

    /// The "BrickWall" asset catalog image resource.
    static let brickWall = DeveloperToolsSupport.ImageResource(name: "BrickWall", bundle: resourceBundle)

    /// The "CabinPainting" asset catalog image resource.
    static let cabinPainting = DeveloperToolsSupport.ImageResource(name: "CabinPainting", bundle: resourceBundle)

    /// The "Cacti" asset catalog image resource.
    static let cacti = DeveloperToolsSupport.ImageResource(name: "Cacti", bundle: resourceBundle)

    /// The "CactiA" asset catalog image resource.
    static let cactiA = DeveloperToolsSupport.ImageResource(name: "CactiA", bundle: resourceBundle)

    /// The "CactiB" asset catalog image resource.
    static let cactiB = DeveloperToolsSupport.ImageResource(name: "CactiB", bundle: resourceBundle)

    /// The "CactiBush" asset catalog image resource.
    static let cactiBush = DeveloperToolsSupport.ImageResource(name: "CactiBush", bundle: resourceBundle)

    /// The "CactiC" asset catalog image resource.
    static let cactiC = DeveloperToolsSupport.ImageResource(name: "CactiC", bundle: resourceBundle)

    /// The "CactiD" asset catalog image resource.
    static let cactiD = DeveloperToolsSupport.ImageResource(name: "CactiD", bundle: resourceBundle)

    /// The "CactiE" asset catalog image resource.
    static let cactiE = DeveloperToolsSupport.ImageResource(name: "CactiE", bundle: resourceBundle)

    /// The "CastleLevelPainting" asset catalog image resource.
    static let castleLevelPainting = DeveloperToolsSupport.ImageResource(name: "CastleLevelPainting", bundle: resourceBundle)

    /// The "CoinBag" asset catalog image resource.
    static let coinBag = DeveloperToolsSupport.ImageResource(name: "CoinBag", bundle: resourceBundle)

    /// The "DesertLevelPainting" asset catalog image resource.
    static let desertLevelPainting = DeveloperToolsSupport.ImageResource(name: "DesertLevelPainting", bundle: resourceBundle)

    /// The "DownArrow" asset catalog image resource.
    static let downArrow = DeveloperToolsSupport.ImageResource(name: "DownArrow", bundle: resourceBundle)

    /// The "EmptyPainting" asset catalog image resource.
    static let emptyPainting = DeveloperToolsSupport.ImageResource(name: "EmptyPainting", bundle: resourceBundle)

    /// The "F" asset catalog image resource.
    static let F = DeveloperToolsSupport.ImageResource(name: "F", bundle: resourceBundle)

    /// The "Floppy Poop" asset catalog image resource.
    static let floppyPoop = DeveloperToolsSupport.ImageResource(name: "Floppy Poop", bundle: resourceBundle)

    /// The "Floppy Poop Large" asset catalog image resource.
    static let floppyPoopLarge = DeveloperToolsSupport.ImageResource(name: "Floppy Poop Large", bundle: resourceBundle)

    /// The "Floppy Poop Mid" asset catalog image resource.
    static let floppyPoopMid = DeveloperToolsSupport.ImageResource(name: "Floppy Poop Mid", bundle: resourceBundle)

    /// The "FloppyButtonBlue" asset catalog image resource.
    static let floppyButtonBlue = DeveloperToolsSupport.ImageResource(name: "FloppyButtonBlue", bundle: resourceBundle)

    /// The "FloppyButtonBlueHover" asset catalog image resource.
    static let floppyButtonBlueHover = DeveloperToolsSupport.ImageResource(name: "FloppyButtonBlueHover", bundle: resourceBundle)

    /// The "FloppyLogo" asset catalog image resource.
    static let floppyLogo = DeveloperToolsSupport.ImageResource(name: "FloppyLogo", bundle: resourceBundle)

    /// The "FloppyTurdCreditsBackground" asset catalog image resource.
    static let floppyTurdCreditsBackground = DeveloperToolsSupport.ImageResource(name: "FloppyTurdCreditsBackground", bundle: resourceBundle)

    /// The "FloppyTurdMorte" asset catalog image resource.
    static let floppyTurdMorte = DeveloperToolsSupport.ImageResource(name: "FloppyTurdMorte", bundle: resourceBundle)

    /// The "GameOverBackground" asset catalog image resource.
    static let gameOverBackground = DeveloperToolsSupport.ImageResource(name: "GameOverBackground", bundle: resourceBundle)

    /// The "GameOverScore" asset catalog image resource.
    static let gameOverScore = DeveloperToolsSupport.ImageResource(name: "GameOverScore", bundle: resourceBundle)

    /// The "GoldCoin" asset catalog image resource.
    static let goldCoin = DeveloperToolsSupport.ImageResource(name: "GoldCoin", bundle: resourceBundle)

    /// The "HatFrame" asset catalog image resource.
    static let hatFrame = DeveloperToolsSupport.ImageResource(name: "HatFrame", bundle: resourceBundle)

    /// The "HatFrameDenied" asset catalog image resource.
    static let hatFrameDenied = DeveloperToolsSupport.ImageResource(name: "HatFrameDenied", bundle: resourceBundle)

    /// The "HatFrameHover" asset catalog image resource.
    static let hatFrameHover = DeveloperToolsSupport.ImageResource(name: "HatFrameHover", bundle: resourceBundle)

    /// The "HatFrameLocked" asset catalog image resource.
    static let hatFrameLocked = DeveloperToolsSupport.ImageResource(name: "HatFrameLocked", bundle: resourceBundle)

    /// The "HatFrameSelected" asset catalog image resource.
    static let hatFrameSelected = DeveloperToolsSupport.ImageResource(name: "HatFrameSelected", bundle: resourceBundle)

    /// The "Janitor" asset catalog image resource.
    static let janitor = DeveloperToolsSupport.ImageResource(name: "Janitor", bundle: resourceBundle)

    /// The "JanitorSurprise" asset catalog image resource.
    static let janitorSurprise = DeveloperToolsSupport.ImageResource(name: "JanitorSurprise", bundle: resourceBundle)

    /// The "JanitorSweep" asset catalog image resource.
    static let janitorSweep = DeveloperToolsSupport.ImageResource(name: "JanitorSweep", bundle: resourceBundle)

    /// The "LeftArrow" asset catalog image resource.
    static let leftArrow = DeveloperToolsSupport.ImageResource(name: "LeftArrow", bundle: resourceBundle)

    /// The "LeftArrowHover" asset catalog image resource.
    static let leftArrowHover = DeveloperToolsSupport.ImageResource(name: "LeftArrowHover", bundle: resourceBundle)

    /// The "Level1BackLayerBackground" asset catalog image resource.
    static let level1BackLayerBackground = DeveloperToolsSupport.ImageResource(name: "Level1BackLayerBackground", bundle: resourceBundle)

    /// The "Level1Clouds" asset catalog image resource.
    static let level1Clouds = DeveloperToolsSupport.ImageResource(name: "Level1Clouds", bundle: resourceBundle)

    /// The "Level1FrontLayerBackground" asset catalog image resource.
    static let level1FrontLayerBackground = DeveloperToolsSupport.ImageResource(name: "Level1FrontLayerBackground", bundle: resourceBundle)

    /// The "Level1MidLayerBackground" asset catalog image resource.
    static let level1MidLayerBackground = DeveloperToolsSupport.ImageResource(name: "Level1MidLayerBackground", bundle: resourceBundle)

    /// The "Level3BackLayerBackground" asset catalog image resource.
    static let level3BackLayerBackground = DeveloperToolsSupport.ImageResource(name: "Level3BackLayerBackground", bundle: resourceBundle)

    /// The "Level3FrontLayerBackground" asset catalog image resource.
    static let level3FrontLayerBackground = DeveloperToolsSupport.ImageResource(name: "Level3FrontLayerBackground", bundle: resourceBundle)

    /// The "Level3MidLayerBackground" asset catalog image resource.
    static let level3MidLayerBackground = DeveloperToolsSupport.ImageResource(name: "Level3MidLayerBackground", bundle: resourceBundle)

    /// The "LockedPainting" asset catalog image resource.
    static let lockedPainting = DeveloperToolsSupport.ImageResource(name: "LockedPainting", bundle: resourceBundle)

    /// The "MainMenu" asset catalog image resource.
    static let mainMenu = DeveloperToolsSupport.ImageResource(name: "MainMenu", bundle: resourceBundle)

    /// The "MainMenuMobile" asset catalog image resource.
    static let mainMenuMobile = DeveloperToolsSupport.ImageResource(name: "MainMenuMobile", bundle: resourceBundle)

    /// The "Outhouse" asset catalog image resource.
    static let outhouse = DeveloperToolsSupport.ImageResource(name: "Outhouse", bundle: resourceBundle)

    /// The "OuthouseToilet" asset catalog image resource.
    static let outhouseToilet = DeveloperToolsSupport.ImageResource(name: "OuthouseToilet", bundle: resourceBundle)

    /// The "ParkLevelPainting" asset catalog image resource.
    static let parkLevelPainting = DeveloperToolsSupport.ImageResource(name: "ParkLevelPainting", bundle: resourceBundle)

    /// The "PauseMenuBackground" asset catalog image resource.
    static let pauseMenuBackground = DeveloperToolsSupport.ImageResource(name: "PauseMenuBackground", bundle: resourceBundle)

    /// The "PauseMenuBackgroundMobile" asset catalog image resource.
    static let pauseMenuBackgroundMobile = DeveloperToolsSupport.ImageResource(name: "PauseMenuBackgroundMobile", bundle: resourceBundle)

    /// The "PauseMenuRibbonButton" asset catalog image resource.
    static let pauseMenuRibbonButton = DeveloperToolsSupport.ImageResource(name: "PauseMenuRibbonButton", bundle: resourceBundle)

    /// The "PinwheelHat" asset catalog image resource.
    static let pinwheelHat = DeveloperToolsSupport.ImageResource(name: "PinwheelHat", bundle: resourceBundle)

    /// The "PooHeart" asset catalog image resource.
    static let pooHeart = DeveloperToolsSupport.ImageResource(name: "PooHeart", bundle: resourceBundle)

    /// The "PooHeartBig" asset catalog image resource.
    static let pooHeartBig = DeveloperToolsSupport.ImageResource(name: "PooHeartBig", bundle: resourceBundle)

    /// The "PooHeartRainbow" asset catalog image resource.
    static let pooHeartRainbow = DeveloperToolsSupport.ImageResource(name: "PooHeartRainbow", bundle: resourceBundle)

    /// The "PooHeartRainbowBeam" asset catalog image resource.
    static let pooHeartRainbowBeam = DeveloperToolsSupport.ImageResource(name: "PooHeartRainbowBeam", bundle: resourceBundle)

    /// The "RabbitKnightPainting" asset catalog image resource.
    static let rabbitKnightPainting = DeveloperToolsSupport.ImageResource(name: "RabbitKnightPainting", bundle: resourceBundle)

    /// The "RamsesHat" asset catalog image resource.
    static let ramsesHat = DeveloperToolsSupport.ImageResource(name: "RamsesHat", bundle: resourceBundle)

    /// The "RatBeachPainting" asset catalog image resource.
    static let ratBeachPainting = DeveloperToolsSupport.ImageResource(name: "RatBeachPainting", bundle: resourceBundle)

    /// The "RatCopterHurt" asset catalog image resource.
    static let ratCopterHurt = DeveloperToolsSupport.ImageResource(name: "RatCopterHurt", bundle: resourceBundle)

    /// The "RatCopterIdle" asset catalog image resource.
    static let ratCopterIdle = DeveloperToolsSupport.ImageResource(name: "RatCopterIdle", bundle: resourceBundle)

    /// The "RatKingPainting" asset catalog image resource.
    static let ratKingPainting = DeveloperToolsSupport.ImageResource(name: "RatKingPainting", bundle: resourceBundle)

    /// The "Ratking" asset catalog image resource.
    static let ratking = DeveloperToolsSupport.ImageResource(name: "Ratking", bundle: resourceBundle)

    /// The "RatkingAimBackArmOnly" asset catalog image resource.
    static let ratkingAimBackArmOnly = DeveloperToolsSupport.ImageResource(name: "RatkingAimBackArmOnly", bundle: resourceBundle)

    /// The "RatkingAimTorsoOnly" asset catalog image resource.
    static let ratkingAimTorsoOnly = DeveloperToolsSupport.ImageResource(name: "RatkingAimTorsoOnly", bundle: resourceBundle)

    /// The "RatkingAimTossArmOnly" asset catalog image resource.
    static let ratkingAimTossArmOnly = DeveloperToolsSupport.ImageResource(name: "RatkingAimTossArmOnly", bundle: resourceBundle)

    /// The "RatkingDeath" asset catalog image resource.
    static let ratkingDeath = DeveloperToolsSupport.ImageResource(name: "RatkingDeath", bundle: resourceBundle)

    /// The "RatkingHurt" asset catalog image resource.
    static let ratkingHurt = DeveloperToolsSupport.ImageResource(name: "RatkingHurt", bundle: resourceBundle)

    /// The "RatkingStatic" asset catalog image resource.
    static let ratkingStatic = DeveloperToolsSupport.ImageResource(name: "RatkingStatic", bundle: resourceBundle)

    /// The "RatkingWalk" asset catalog image resource.
    static let ratkingWalk = DeveloperToolsSupport.ImageResource(name: "RatkingWalk", bundle: resourceBundle)

    /// The "RedCoin" asset catalog image resource.
    static let redCoin = DeveloperToolsSupport.ImageResource(name: "RedCoin", bundle: resourceBundle)

    /// The "RightArrow" asset catalog image resource.
    static let rightArrow = DeveloperToolsSupport.ImageResource(name: "RightArrow", bundle: resourceBundle)

    /// The "RightArrowHover" asset catalog image resource.
    static let rightArrowHover = DeveloperToolsSupport.ImageResource(name: "RightArrowHover", bundle: resourceBundle)

    /// The "RiverWalkPainting" asset catalog image resource.
    static let riverWalkPainting = DeveloperToolsSupport.ImageResource(name: "RiverWalkPainting", bundle: resourceBundle)

    /// The "SamuraiHelmet" asset catalog image resource.
    static let samuraiHelmet = DeveloperToolsSupport.ImageResource(name: "SamuraiHelmet", bundle: resourceBundle)

    /// The "Score" asset catalog image resource.
    static let score = DeveloperToolsSupport.ImageResource(name: "Score", bundle: resourceBundle)

    /// The "ScoreSmall" asset catalog image resource.
    static let scoreSmall = DeveloperToolsSupport.ImageResource(name: "ScoreSmall", bundle: resourceBundle)

    /// The "SewerLargeA" asset catalog image resource.
    static let sewerLargeA = DeveloperToolsSupport.ImageResource(name: "SewerLargeA", bundle: resourceBundle)

    /// The "SewerLargeB" asset catalog image resource.
    static let sewerLargeB = DeveloperToolsSupport.ImageResource(name: "SewerLargeB", bundle: resourceBundle)

    /// The "SewerLargeC" asset catalog image resource.
    static let sewerLargeC = DeveloperToolsSupport.ImageResource(name: "SewerLargeC", bundle: resourceBundle)

    /// The "SewerLargeD" asset catalog image resource.
    static let sewerLargeD = DeveloperToolsSupport.ImageResource(name: "SewerLargeD", bundle: resourceBundle)

    /// The "SewerLevelPainting" asset catalog image resource.
    static let sewerLevelPainting = DeveloperToolsSupport.ImageResource(name: "SewerLevelPainting", bundle: resourceBundle)

    /// The "SkillMenuBorder" asset catalog image resource.
    static let skillMenuBorder = DeveloperToolsSupport.ImageResource(name: "SkillMenuBorder", bundle: resourceBundle)

    /// The "SkillPointInfoBackground" asset catalog image resource.
    static let skillPointInfoBackground = DeveloperToolsSupport.ImageResource(name: "SkillPointInfoBackground", bundle: resourceBundle)

    /// The "SnowLevelBackLayerBackground" asset catalog image resource.
    static let snowLevelBackLayerBackground = DeveloperToolsSupport.ImageResource(name: "SnowLevelBackLayerBackground", bundle: resourceBundle)

    /// The "SnowLevelFrontLayerBackground" asset catalog image resource.
    static let snowLevelFrontLayerBackground = DeveloperToolsSupport.ImageResource(name: "SnowLevelFrontLayerBackground", bundle: resourceBundle)

    /// The "SnowLevelFrontLayerTrees" asset catalog image resource.
    static let snowLevelFrontLayerTrees = DeveloperToolsSupport.ImageResource(name: "SnowLevelFrontLayerTrees", bundle: resourceBundle)

    /// The "SnowLevelMidLayerBackground" asset catalog image resource.
    static let snowLevelMidLayerBackground = DeveloperToolsSupport.ImageResource(name: "SnowLevelMidLayerBackground", bundle: resourceBundle)

    /// The "SnowLevelPainting" asset catalog image resource.
    static let snowLevelPainting = DeveloperToolsSupport.ImageResource(name: "SnowLevelPainting", bundle: resourceBundle)

    /// The "SnowManChad" asset catalog image resource.
    static let snowManChad = DeveloperToolsSupport.ImageResource(name: "SnowManChad", bundle: resourceBundle)

    /// The "SnowManChill" asset catalog image resource.
    static let snowManChill = DeveloperToolsSupport.ImageResource(name: "SnowManChill", bundle: resourceBundle)

    /// The "SnowManGreen" asset catalog image resource.
    static let snowManGreen = DeveloperToolsSupport.ImageResource(name: "SnowManGreen", bundle: resourceBundle)

    /// The "SnowManIdle" asset catalog image resource.
    static let snowManIdle = DeveloperToolsSupport.ImageResource(name: "SnowManIdle", bundle: resourceBundle)

    /// The "SnowManThrow" asset catalog image resource.
    static let snowManThrow = DeveloperToolsSupport.ImageResource(name: "SnowManThrow", bundle: resourceBundle)

    /// The "Snowball" asset catalog image resource.
    static let snowball = DeveloperToolsSupport.ImageResource(name: "Snowball", bundle: resourceBundle)

    /// The "Snowfall" asset catalog image resource.
    static let snowfall = DeveloperToolsSupport.ImageResource(name: "Snowfall", bundle: resourceBundle)

    /// The "SpartanHelmet" asset catalog image resource.
    static let spartanHelmet = DeveloperToolsSupport.ImageResource(name: "SpartanHelmet", bundle: resourceBundle)

    /// The "SpikeBall" asset catalog image resource.
    static let spikeBall = DeveloperToolsSupport.ImageResource(name: "SpikeBall", bundle: resourceBundle)

    /// The "SpikeBallBase" asset catalog image resource.
    static let spikeBallBase = DeveloperToolsSupport.ImageResource(name: "SpikeBallBase", bundle: resourceBundle)

    /// The "TeenageTurdHurt" asset catalog image resource.
    static let teenageTurdHurt = DeveloperToolsSupport.ImageResource(name: "TeenageTurdHurt", bundle: resourceBundle)

    /// The "TeenageTurdIdle" asset catalog image resource.
    static let teenageTurdIdle = DeveloperToolsSupport.ImageResource(name: "TeenageTurdIdle", bundle: resourceBundle)

    /// The "TeenageTurdJump" asset catalog image resource.
    static let teenageTurdJump = DeveloperToolsSupport.ImageResource(name: "TeenageTurdJump", bundle: resourceBundle)

    /// The "TeenageTurdShoot" asset catalog image resource.
    static let teenageTurdShoot = DeveloperToolsSupport.ImageResource(name: "TeenageTurdShoot", bundle: resourceBundle)

    /// The "ToiletPaperFlap" asset catalog image resource.
    static let toiletPaperFlap = DeveloperToolsSupport.ImageResource(name: "ToiletPaperFlap", bundle: resourceBundle)

    /// The "ToiletPaperHit" asset catalog image resource.
    static let toiletPaperHit = DeveloperToolsSupport.ImageResource(name: "ToiletPaperHit", bundle: resourceBundle)

    /// The "TopPipeWide" asset catalog image resource.
    static let topPipeWide = DeveloperToolsSupport.ImageResource(name: "TopPipeWide", bundle: resourceBundle)

    /// The "TopPipeWideBlue" asset catalog image resource.
    static let topPipeWideBlue = DeveloperToolsSupport.ImageResource(name: "TopPipeWideBlue", bundle: resourceBundle)

    /// The "TopToilet" asset catalog image resource.
    static let topToilet = DeveloperToolsSupport.ImageResource(name: "TopToilet", bundle: resourceBundle)

    /// The "TopToiletGold" asset catalog image resource.
    static let topToiletGold = DeveloperToolsSupport.ImageResource(name: "TopToiletGold", bundle: resourceBundle)

    /// The "TopToiletSnow" asset catalog image resource.
    static let topToiletSnow = DeveloperToolsSupport.ImageResource(name: "TopToiletSnow", bundle: resourceBundle)

    /// The "TorchPillar" asset catalog image resource.
    static let torchPillar = DeveloperToolsSupport.ImageResource(name: "TorchPillar", bundle: resourceBundle)

    /// The "TryAgainBackground" asset catalog image resource.
    static let tryAgainBackground = DeveloperToolsSupport.ImageResource(name: "TryAgainBackground", bundle: resourceBundle)

    /// The "TurdHeart" asset catalog image resource.
    static let turdHeart = DeveloperToolsSupport.ImageResource(name: "TurdHeart", bundle: resourceBundle)

    /// The "TurdHeart0HalfHollow" asset catalog image resource.
    static let turdHeart0HalfHollow = DeveloperToolsSupport.ImageResource(name: "TurdHeart0HalfHollow", bundle: resourceBundle)

    /// The "TurdHeart0HalfHollow1Half" asset catalog image resource.
    static let turdHeart0HalfHollow1Half = DeveloperToolsSupport.ImageResource(name: "TurdHeart0HalfHollow1Half", bundle: resourceBundle)

    /// The "TurdHeart0ThirdHollow" asset catalog image resource.
    static let turdHeart0ThirdHollow = DeveloperToolsSupport.ImageResource(name: "TurdHeart0ThirdHollow", bundle: resourceBundle)

    /// The "TurdHeart0ThirdHollow1Third" asset catalog image resource.
    static let turdHeart0ThirdHollow1Third = DeveloperToolsSupport.ImageResource(name: "TurdHeart0ThirdHollow1Third", bundle: resourceBundle)

    /// The "TurdHeart1Half" asset catalog image resource.
    static let turdHeart1Half = DeveloperToolsSupport.ImageResource(name: "TurdHeart1Half", bundle: resourceBundle)

    /// The "TurdHeart1HalfHollow" asset catalog image resource.
    static let turdHeart1HalfHollow = DeveloperToolsSupport.ImageResource(name: "TurdHeart1HalfHollow", bundle: resourceBundle)

    /// The "TurdHeart1Third" asset catalog image resource.
    static let turdHeart1Third = DeveloperToolsSupport.ImageResource(name: "TurdHeart1Third", bundle: resourceBundle)

    /// The "TurdHeart1ThirdHollow" asset catalog image resource.
    static let turdHeart1ThirdHollow = DeveloperToolsSupport.ImageResource(name: "TurdHeart1ThirdHollow", bundle: resourceBundle)

    /// The "TurdHeart1ThirdsHollow1Thirds" asset catalog image resource.
    static let turdHeart1ThirdsHollow1Thirds = DeveloperToolsSupport.ImageResource(name: "TurdHeart1ThirdsHollow1Thirds", bundle: resourceBundle)

    /// The "TurdHeart2Thirds" asset catalog image resource.
    static let turdHeart2Thirds = DeveloperToolsSupport.ImageResource(name: "TurdHeart2Thirds", bundle: resourceBundle)

    /// The "TurdHeart2ThirdsHollow" asset catalog image resource.
    static let turdHeart2ThirdsHollow = DeveloperToolsSupport.ImageResource(name: "TurdHeart2ThirdsHollow", bundle: resourceBundle)

    /// The "TurdHeart2ThirdsHollow2Thirds" asset catalog image resource.
    static let turdHeart2ThirdsHollow2Thirds = DeveloperToolsSupport.ImageResource(name: "TurdHeart2ThirdsHollow2Thirds", bundle: resourceBundle)

    /// The "TurdHeartHollow" asset catalog image resource.
    static let turdHeartHollow = DeveloperToolsSupport.ImageResource(name: "TurdHeartHollow", bundle: resourceBundle)

    /// The "TurdHeartSmall" asset catalog image resource.
    static let turdHeartSmall = DeveloperToolsSupport.ImageResource(name: "TurdHeartSmall", bundle: resourceBundle)

    /// The "TurdPointButton" asset catalog image resource.
    static let turdPointButton = DeveloperToolsSupport.ImageResource(name: "TurdPointButton", bundle: resourceBundle)

    /// The "TurdPointButtonAvailable" asset catalog image resource.
    static let turdPointButtonAvailable = DeveloperToolsSupport.ImageResource(name: "TurdPointButtonAvailable", bundle: resourceBundle)

    /// The "TurdPointButtonClaimed" asset catalog image resource.
    static let turdPointButtonClaimed = DeveloperToolsSupport.ImageResource(name: "TurdPointButtonClaimed", bundle: resourceBundle)

    /// The "TurdPointButtonFocused" asset catalog image resource.
    static let turdPointButtonFocused = DeveloperToolsSupport.ImageResource(name: "TurdPointButtonFocused", bundle: resourceBundle)

    /// The "TurdPointMenu" asset catalog image resource.
    static let turdPointMenu = DeveloperToolsSupport.ImageResource(name: "TurdPointMenu", bundle: resourceBundle)

    /// The "TurdletHurt" asset catalog image resource.
    static let turdletHurt = DeveloperToolsSupport.ImageResource(name: "TurdletHurt", bundle: resourceBundle)

    /// The "TurdletIdle" asset catalog image resource.
    static let turdletIdle = DeveloperToolsSupport.ImageResource(name: "TurdletIdle", bundle: resourceBundle)

    /// The "TurdletJump" asset catalog image resource.
    static let turdletJump = DeveloperToolsSupport.ImageResource(name: "TurdletJump", bundle: resourceBundle)

    /// The "TurdletShoot" asset catalog image resource.
    static let turdletShoot = DeveloperToolsSupport.ImageResource(name: "TurdletShoot", bundle: resourceBundle)

    /// The "UpArrow" asset catalog image resource.
    static let upArrow = DeveloperToolsSupport.ImageResource(name: "UpArrow", bundle: resourceBundle)

    /// The "Whacky_Joe_msdf_atlas" asset catalog image resource.
    static let whackyJoeMsdfAtlas = DeveloperToolsSupport.ImageResource(name: "Whacky_Joe_msdf_atlas", bundle: resourceBundle)

    /// The "a" asset catalog image resource.
    static let a = DeveloperToolsSupport.ImageResource(name: "a", bundle: resourceBundle)

    /// The "achievementicon" asset catalog image resource.
    static let achievementicon = DeveloperToolsSupport.ImageResource(name: "achievementicon", bundle: resourceBundle)

    /// The "achievementiconunlocked" asset catalog image resource.
    static let achievementiconunlocked = DeveloperToolsSupport.ImageResource(name: "achievementiconunlocked", bundle: resourceBundle)

    /// The "arrowsturning" asset catalog image resource.
    static let arrowsturning = DeveloperToolsSupport.ImageResource(name: "arrowsturning", bundle: resourceBundle)

    /// The "ballcap" asset catalog image resource.
    static let ballcap = DeveloperToolsSupport.ImageResource(name: "ballcap", bundle: resourceBundle)

    /// The "ballcapbigturdjump" asset catalog image resource.
    static let ballcapbigturdjump = DeveloperToolsSupport.ImageResource(name: "ballcapbigturdjump", bundle: resourceBundle)

    /// The "ballcapbigturdshoot" asset catalog image resource.
    static let ballcapbigturdshoot = DeveloperToolsSupport.ImageResource(name: "ballcapbigturdshoot", bundle: resourceBundle)

    /// The "ballcapturdletjump" asset catalog image resource.
    static let ballcapturdletjump = DeveloperToolsSupport.ImageResource(name: "ballcapturdletjump", bundle: resourceBundle)

    /// The "ballcapturdletshoot" asset catalog image resource.
    static let ballcapturdletshoot = DeveloperToolsSupport.ImageResource(name: "ballcapturdletshoot", bundle: resourceBundle)

    /// The "berethatbigturdjump" asset catalog image resource.
    static let berethatbigturdjump = DeveloperToolsSupport.ImageResource(name: "berethatbigturdjump", bundle: resourceBundle)

    /// The "berethatbigturdshoot" asset catalog image resource.
    static let berethatbigturdshoot = DeveloperToolsSupport.ImageResource(name: "berethatbigturdshoot", bundle: resourceBundle)

    /// The "berethatturdletjump" asset catalog image resource.
    static let berethatturdletjump = DeveloperToolsSupport.ImageResource(name: "berethatturdletjump", bundle: resourceBundle)

    /// The "berethatturdletshoot" asset catalog image resource.
    static let berethatturdletshoot = DeveloperToolsSupport.ImageResource(name: "berethatturdletshoot", bundle: resourceBundle)

    /// The "blast_big" asset catalog image resource.
    static let blastBig = DeveloperToolsSupport.ImageResource(name: "blast_big", bundle: resourceBundle)

    /// The "blast_small" asset catalog image resource.
    static let blastSmall = DeveloperToolsSupport.ImageResource(name: "blast_small", bundle: resourceBundle)

    /// The "bosspillar" asset catalog image resource.
    static let bosspillar = DeveloperToolsSupport.ImageResource(name: "bosspillar", bundle: resourceBundle)

    /// The "castlebacklayerbackground" asset catalog image resource.
    static let castlebacklayerbackground = DeveloperToolsSupport.ImageResource(name: "castlebacklayerbackground", bundle: resourceBundle)

    /// The "castlelevelchandelier" asset catalog image resource.
    static let castlelevelchandelier = DeveloperToolsSupport.ImageResource(name: "castlelevelchandelier", bundle: resourceBundle)

    /// The "castlelevelfloortorch" asset catalog image resource.
    static let castlelevelfloortorch = DeveloperToolsSupport.ImageResource(name: "castlelevelfloortorch", bundle: resourceBundle)

    /// The "cowboyhat" asset catalog image resource.
    static let cowboyhat = DeveloperToolsSupport.ImageResource(name: "cowboyhat", bundle: resourceBundle)

    /// The "cowboyhatbigturd" asset catalog image resource.
    static let cowboyhatbigturd = DeveloperToolsSupport.ImageResource(name: "cowboyhatbigturd", bundle: resourceBundle)

    /// The "cowboyhatbigturdjump" asset catalog image resource.
    static let cowboyhatbigturdjump = DeveloperToolsSupport.ImageResource(name: "cowboyhatbigturdjump", bundle: resourceBundle)

    /// The "cowboyhatbigturdshoot" asset catalog image resource.
    static let cowboyhatbigturdshoot = DeveloperToolsSupport.ImageResource(name: "cowboyhatbigturdshoot", bundle: resourceBundle)

    /// The "cowboyhatteenage" asset catalog image resource.
    static let cowboyhatteenage = DeveloperToolsSupport.ImageResource(name: "cowboyhatteenage", bundle: resourceBundle)

    /// The "cowboyhatteenageshoot" asset catalog image resource.
    static let cowboyhatteenageshoot = DeveloperToolsSupport.ImageResource(name: "cowboyhatteenageshoot", bundle: resourceBundle)

    /// The "cowboyhatturdlet" asset catalog image resource.
    static let cowboyhatturdlet = DeveloperToolsSupport.ImageResource(name: "cowboyhatturdlet", bundle: resourceBundle)

    /// The "cowboyhatturdletjump" asset catalog image resource.
    static let cowboyhatturdletjump = DeveloperToolsSupport.ImageResource(name: "cowboyhatturdletjump", bundle: resourceBundle)

    /// The "cowboyhatturdletshoot" asset catalog image resource.
    static let cowboyhatturdletshoot = DeveloperToolsSupport.ImageResource(name: "cowboyhatturdletshoot", bundle: resourceBundle)

    /// The "crown" asset catalog image resource.
    static let crown = DeveloperToolsSupport.ImageResource(name: "crown", bundle: resourceBundle)

    /// The "crownhatbigturdjump" asset catalog image resource.
    static let crownhatbigturdjump = DeveloperToolsSupport.ImageResource(name: "crownhatbigturdjump", bundle: resourceBundle)

    /// The "crownhatbigturdshoot" asset catalog image resource.
    static let crownhatbigturdshoot = DeveloperToolsSupport.ImageResource(name: "crownhatbigturdshoot", bundle: resourceBundle)

    /// The "crownhatturdletjump" asset catalog image resource.
    static let crownhatturdletjump = DeveloperToolsSupport.ImageResource(name: "crownhatturdletjump", bundle: resourceBundle)

    /// The "crownhatturdletshoot" asset catalog image resource.
    static let crownhatturdletshoot = DeveloperToolsSupport.ImageResource(name: "crownhatturdletshoot", bundle: resourceBundle)

    /// The "curtains" asset catalog image resource.
    static let curtains = DeveloperToolsSupport.ImageResource(name: "curtains", bundle: resourceBundle)

    /// The "dancingcacti" asset catalog image resource.
    static let dancingcacti = DeveloperToolsSupport.ImageResource(name: "dancingcacti", bundle: resourceBundle)

    /// The "dancingcacticowboy" asset catalog image resource.
    static let dancingcacticowboy = DeveloperToolsSupport.ImageResource(name: "dancingcacticowboy", bundle: resourceBundle)

    /// The "dancingcactismall" asset catalog image resource.
    static let dancingcactismall = DeveloperToolsSupport.ImageResource(name: "dancingcactismall", bundle: resourceBundle)

    /// The "dooragbigturdjump" asset catalog image resource.
    static let dooragbigturdjump = DeveloperToolsSupport.ImageResource(name: "dooragbigturdjump", bundle: resourceBundle)

    /// The "dooragbigturdshoot" asset catalog image resource.
    static let dooragbigturdshoot = DeveloperToolsSupport.ImageResource(name: "dooragbigturdshoot", bundle: resourceBundle)

    /// The "dooraghat" asset catalog image resource.
    static let dooraghat = DeveloperToolsSupport.ImageResource(name: "dooraghat", bundle: resourceBundle)

    /// The "dooragturdletjump" asset catalog image resource.
    static let dooragturdletjump = DeveloperToolsSupport.ImageResource(name: "dooragturdletjump", bundle: resourceBundle)

    /// The "dooragturdletshoot" asset catalog image resource.
    static let dooragturdletshoot = DeveloperToolsSupport.ImageResource(name: "dooragturdletshoot", bundle: resourceBundle)

    /// The "flowerhat" asset catalog image resource.
    static let flowerhat = DeveloperToolsSupport.ImageResource(name: "flowerhat", bundle: resourceBundle)

    /// The "flowerhatbigturdjump" asset catalog image resource.
    static let flowerhatbigturdjump = DeveloperToolsSupport.ImageResource(name: "flowerhatbigturdjump", bundle: resourceBundle)

    /// The "flowerhatbigturdshoot" asset catalog image resource.
    static let flowerhatbigturdshoot = DeveloperToolsSupport.ImageResource(name: "flowerhatbigturdshoot", bundle: resourceBundle)

    /// The "flowerhatturdletjump" asset catalog image resource.
    static let flowerhatturdletjump = DeveloperToolsSupport.ImageResource(name: "flowerhatturdletjump", bundle: resourceBundle)

    /// The "flowerhatturdletshoot" asset catalog image resource.
    static let flowerhatturdletshoot = DeveloperToolsSupport.ImageResource(name: "flowerhatturdletshoot", bundle: resourceBundle)

    /// The "minusbutton" asset catalog image resource.
    static let minusbutton = DeveloperToolsSupport.ImageResource(name: "minusbutton", bundle: resourceBundle)

    /// The "minusbuttonclicked" asset catalog image resource.
    static let minusbuttonclicked = DeveloperToolsSupport.ImageResource(name: "minusbuttonclicked", bundle: resourceBundle)

    /// The "minusbuttonhover" asset catalog image resource.
    static let minusbuttonhover = DeveloperToolsSupport.ImageResource(name: "minusbuttonhover", bundle: resourceBundle)

    /// The "mutebutton" asset catalog image resource.
    static let mutebutton = DeveloperToolsSupport.ImageResource(name: "mutebutton", bundle: resourceBundle)

    /// The "mutebuttonclicked" asset catalog image resource.
    static let mutebuttonclicked = DeveloperToolsSupport.ImageResource(name: "mutebuttonclicked", bundle: resourceBundle)

    /// The "mutebuttonhover" asset catalog image resource.
    static let mutebuttonhover = DeveloperToolsSupport.ImageResource(name: "mutebuttonhover", bundle: resourceBundle)

    /// The "mutebuttonlocked" asset catalog image resource.
    static let mutebuttonlocked = DeveloperToolsSupport.ImageResource(name: "mutebuttonlocked", bundle: resourceBundle)

    /// The "mutebuttonlockedclicked" asset catalog image resource.
    static let mutebuttonlockedclicked = DeveloperToolsSupport.ImageResource(name: "mutebuttonlockedclicked", bundle: resourceBundle)

    /// The "mutebuttonlockedhover" asset catalog image resource.
    static let mutebuttonlockedhover = DeveloperToolsSupport.ImageResource(name: "mutebuttonlockedhover", bundle: resourceBundle)

    /// The "pinwheelbigturdjump" asset catalog image resource.
    static let pinwheelbigturdjump = DeveloperToolsSupport.ImageResource(name: "pinwheelbigturdjump", bundle: resourceBundle)

    /// The "pinwheelbigturdshoot" asset catalog image resource.
    static let pinwheelbigturdshoot = DeveloperToolsSupport.ImageResource(name: "pinwheelbigturdshoot", bundle: resourceBundle)

    /// The "pinwheelturdletjump" asset catalog image resource.
    static let pinwheelturdletjump = DeveloperToolsSupport.ImageResource(name: "pinwheelturdletjump", bundle: resourceBundle)

    /// The "pinwheelturdletshoot" asset catalog image resource.
    static let pinwheelturdletshoot = DeveloperToolsSupport.ImageResource(name: "pinwheelturdletshoot", bundle: resourceBundle)

    /// The "plusbutton" asset catalog image resource.
    static let plusbutton = DeveloperToolsSupport.ImageResource(name: "plusbutton", bundle: resourceBundle)

    /// The "plusbuttonclicked" asset catalog image resource.
    static let plusbuttonclicked = DeveloperToolsSupport.ImageResource(name: "plusbuttonclicked", bundle: resourceBundle)

    /// The "plusbuttonhover" asset catalog image resource.
    static let plusbuttonhover = DeveloperToolsSupport.ImageResource(name: "plusbuttonhover", bundle: resourceBundle)

    /// The "poophat" asset catalog image resource.
    static let poophat = DeveloperToolsSupport.ImageResource(name: "poophat", bundle: resourceBundle)

    /// The "poophatbigturdjump" asset catalog image resource.
    static let poophatbigturdjump = DeveloperToolsSupport.ImageResource(name: "poophatbigturdjump", bundle: resourceBundle)

    /// The "poophatbigturdshoot" asset catalog image resource.
    static let poophatbigturdshoot = DeveloperToolsSupport.ImageResource(name: "poophatbigturdshoot", bundle: resourceBundle)

    /// The "poophatturdletjump" asset catalog image resource.
    static let poophatturdletjump = DeveloperToolsSupport.ImageResource(name: "poophatturdletjump", bundle: resourceBundle)

    /// The "poophatturdletshoot" asset catalog image resource.
    static let poophatturdletshoot = DeveloperToolsSupport.ImageResource(name: "poophatturdletshoot", bundle: resourceBundle)

    /// The "ramsesbigturdjump" asset catalog image resource.
    static let ramsesbigturdjump = DeveloperToolsSupport.ImageResource(name: "ramsesbigturdjump", bundle: resourceBundle)

    /// The "ramsesbigturdshoot" asset catalog image resource.
    static let ramsesbigturdshoot = DeveloperToolsSupport.ImageResource(name: "ramsesbigturdshoot", bundle: resourceBundle)

    /// The "ramsesturdletjump" asset catalog image resource.
    static let ramsesturdletjump = DeveloperToolsSupport.ImageResource(name: "ramsesturdletjump", bundle: resourceBundle)

    /// The "ramsesturdletshoot" asset catalog image resource.
    static let ramsesturdletshoot = DeveloperToolsSupport.ImageResource(name: "ramsesturdletshoot", bundle: resourceBundle)

    /// The "samuraibigturdjump" asset catalog image resource.
    static let samuraibigturdjump = DeveloperToolsSupport.ImageResource(name: "samuraibigturdjump", bundle: resourceBundle)

    /// The "samuraibigturdshoot" asset catalog image resource.
    static let samuraibigturdshoot = DeveloperToolsSupport.ImageResource(name: "samuraibigturdshoot", bundle: resourceBundle)

    /// The "samuraiturdletjump" asset catalog image resource.
    static let samuraiturdletjump = DeveloperToolsSupport.ImageResource(name: "samuraiturdletjump", bundle: resourceBundle)

    /// The "samuraiturdletshoot" asset catalog image resource.
    static let samuraiturdletshoot = DeveloperToolsSupport.ImageResource(name: "samuraiturdletshoot", bundle: resourceBundle)

    /// The "screenCurtains" asset catalog image resource.
    static let screenCurtains = DeveloperToolsSupport.ImageResource(name: "screenCurtains", bundle: resourceBundle)

    /// The "settingsbutton" asset catalog image resource.
    static let settingsbutton = DeveloperToolsSupport.ImageResource(name: "settingsbutton", bundle: resourceBundle)

    /// The "settingsbuttonclicked" asset catalog image resource.
    static let settingsbuttonclicked = DeveloperToolsSupport.ImageResource(name: "settingsbuttonclicked", bundle: resourceBundle)

    /// The "shellhat" asset catalog image resource.
    static let shellhat = DeveloperToolsSupport.ImageResource(name: "shellhat", bundle: resourceBundle)

    /// The "shellhatbigturdjump" asset catalog image resource.
    static let shellhatbigturdjump = DeveloperToolsSupport.ImageResource(name: "shellhatbigturdjump", bundle: resourceBundle)

    /// The "shellhatbigturdshoot" asset catalog image resource.
    static let shellhatbigturdshoot = DeveloperToolsSupport.ImageResource(name: "shellhatbigturdshoot", bundle: resourceBundle)

    /// The "shellhatturdletjump" asset catalog image resource.
    static let shellhatturdletjump = DeveloperToolsSupport.ImageResource(name: "shellhatturdletjump", bundle: resourceBundle)

    /// The "shellhatturdletshoot" asset catalog image resource.
    static let shellhatturdletshoot = DeveloperToolsSupport.ImageResource(name: "shellhatturdletshoot", bundle: resourceBundle)

    /// The "spartanhatbigturdjump" asset catalog image resource.
    static let spartanhatbigturdjump = DeveloperToolsSupport.ImageResource(name: "spartanhatbigturdjump", bundle: resourceBundle)

    /// The "spartanhatbigturdshoot" asset catalog image resource.
    static let spartanhatbigturdshoot = DeveloperToolsSupport.ImageResource(name: "spartanhatbigturdshoot", bundle: resourceBundle)

    /// The "spartanhatturdletjump" asset catalog image resource.
    static let spartanhatturdletjump = DeveloperToolsSupport.ImageResource(name: "spartanhatturdletjump", bundle: resourceBundle)

    /// The "spartanhatturdletshoot" asset catalog image resource.
    static let spartanhatturdletshoot = DeveloperToolsSupport.ImageResource(name: "spartanhatturdletshoot", bundle: resourceBundle)

    /// The "strawhat" asset catalog image resource.
    static let strawhat = DeveloperToolsSupport.ImageResource(name: "strawhat", bundle: resourceBundle)

    /// The "strawhatbigturdjump" asset catalog image resource.
    static let strawhatbigturdjump = DeveloperToolsSupport.ImageResource(name: "strawhatbigturdjump", bundle: resourceBundle)

    /// The "strawhatbigturdshoot" asset catalog image resource.
    static let strawhatbigturdshoot = DeveloperToolsSupport.ImageResource(name: "strawhatbigturdshoot", bundle: resourceBundle)

    /// The "strawhatturdletjump" asset catalog image resource.
    static let strawhatturdletjump = DeveloperToolsSupport.ImageResource(name: "strawhatturdletjump", bundle: resourceBundle)

    /// The "strawhatturdletshoot" asset catalog image resource.
    static let strawhatturdletshoot = DeveloperToolsSupport.ImageResource(name: "strawhatturdletshoot", bundle: resourceBundle)

    /// The "toiletpaperprojectile" asset catalog image resource.
    static let toiletpaperprojectile = DeveloperToolsSupport.ImageResource(name: "toiletpaperprojectile", bundle: resourceBundle)

    /// The "tophat" asset catalog image resource.
    static let tophat = DeveloperToolsSupport.ImageResource(name: "tophat", bundle: resourceBundle)

    /// The "tophatbigturdjump" asset catalog image resource.
    static let tophatbigturdjump = DeveloperToolsSupport.ImageResource(name: "tophatbigturdjump", bundle: resourceBundle)

    /// The "tophatbigturdshoot" asset catalog image resource.
    static let tophatbigturdshoot = DeveloperToolsSupport.ImageResource(name: "tophatbigturdshoot", bundle: resourceBundle)

    /// The "tophatturdletjump" asset catalog image resource.
    static let tophatturdletjump = DeveloperToolsSupport.ImageResource(name: "tophatturdletjump", bundle: resourceBundle)

    /// The "tophatturdletshoot" asset catalog image resource.
    static let tophatturdletshoot = DeveloperToolsSupport.ImageResource(name: "tophatturdletshoot", bundle: resourceBundle)

    /// The "ushanka" asset catalog image resource.
    static let ushanka = DeveloperToolsSupport.ImageResource(name: "ushanka", bundle: resourceBundle)

    /// The "ushankabigturdjump" asset catalog image resource.
    static let ushankabigturdjump = DeveloperToolsSupport.ImageResource(name: "ushankabigturdjump", bundle: resourceBundle)

    /// The "ushankabigturdshoot" asset catalog image resource.
    static let ushankabigturdshoot = DeveloperToolsSupport.ImageResource(name: "ushankabigturdshoot", bundle: resourceBundle)

    /// The "ushankaturdletjump" asset catalog image resource.
    static let ushankaturdletjump = DeveloperToolsSupport.ImageResource(name: "ushankaturdletjump", bundle: resourceBundle)

    /// The "ushankaturdletshoot" asset catalog image resource.
    static let ushankaturdletshoot = DeveloperToolsSupport.ImageResource(name: "ushankaturdletshoot", bundle: resourceBundle)

    /// The "volumemeterempty" asset catalog image resource.
    static let volumemeterempty = DeveloperToolsSupport.ImageResource(name: "volumemeterempty", bundle: resourceBundle)

    /// The "volumemeterfull" asset catalog image resource.
    static let volumemeterfull = DeveloperToolsSupport.ImageResource(name: "volumemeterfull", bundle: resourceBundle)

}

