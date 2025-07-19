import Foundation
#if canImport(AppKit)
import AppKit
#endif
#if canImport(UIKit)
import UIKit
#endif
#if canImport(SwiftUI)
import SwiftUI
#endif
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

@available(iOS 11.0, macOS 10.13, tvOS 11.0, *)
extension ColorResource {

}

// MARK: - Image Symbols -

@available(iOS 11.0, macOS 10.7, tvOS 11.0, *)
extension ImageResource {

    /// The "Beret" asset catalog image resource.
    static let beret = ImageResource(name: "Beret", bundle: resourceBundle)

    /// The "BigTurdHurt" asset catalog image resource.
    static let bigTurdHurt = ImageResource(name: "BigTurdHurt", bundle: resourceBundle)

    /// The "BigTurdIdle" asset catalog image resource.
    static let bigTurdIdle = ImageResource(name: "BigTurdIdle", bundle: resourceBundle)

    /// The "BigTurdJump" asset catalog image resource.
    static let bigTurdJump = ImageResource(name: "BigTurdJump", bundle: resourceBundle)

    /// The "BigTurdShoot" asset catalog image resource.
    static let bigTurdShoot = ImageResource(name: "BigTurdShoot", bundle: resourceBundle)

    /// The "BirdHurt" asset catalog image resource.
    static let birdHurt = ImageResource(name: "BirdHurt", bundle: resourceBundle)

    /// The "BirdIdle" asset catalog image resource.
    static let birdIdle = ImageResource(name: "BirdIdle", bundle: resourceBundle)

    /// The "BlueCoin" asset catalog image resource.
    static let blueCoin = ImageResource(name: "BlueCoin", bundle: resourceBundle)

    /// The "BossBarFrame" asset catalog image resource.
    static let bossBarFrame = ImageResource(name: "BossBarFrame", bundle: resourceBundle)

    /// The "BossBarHealth" asset catalog image resource.
    static let bossBarHealth = ImageResource(name: "BossBarHealth", bundle: resourceBundle)

    /// The "BossBarHurt" asset catalog image resource.
    static let bossBarHurt = ImageResource(name: "BossBarHurt", bundle: resourceBundle)

    /// The "BossFloor" asset catalog image resource.
    static let bossFloor = ImageResource(name: "BossFloor", bundle: resourceBundle)

    /// The "BossWalls" asset catalog image resource.
    static let bossWalls = ImageResource(name: "BossWalls", bundle: resourceBundle)

    /// The "BottomPipeWide" asset catalog image resource.
    static let bottomPipeWide = ImageResource(name: "BottomPipeWide", bundle: resourceBundle)

    /// The "BottomPipeWideBlue" asset catalog image resource.
    static let bottomPipeWideBlue = ImageResource(name: "BottomPipeWideBlue", bundle: resourceBundle)

    /// The "BottomToilet" asset catalog image resource.
    static let bottomToilet = ImageResource(name: "BottomToilet", bundle: resourceBundle)

    /// The "BottomToiletGold" asset catalog image resource.
    static let bottomToiletGold = ImageResource(name: "BottomToiletGold", bundle: resourceBundle)

    /// The "BottomToiletGold-export" asset catalog image resource.
    static let bottomToiletGoldExport = ImageResource(name: "BottomToiletGold-export", bundle: resourceBundle)

    /// The "BottomToiletSnow" asset catalog image resource.
    static let bottomToiletSnow = ImageResource(name: "BottomToiletSnow", bundle: resourceBundle)

    /// The "BrickWall" asset catalog image resource.
    static let brickWall = ImageResource(name: "BrickWall", bundle: resourceBundle)

    /// The "CabinPainting" asset catalog image resource.
    static let cabinPainting = ImageResource(name: "CabinPainting", bundle: resourceBundle)

    /// The "Cacti" asset catalog image resource.
    static let cacti = ImageResource(name: "Cacti", bundle: resourceBundle)

    /// The "CactiA" asset catalog image resource.
    static let cactiA = ImageResource(name: "CactiA", bundle: resourceBundle)

    /// The "CactiB" asset catalog image resource.
    static let cactiB = ImageResource(name: "CactiB", bundle: resourceBundle)

    /// The "CactiBush" asset catalog image resource.
    static let cactiBush = ImageResource(name: "CactiBush", bundle: resourceBundle)

    /// The "CactiC" asset catalog image resource.
    static let cactiC = ImageResource(name: "CactiC", bundle: resourceBundle)

    /// The "CactiD" asset catalog image resource.
    static let cactiD = ImageResource(name: "CactiD", bundle: resourceBundle)

    /// The "CactiE" asset catalog image resource.
    static let cactiE = ImageResource(name: "CactiE", bundle: resourceBundle)

    /// The "CastleLevelPainting" asset catalog image resource.
    static let castleLevelPainting = ImageResource(name: "CastleLevelPainting", bundle: resourceBundle)

    /// The "CoinBag" asset catalog image resource.
    static let coinBag = ImageResource(name: "CoinBag", bundle: resourceBundle)

    /// The "DesertLevelPainting" asset catalog image resource.
    static let desertLevelPainting = ImageResource(name: "DesertLevelPainting", bundle: resourceBundle)

    /// The "DownArrow" asset catalog image resource.
    static let downArrow = ImageResource(name: "DownArrow", bundle: resourceBundle)

    /// The "EmptyPainting" asset catalog image resource.
    static let emptyPainting = ImageResource(name: "EmptyPainting", bundle: resourceBundle)

    /// The "F" asset catalog image resource.
    static let F = ImageResource(name: "F", bundle: resourceBundle)

    /// The "Floppy Poop" asset catalog image resource.
    static let floppyPoop = ImageResource(name: "Floppy Poop", bundle: resourceBundle)

    /// The "Floppy Poop Large" asset catalog image resource.
    static let floppyPoopLarge = ImageResource(name: "Floppy Poop Large", bundle: resourceBundle)

    /// The "Floppy Poop Mid" asset catalog image resource.
    static let floppyPoopMid = ImageResource(name: "Floppy Poop Mid", bundle: resourceBundle)

    /// The "FloppyButtonBlue" asset catalog image resource.
    static let floppyButtonBlue = ImageResource(name: "FloppyButtonBlue", bundle: resourceBundle)

    /// The "FloppyButtonBlueHover" asset catalog image resource.
    static let floppyButtonBlueHover = ImageResource(name: "FloppyButtonBlueHover", bundle: resourceBundle)

    /// The "FloppyLogo" asset catalog image resource.
    static let floppyLogo = ImageResource(name: "FloppyLogo", bundle: resourceBundle)

    /// The "FloppyTurdCreditsBackground" asset catalog image resource.
    static let floppyTurdCreditsBackground = ImageResource(name: "FloppyTurdCreditsBackground", bundle: resourceBundle)

    /// The "FloppyTurdMorte" asset catalog image resource.
    static let floppyTurdMorte = ImageResource(name: "FloppyTurdMorte", bundle: resourceBundle)

    /// The "GameOverBackground" asset catalog image resource.
    static let gameOverBackground = ImageResource(name: "GameOverBackground", bundle: resourceBundle)

    /// The "GameOverScore" asset catalog image resource.
    static let gameOverScore = ImageResource(name: "GameOverScore", bundle: resourceBundle)

    /// The "GoldCoin" asset catalog image resource.
    static let goldCoin = ImageResource(name: "GoldCoin", bundle: resourceBundle)

    /// The "HatFrame" asset catalog image resource.
    static let hatFrame = ImageResource(name: "HatFrame", bundle: resourceBundle)

    /// The "HatFrameDenied" asset catalog image resource.
    static let hatFrameDenied = ImageResource(name: "HatFrameDenied", bundle: resourceBundle)

    /// The "HatFrameHover" asset catalog image resource.
    static let hatFrameHover = ImageResource(name: "HatFrameHover", bundle: resourceBundle)

    /// The "HatFrameLocked" asset catalog image resource.
    static let hatFrameLocked = ImageResource(name: "HatFrameLocked", bundle: resourceBundle)

    /// The "HatFrameSelected" asset catalog image resource.
    static let hatFrameSelected = ImageResource(name: "HatFrameSelected", bundle: resourceBundle)

    /// The "Janitor" asset catalog image resource.
    static let janitor = ImageResource(name: "Janitor", bundle: resourceBundle)

    /// The "JanitorSurprise" asset catalog image resource.
    static let janitorSurprise = ImageResource(name: "JanitorSurprise", bundle: resourceBundle)

    /// The "JanitorSweep" asset catalog image resource.
    static let janitorSweep = ImageResource(name: "JanitorSweep", bundle: resourceBundle)

    /// The "LeftArrow" asset catalog image resource.
    static let leftArrow = ImageResource(name: "LeftArrow", bundle: resourceBundle)

    /// The "LeftArrowHover" asset catalog image resource.
    static let leftArrowHover = ImageResource(name: "LeftArrowHover", bundle: resourceBundle)

    /// The "Level1BackLayerBackground" asset catalog image resource.
    static let level1BackLayerBackground = ImageResource(name: "Level1BackLayerBackground", bundle: resourceBundle)

    /// The "Level1Clouds" asset catalog image resource.
    static let level1Clouds = ImageResource(name: "Level1Clouds", bundle: resourceBundle)

    /// The "Level1FrontLayerBackground" asset catalog image resource.
    static let level1FrontLayerBackground = ImageResource(name: "Level1FrontLayerBackground", bundle: resourceBundle)

    /// The "Level1MidLayerBackground" asset catalog image resource.
    static let level1MidLayerBackground = ImageResource(name: "Level1MidLayerBackground", bundle: resourceBundle)

    /// The "Level3BackLayerBackground" asset catalog image resource.
    static let level3BackLayerBackground = ImageResource(name: "Level3BackLayerBackground", bundle: resourceBundle)

    /// The "Level3FrontLayerBackground" asset catalog image resource.
    static let level3FrontLayerBackground = ImageResource(name: "Level3FrontLayerBackground", bundle: resourceBundle)

    /// The "Level3MidLayerBackground" asset catalog image resource.
    static let level3MidLayerBackground = ImageResource(name: "Level3MidLayerBackground", bundle: resourceBundle)

    /// The "LockedPainting" asset catalog image resource.
    static let lockedPainting = ImageResource(name: "LockedPainting", bundle: resourceBundle)

    /// The "MainMenu" asset catalog image resource.
    static let mainMenu = ImageResource(name: "MainMenu", bundle: resourceBundle)

    /// The "MainMenuMobile" asset catalog image resource.
    static let mainMenuMobile = ImageResource(name: "MainMenuMobile", bundle: resourceBundle)

    /// The "Outhouse" asset catalog image resource.
    static let outhouse = ImageResource(name: "Outhouse", bundle: resourceBundle)

    /// The "OuthouseToilet" asset catalog image resource.
    static let outhouseToilet = ImageResource(name: "OuthouseToilet", bundle: resourceBundle)

    /// The "ParkLevelPainting" asset catalog image resource.
    static let parkLevelPainting = ImageResource(name: "ParkLevelPainting", bundle: resourceBundle)

    /// The "PauseMenuBackground" asset catalog image resource.
    static let pauseMenuBackground = ImageResource(name: "PauseMenuBackground", bundle: resourceBundle)

    /// The "PinwheelHat" asset catalog image resource.
    static let pinwheelHat = ImageResource(name: "PinwheelHat", bundle: resourceBundle)

    /// The "PooHeart" asset catalog image resource.
    static let pooHeart = ImageResource(name: "PooHeart", bundle: resourceBundle)

    /// The "PooHeartBig" asset catalog image resource.
    static let pooHeartBig = ImageResource(name: "PooHeartBig", bundle: resourceBundle)

    /// The "PooHeartRainbow" asset catalog image resource.
    static let pooHeartRainbow = ImageResource(name: "PooHeartRainbow", bundle: resourceBundle)

    /// The "PooHeartRainbowBeam" asset catalog image resource.
    static let pooHeartRainbowBeam = ImageResource(name: "PooHeartRainbowBeam", bundle: resourceBundle)

    /// The "RabbitKnightPainting" asset catalog image resource.
    static let rabbitKnightPainting = ImageResource(name: "RabbitKnightPainting", bundle: resourceBundle)

    /// The "RamsesHat" asset catalog image resource.
    static let ramsesHat = ImageResource(name: "RamsesHat", bundle: resourceBundle)

    /// The "RatBeachPainting" asset catalog image resource.
    static let ratBeachPainting = ImageResource(name: "RatBeachPainting", bundle: resourceBundle)

    /// The "RatCopterHurt" asset catalog image resource.
    static let ratCopterHurt = ImageResource(name: "RatCopterHurt", bundle: resourceBundle)

    /// The "RatCopterIdle" asset catalog image resource.
    static let ratCopterIdle = ImageResource(name: "RatCopterIdle", bundle: resourceBundle)

    /// The "RatKingPainting" asset catalog image resource.
    static let ratKingPainting = ImageResource(name: "RatKingPainting", bundle: resourceBundle)

    /// The "Ratking" asset catalog image resource.
    static let ratking = ImageResource(name: "Ratking", bundle: resourceBundle)

    /// The "RatkingAimBackArmOnly" asset catalog image resource.
    static let ratkingAimBackArmOnly = ImageResource(name: "RatkingAimBackArmOnly", bundle: resourceBundle)

    /// The "RatkingAimTorsoOnly" asset catalog image resource.
    static let ratkingAimTorsoOnly = ImageResource(name: "RatkingAimTorsoOnly", bundle: resourceBundle)

    /// The "RatkingAimTossArmOnly" asset catalog image resource.
    static let ratkingAimTossArmOnly = ImageResource(name: "RatkingAimTossArmOnly", bundle: resourceBundle)

    /// The "RatkingDeath" asset catalog image resource.
    static let ratkingDeath = ImageResource(name: "RatkingDeath", bundle: resourceBundle)

    /// The "RatkingHurt" asset catalog image resource.
    static let ratkingHurt = ImageResource(name: "RatkingHurt", bundle: resourceBundle)

    /// The "RatkingStatic" asset catalog image resource.
    static let ratkingStatic = ImageResource(name: "RatkingStatic", bundle: resourceBundle)

    /// The "RatkingWalk" asset catalog image resource.
    static let ratkingWalk = ImageResource(name: "RatkingWalk", bundle: resourceBundle)

    /// The "RedCoin" asset catalog image resource.
    static let redCoin = ImageResource(name: "RedCoin", bundle: resourceBundle)

    /// The "RightArrow" asset catalog image resource.
    static let rightArrow = ImageResource(name: "RightArrow", bundle: resourceBundle)

    /// The "RightArrowHover" asset catalog image resource.
    static let rightArrowHover = ImageResource(name: "RightArrowHover", bundle: resourceBundle)

    /// The "RiverWalkPainting" asset catalog image resource.
    static let riverWalkPainting = ImageResource(name: "RiverWalkPainting", bundle: resourceBundle)

    /// The "SamuraiHelmet" asset catalog image resource.
    static let samuraiHelmet = ImageResource(name: "SamuraiHelmet", bundle: resourceBundle)

    /// The "Score" asset catalog image resource.
    static let score = ImageResource(name: "Score", bundle: resourceBundle)

    /// The "ScoreSmall" asset catalog image resource.
    static let scoreSmall = ImageResource(name: "ScoreSmall", bundle: resourceBundle)

    /// The "SewerBackgroundRunningWater" asset catalog image resource.
    static let sewerBackgroundRunningWater = ImageResource(name: "SewerBackgroundRunningWater", bundle: resourceBundle)

    /// The "SewerLevelPainting" asset catalog image resource.
    static let sewerLevelPainting = ImageResource(name: "SewerLevelPainting", bundle: resourceBundle)

    /// The "SkillMenuBorder" asset catalog image resource.
    static let skillMenuBorder = ImageResource(name: "SkillMenuBorder", bundle: resourceBundle)

    /// The "SkillPointInfoBackground" asset catalog image resource.
    static let skillPointInfoBackground = ImageResource(name: "SkillPointInfoBackground", bundle: resourceBundle)

    /// The "SnowLevelBackTrees" asset catalog image resource.
    static let snowLevelBackTrees = ImageResource(name: "SnowLevelBackTrees", bundle: resourceBundle)

    /// The "SnowLevelBackground" asset catalog image resource.
    static let snowLevelBackground = ImageResource(name: "SnowLevelBackground", bundle: resourceBundle)

    /// The "SnowLevelFrontTrees" asset catalog image resource.
    static let snowLevelFrontTrees = ImageResource(name: "SnowLevelFrontTrees", bundle: resourceBundle)

    /// The "SnowLevelMountains" asset catalog image resource.
    static let snowLevelMountains = ImageResource(name: "SnowLevelMountains", bundle: resourceBundle)

    /// The "SnowLevelPainting" asset catalog image resource.
    static let snowLevelPainting = ImageResource(name: "SnowLevelPainting", bundle: resourceBundle)

    /// The "SnowLevelTundra" asset catalog image resource.
    static let snowLevelTundra = ImageResource(name: "SnowLevelTundra", bundle: resourceBundle)

    /// The "SnowManChad" asset catalog image resource.
    static let snowManChad = ImageResource(name: "SnowManChad", bundle: resourceBundle)

    /// The "SnowManChill" asset catalog image resource.
    static let snowManChill = ImageResource(name: "SnowManChill", bundle: resourceBundle)

    /// The "SnowManGreen" asset catalog image resource.
    static let snowManGreen = ImageResource(name: "SnowManGreen", bundle: resourceBundle)

    /// The "SnowManIdle" asset catalog image resource.
    static let snowManIdle = ImageResource(name: "SnowManIdle", bundle: resourceBundle)

    /// The "SnowManThrow" asset catalog image resource.
    static let snowManThrow = ImageResource(name: "SnowManThrow", bundle: resourceBundle)

    /// The "Snowball" asset catalog image resource.
    static let snowball = ImageResource(name: "Snowball", bundle: resourceBundle)

    /// The "Snowfall" asset catalog image resource.
    static let snowfall = ImageResource(name: "Snowfall", bundle: resourceBundle)

    /// The "SpartanHelmet" asset catalog image resource.
    static let spartanHelmet = ImageResource(name: "SpartanHelmet", bundle: resourceBundle)

    /// The "SpikeBall" asset catalog image resource.
    static let spikeBall = ImageResource(name: "SpikeBall", bundle: resourceBundle)

    /// The "SpikeBallBase" asset catalog image resource.
    static let spikeBallBase = ImageResource(name: "SpikeBallBase", bundle: resourceBundle)

    /// The "TeenageTurdHurt" asset catalog image resource.
    static let teenageTurdHurt = ImageResource(name: "TeenageTurdHurt", bundle: resourceBundle)

    /// The "TeenageTurdIdle" asset catalog image resource.
    static let teenageTurdIdle = ImageResource(name: "TeenageTurdIdle", bundle: resourceBundle)

    /// The "TeenageTurdJump" asset catalog image resource.
    static let teenageTurdJump = ImageResource(name: "TeenageTurdJump", bundle: resourceBundle)

    /// The "TeenageTurdShoot" asset catalog image resource.
    static let teenageTurdShoot = ImageResource(name: "TeenageTurdShoot", bundle: resourceBundle)

    /// The "ToiletPaperFlap" asset catalog image resource.
    static let toiletPaperFlap = ImageResource(name: "ToiletPaperFlap", bundle: resourceBundle)

    /// The "ToiletPaperHit" asset catalog image resource.
    static let toiletPaperHit = ImageResource(name: "ToiletPaperHit", bundle: resourceBundle)

    /// The "TopPipeWide" asset catalog image resource.
    static let topPipeWide = ImageResource(name: "TopPipeWide", bundle: resourceBundle)

    /// The "TopPipeWideBlue" asset catalog image resource.
    static let topPipeWideBlue = ImageResource(name: "TopPipeWideBlue", bundle: resourceBundle)

    /// The "TopToilet" asset catalog image resource.
    static let topToilet = ImageResource(name: "TopToilet", bundle: resourceBundle)

    /// The "TopToiletGold" asset catalog image resource.
    static let topToiletGold = ImageResource(name: "TopToiletGold", bundle: resourceBundle)

    /// The "TopToiletSnow" asset catalog image resource.
    static let topToiletSnow = ImageResource(name: "TopToiletSnow", bundle: resourceBundle)

    /// The "TorchPillar" asset catalog image resource.
    static let torchPillar = ImageResource(name: "TorchPillar", bundle: resourceBundle)

    /// The "TryAgainBackground" asset catalog image resource.
    static let tryAgainBackground = ImageResource(name: "TryAgainBackground", bundle: resourceBundle)

    /// The "TurdHeart" asset catalog image resource.
    static let turdHeart = ImageResource(name: "TurdHeart", bundle: resourceBundle)

    /// The "TurdHeart0HalfHollow" asset catalog image resource.
    static let turdHeart0HalfHollow = ImageResource(name: "TurdHeart0HalfHollow", bundle: resourceBundle)

    /// The "TurdHeart0HalfHollow1Half" asset catalog image resource.
    static let turdHeart0HalfHollow1Half = ImageResource(name: "TurdHeart0HalfHollow1Half", bundle: resourceBundle)

    /// The "TurdHeart0ThirdHollow" asset catalog image resource.
    static let turdHeart0ThirdHollow = ImageResource(name: "TurdHeart0ThirdHollow", bundle: resourceBundle)

    /// The "TurdHeart0ThirdHollow1Third" asset catalog image resource.
    static let turdHeart0ThirdHollow1Third = ImageResource(name: "TurdHeart0ThirdHollow1Third", bundle: resourceBundle)

    /// The "TurdHeart1Half" asset catalog image resource.
    static let turdHeart1Half = ImageResource(name: "TurdHeart1Half", bundle: resourceBundle)

    /// The "TurdHeart1HalfHollow" asset catalog image resource.
    static let turdHeart1HalfHollow = ImageResource(name: "TurdHeart1HalfHollow", bundle: resourceBundle)

    /// The "TurdHeart1Third" asset catalog image resource.
    static let turdHeart1Third = ImageResource(name: "TurdHeart1Third", bundle: resourceBundle)

    /// The "TurdHeart1ThirdHollow" asset catalog image resource.
    static let turdHeart1ThirdHollow = ImageResource(name: "TurdHeart1ThirdHollow", bundle: resourceBundle)

    /// The "TurdHeart1ThirdsHollow1Thirds" asset catalog image resource.
    static let turdHeart1ThirdsHollow1Thirds = ImageResource(name: "TurdHeart1ThirdsHollow1Thirds", bundle: resourceBundle)

    /// The "TurdHeart2Thirds" asset catalog image resource.
    static let turdHeart2Thirds = ImageResource(name: "TurdHeart2Thirds", bundle: resourceBundle)

    /// The "TurdHeart2ThirdsHollow" asset catalog image resource.
    static let turdHeart2ThirdsHollow = ImageResource(name: "TurdHeart2ThirdsHollow", bundle: resourceBundle)

    /// The "TurdHeart2ThirdsHollow2Thirds" asset catalog image resource.
    static let turdHeart2ThirdsHollow2Thirds = ImageResource(name: "TurdHeart2ThirdsHollow2Thirds", bundle: resourceBundle)

    /// The "TurdHeartHollow" asset catalog image resource.
    static let turdHeartHollow = ImageResource(name: "TurdHeartHollow", bundle: resourceBundle)

    /// The "TurdHeartSmall" asset catalog image resource.
    static let turdHeartSmall = ImageResource(name: "TurdHeartSmall", bundle: resourceBundle)

    /// The "TurdPointButton" asset catalog image resource.
    static let turdPointButton = ImageResource(name: "TurdPointButton", bundle: resourceBundle)

    /// The "TurdPointButtonAvailable" asset catalog image resource.
    static let turdPointButtonAvailable = ImageResource(name: "TurdPointButtonAvailable", bundle: resourceBundle)

    /// The "TurdPointButtonClaimed" asset catalog image resource.
    static let turdPointButtonClaimed = ImageResource(name: "TurdPointButtonClaimed", bundle: resourceBundle)

    /// The "TurdPointButtonFocused" asset catalog image resource.
    static let turdPointButtonFocused = ImageResource(name: "TurdPointButtonFocused", bundle: resourceBundle)

    /// The "TurdPointMenu" asset catalog image resource.
    static let turdPointMenu = ImageResource(name: "TurdPointMenu", bundle: resourceBundle)

    /// The "TurdletHurt" asset catalog image resource.
    static let turdletHurt = ImageResource(name: "TurdletHurt", bundle: resourceBundle)

    /// The "TurdletIdle" asset catalog image resource.
    static let turdletIdle = ImageResource(name: "TurdletIdle", bundle: resourceBundle)

    /// The "TurdletJump" asset catalog image resource.
    static let turdletJump = ImageResource(name: "TurdletJump", bundle: resourceBundle)

    /// The "TurdletShoot" asset catalog image resource.
    static let turdletShoot = ImageResource(name: "TurdletShoot", bundle: resourceBundle)

    /// The "UpArrow" asset catalog image resource.
    static let upArrow = ImageResource(name: "UpArrow", bundle: resourceBundle)

    /// The "a" asset catalog image resource.
    static let a = ImageResource(name: "a", bundle: resourceBundle)

    /// The "achievementicon" asset catalog image resource.
    static let achievementicon = ImageResource(name: "achievementicon", bundle: resourceBundle)

    /// The "achievementiconunlocked" asset catalog image resource.
    static let achievementiconunlocked = ImageResource(name: "achievementiconunlocked", bundle: resourceBundle)

    /// The "ballcap" asset catalog image resource.
    static let ballcap = ImageResource(name: "ballcap", bundle: resourceBundle)

    /// The "ballcapbigturdjump" asset catalog image resource.
    static let ballcapbigturdjump = ImageResource(name: "ballcapbigturdjump", bundle: resourceBundle)

    /// The "ballcapbigturdshoot" asset catalog image resource.
    static let ballcapbigturdshoot = ImageResource(name: "ballcapbigturdshoot", bundle: resourceBundle)

    /// The "ballcapturdletjump" asset catalog image resource.
    static let ballcapturdletjump = ImageResource(name: "ballcapturdletjump", bundle: resourceBundle)

    /// The "ballcapturdletshoot" asset catalog image resource.
    static let ballcapturdletshoot = ImageResource(name: "ballcapturdletshoot", bundle: resourceBundle)

    /// The "berethatbigturdjump" asset catalog image resource.
    static let berethatbigturdjump = ImageResource(name: "berethatbigturdjump", bundle: resourceBundle)

    /// The "berethatbigturdshoot" asset catalog image resource.
    static let berethatbigturdshoot = ImageResource(name: "berethatbigturdshoot", bundle: resourceBundle)

    /// The "berethatturdletjump" asset catalog image resource.
    static let berethatturdletjump = ImageResource(name: "berethatturdletjump", bundle: resourceBundle)

    /// The "berethatturdletshoot" asset catalog image resource.
    static let berethatturdletshoot = ImageResource(name: "berethatturdletshoot", bundle: resourceBundle)

    /// The "blast_big" asset catalog image resource.
    static let blastBig = ImageResource(name: "blast_big", bundle: resourceBundle)

    /// The "blast_small" asset catalog image resource.
    static let blastSmall = ImageResource(name: "blast_small", bundle: resourceBundle)

    /// The "bosspillar" asset catalog image resource.
    static let bosspillar = ImageResource(name: "bosspillar", bundle: resourceBundle)

    /// The "castlelevelbackgroundwall" asset catalog image resource.
    static let castlelevelbackgroundwall = ImageResource(name: "castlelevelbackgroundwall", bundle: resourceBundle)

    /// The "castlelevelchandelier" asset catalog image resource.
    static let castlelevelchandelier = ImageResource(name: "castlelevelchandelier", bundle: resourceBundle)

    /// The "castlelevelfloorceiling" asset catalog image resource.
    static let castlelevelfloorceiling = ImageResource(name: "castlelevelfloorceiling", bundle: resourceBundle)

    /// The "castlelevelfloortorch" asset catalog image resource.
    static let castlelevelfloortorch = ImageResource(name: "castlelevelfloortorch", bundle: resourceBundle)

    /// The "cowboyhat" asset catalog image resource.
    static let cowboyhat = ImageResource(name: "cowboyhat", bundle: resourceBundle)

    /// The "cowboyhatbigturd" asset catalog image resource.
    static let cowboyhatbigturd = ImageResource(name: "cowboyhatbigturd", bundle: resourceBundle)

    /// The "cowboyhatbigturdjump" asset catalog image resource.
    static let cowboyhatbigturdjump = ImageResource(name: "cowboyhatbigturdjump", bundle: resourceBundle)

    /// The "cowboyhatbigturdshoot" asset catalog image resource.
    static let cowboyhatbigturdshoot = ImageResource(name: "cowboyhatbigturdshoot", bundle: resourceBundle)

    /// The "cowboyhatteenage" asset catalog image resource.
    static let cowboyhatteenage = ImageResource(name: "cowboyhatteenage", bundle: resourceBundle)

    /// The "cowboyhatteenageshoot" asset catalog image resource.
    static let cowboyhatteenageshoot = ImageResource(name: "cowboyhatteenageshoot", bundle: resourceBundle)

    /// The "cowboyhatturdlet" asset catalog image resource.
    static let cowboyhatturdlet = ImageResource(name: "cowboyhatturdlet", bundle: resourceBundle)

    /// The "cowboyhatturdletjump" asset catalog image resource.
    static let cowboyhatturdletjump = ImageResource(name: "cowboyhatturdletjump", bundle: resourceBundle)

    /// The "cowboyhatturdletshoot" asset catalog image resource.
    static let cowboyhatturdletshoot = ImageResource(name: "cowboyhatturdletshoot", bundle: resourceBundle)

    /// The "crown" asset catalog image resource.
    static let crown = ImageResource(name: "crown", bundle: resourceBundle)

    /// The "crownhatbigturdjump" asset catalog image resource.
    static let crownhatbigturdjump = ImageResource(name: "crownhatbigturdjump", bundle: resourceBundle)

    /// The "crownhatbigturdshoot" asset catalog image resource.
    static let crownhatbigturdshoot = ImageResource(name: "crownhatbigturdshoot", bundle: resourceBundle)

    /// The "crownhatturdletjump" asset catalog image resource.
    static let crownhatturdletjump = ImageResource(name: "crownhatturdletjump", bundle: resourceBundle)

    /// The "crownhatturdletshoot" asset catalog image resource.
    static let crownhatturdletshoot = ImageResource(name: "crownhatturdletshoot", bundle: resourceBundle)

    /// The "curtains" asset catalog image resource.
    static let curtains = ImageResource(name: "curtains", bundle: resourceBundle)

    /// The "dancingcacti" asset catalog image resource.
    static let dancingcacti = ImageResource(name: "dancingcacti", bundle: resourceBundle)

    /// The "dancingcacticowboy" asset catalog image resource.
    static let dancingcacticowboy = ImageResource(name: "dancingcacticowboy", bundle: resourceBundle)

    /// The "dancingcactismall" asset catalog image resource.
    static let dancingcactismall = ImageResource(name: "dancingcactismall", bundle: resourceBundle)

    /// The "darkclouds" asset catalog image resource.
    static let darkclouds = ImageResource(name: "darkclouds", bundle: resourceBundle)

    /// The "dooragbigturdjump" asset catalog image resource.
    static let dooragbigturdjump = ImageResource(name: "dooragbigturdjump", bundle: resourceBundle)

    /// The "dooragbigturdshoot" asset catalog image resource.
    static let dooragbigturdshoot = ImageResource(name: "dooragbigturdshoot", bundle: resourceBundle)

    /// The "dooraghat" asset catalog image resource.
    static let dooraghat = ImageResource(name: "dooraghat", bundle: resourceBundle)

    /// The "dooragturdletjump" asset catalog image resource.
    static let dooragturdletjump = ImageResource(name: "dooragturdletjump", bundle: resourceBundle)

    /// The "dooragturdletshoot" asset catalog image resource.
    static let dooragturdletshoot = ImageResource(name: "dooragturdletshoot", bundle: resourceBundle)

    /// The "flowerhat" asset catalog image resource.
    static let flowerhat = ImageResource(name: "flowerhat", bundle: resourceBundle)

    /// The "flowerhatbigturdjump" asset catalog image resource.
    static let flowerhatbigturdjump = ImageResource(name: "flowerhatbigturdjump", bundle: resourceBundle)

    /// The "flowerhatbigturdshoot" asset catalog image resource.
    static let flowerhatbigturdshoot = ImageResource(name: "flowerhatbigturdshoot", bundle: resourceBundle)

    /// The "flowerhatturdletjump" asset catalog image resource.
    static let flowerhatturdletjump = ImageResource(name: "flowerhatturdletjump", bundle: resourceBundle)

    /// The "flowerhatturdletshoot" asset catalog image resource.
    static let flowerhatturdletshoot = ImageResource(name: "flowerhatturdletshoot", bundle: resourceBundle)

    /// The "minusbutton" asset catalog image resource.
    static let minusbutton = ImageResource(name: "minusbutton", bundle: resourceBundle)

    /// The "minusbuttonclicked" asset catalog image resource.
    static let minusbuttonclicked = ImageResource(name: "minusbuttonclicked", bundle: resourceBundle)

    /// The "minusbuttonhover" asset catalog image resource.
    static let minusbuttonhover = ImageResource(name: "minusbuttonhover", bundle: resourceBundle)

    /// The "mutebutton" asset catalog image resource.
    static let mutebutton = ImageResource(name: "mutebutton", bundle: resourceBundle)

    /// The "mutebuttonclicked" asset catalog image resource.
    static let mutebuttonclicked = ImageResource(name: "mutebuttonclicked", bundle: resourceBundle)

    /// The "mutebuttonhover" asset catalog image resource.
    static let mutebuttonhover = ImageResource(name: "mutebuttonhover", bundle: resourceBundle)

    /// The "mutebuttonlocked" asset catalog image resource.
    static let mutebuttonlocked = ImageResource(name: "mutebuttonlocked", bundle: resourceBundle)

    /// The "mutebuttonlockedclicked" asset catalog image resource.
    static let mutebuttonlockedclicked = ImageResource(name: "mutebuttonlockedclicked", bundle: resourceBundle)

    /// The "mutebuttonlockedhover" asset catalog image resource.
    static let mutebuttonlockedhover = ImageResource(name: "mutebuttonlockedhover", bundle: resourceBundle)

    /// The "pinwheelbigturdjump" asset catalog image resource.
    static let pinwheelbigturdjump = ImageResource(name: "pinwheelbigturdjump", bundle: resourceBundle)

    /// The "pinwheelbigturdshoot" asset catalog image resource.
    static let pinwheelbigturdshoot = ImageResource(name: "pinwheelbigturdshoot", bundle: resourceBundle)

    /// The "pinwheelturdletjump" asset catalog image resource.
    static let pinwheelturdletjump = ImageResource(name: "pinwheelturdletjump", bundle: resourceBundle)

    /// The "pinwheelturdletshoot" asset catalog image resource.
    static let pinwheelturdletshoot = ImageResource(name: "pinwheelturdletshoot", bundle: resourceBundle)

    /// The "placeholder" asset catalog image resource.
    static let placeholder = ImageResource(name: "placeholder", bundle: resourceBundle)

    /// The "plusbutton" asset catalog image resource.
    static let plusbutton = ImageResource(name: "plusbutton", bundle: resourceBundle)

    /// The "plusbuttonclicked" asset catalog image resource.
    static let plusbuttonclicked = ImageResource(name: "plusbuttonclicked", bundle: resourceBundle)

    /// The "plusbuttonhover" asset catalog image resource.
    static let plusbuttonhover = ImageResource(name: "plusbuttonhover", bundle: resourceBundle)

    /// The "poophat" asset catalog image resource.
    static let poophat = ImageResource(name: "poophat", bundle: resourceBundle)

    /// The "poophatbigturdjump" asset catalog image resource.
    static let poophatbigturdjump = ImageResource(name: "poophatbigturdjump", bundle: resourceBundle)

    /// The "poophatbigturdshoot" asset catalog image resource.
    static let poophatbigturdshoot = ImageResource(name: "poophatbigturdshoot", bundle: resourceBundle)

    /// The "poophatturdletjump" asset catalog image resource.
    static let poophatturdletjump = ImageResource(name: "poophatturdletjump", bundle: resourceBundle)

    /// The "poophatturdletshoot" asset catalog image resource.
    static let poophatturdletshoot = ImageResource(name: "poophatturdletshoot", bundle: resourceBundle)

    /// The "ramsesbigturdjump" asset catalog image resource.
    static let ramsesbigturdjump = ImageResource(name: "ramsesbigturdjump", bundle: resourceBundle)

    /// The "ramsesbigturdshoot" asset catalog image resource.
    static let ramsesbigturdshoot = ImageResource(name: "ramsesbigturdshoot", bundle: resourceBundle)

    /// The "ramsesturdletjump" asset catalog image resource.
    static let ramsesturdletjump = ImageResource(name: "ramsesturdletjump", bundle: resourceBundle)

    /// The "ramsesturdletshoot" asset catalog image resource.
    static let ramsesturdletshoot = ImageResource(name: "ramsesturdletshoot", bundle: resourceBundle)

    /// The "ratkingbackground" asset catalog image resource.
    static let ratkingbackground = ImageResource(name: "ratkingbackground", bundle: resourceBundle)

    /// The "samuraibigturdjump" asset catalog image resource.
    static let samuraibigturdjump = ImageResource(name: "samuraibigturdjump", bundle: resourceBundle)

    /// The "samuraibigturdshoot" asset catalog image resource.
    static let samuraibigturdshoot = ImageResource(name: "samuraibigturdshoot", bundle: resourceBundle)

    /// The "samuraiturdletjump" asset catalog image resource.
    static let samuraiturdletjump = ImageResource(name: "samuraiturdletjump", bundle: resourceBundle)

    /// The "samuraiturdletshoot" asset catalog image resource.
    static let samuraiturdletshoot = ImageResource(name: "samuraiturdletshoot", bundle: resourceBundle)

    /// The "screenCurtains" asset catalog image resource.
    static let screenCurtains = ImageResource(name: "screenCurtains", bundle: resourceBundle)

    /// The "sewerrunningwaterwide" asset catalog image resource.
    static let sewerrunningwaterwide = ImageResource(name: "sewerrunningwaterwide", bundle: resourceBundle)

    /// The "sewerwidevarA" asset catalog image resource.
    static let sewerwidevarA = ImageResource(name: "sewerwidevarA", bundle: resourceBundle)

    /// The "sewerwidevarB" asset catalog image resource.
    static let sewerwidevarB = ImageResource(name: "sewerwidevarB", bundle: resourceBundle)

    /// The "sewerwidevarC" asset catalog image resource.
    static let sewerwidevarC = ImageResource(name: "sewerwidevarC", bundle: resourceBundle)

    /// The "sewerwidevarD" asset catalog image resource.
    static let sewerwidevarD = ImageResource(name: "sewerwidevarD", bundle: resourceBundle)

    /// The "sewerwidevarE" asset catalog image resource.
    static let sewerwidevarE = ImageResource(name: "sewerwidevarE", bundle: resourceBundle)

    /// The "shellhat" asset catalog image resource.
    static let shellhat = ImageResource(name: "shellhat", bundle: resourceBundle)

    /// The "shellhatbigturdjump" asset catalog image resource.
    static let shellhatbigturdjump = ImageResource(name: "shellhatbigturdjump", bundle: resourceBundle)

    /// The "shellhatbigturdshoot" asset catalog image resource.
    static let shellhatbigturdshoot = ImageResource(name: "shellhatbigturdshoot", bundle: resourceBundle)

    /// The "shellhatturdletjump" asset catalog image resource.
    static let shellhatturdletjump = ImageResource(name: "shellhatturdletjump", bundle: resourceBundle)

    /// The "shellhatturdletshoot" asset catalog image resource.
    static let shellhatturdletshoot = ImageResource(name: "shellhatturdletshoot", bundle: resourceBundle)

    /// The "snow_tile" asset catalog image resource.
    static let snowTile = ImageResource(name: "snow_tile", bundle: resourceBundle)

    /// The "spartanhatbigturdjump" asset catalog image resource.
    static let spartanhatbigturdjump = ImageResource(name: "spartanhatbigturdjump", bundle: resourceBundle)

    /// The "spartanhatbigturdshoot" asset catalog image resource.
    static let spartanhatbigturdshoot = ImageResource(name: "spartanhatbigturdshoot", bundle: resourceBundle)

    /// The "spartanhatturdletjump" asset catalog image resource.
    static let spartanhatturdletjump = ImageResource(name: "spartanhatturdletjump", bundle: resourceBundle)

    /// The "spartanhatturdletshoot" asset catalog image resource.
    static let spartanhatturdletshoot = ImageResource(name: "spartanhatturdletshoot", bundle: resourceBundle)

    /// The "strawhat" asset catalog image resource.
    static let strawhat = ImageResource(name: "strawhat", bundle: resourceBundle)

    /// The "strawhatbigturdjump" asset catalog image resource.
    static let strawhatbigturdjump = ImageResource(name: "strawhatbigturdjump", bundle: resourceBundle)

    /// The "strawhatbigturdshoot" asset catalog image resource.
    static let strawhatbigturdshoot = ImageResource(name: "strawhatbigturdshoot", bundle: resourceBundle)

    /// The "strawhatturdletjump" asset catalog image resource.
    static let strawhatturdletjump = ImageResource(name: "strawhatturdletjump", bundle: resourceBundle)

    /// The "strawhatturdletshoot" asset catalog image resource.
    static let strawhatturdletshoot = ImageResource(name: "strawhatturdletshoot", bundle: resourceBundle)

    /// The "toiletpaperprojectile" asset catalog image resource.
    static let toiletpaperprojectile = ImageResource(name: "toiletpaperprojectile", bundle: resourceBundle)

    /// The "tophat" asset catalog image resource.
    static let tophat = ImageResource(name: "tophat", bundle: resourceBundle)

    /// The "tophatbigturdjump" asset catalog image resource.
    static let tophatbigturdjump = ImageResource(name: "tophatbigturdjump", bundle: resourceBundle)

    /// The "tophatbigturdshoot" asset catalog image resource.
    static let tophatbigturdshoot = ImageResource(name: "tophatbigturdshoot", bundle: resourceBundle)

    /// The "tophatturdletjump" asset catalog image resource.
    static let tophatturdletjump = ImageResource(name: "tophatturdletjump", bundle: resourceBundle)

    /// The "tophatturdletshoot" asset catalog image resource.
    static let tophatturdletshoot = ImageResource(name: "tophatturdletshoot", bundle: resourceBundle)

    /// The "ushanka" asset catalog image resource.
    static let ushanka = ImageResource(name: "ushanka", bundle: resourceBundle)

    /// The "ushankabigturdjump" asset catalog image resource.
    static let ushankabigturdjump = ImageResource(name: "ushankabigturdjump", bundle: resourceBundle)

    /// The "ushankabigturdshoot" asset catalog image resource.
    static let ushankabigturdshoot = ImageResource(name: "ushankabigturdshoot", bundle: resourceBundle)

    /// The "ushankaturdletjump" asset catalog image resource.
    static let ushankaturdletjump = ImageResource(name: "ushankaturdletjump", bundle: resourceBundle)

    /// The "ushankaturdletshoot" asset catalog image resource.
    static let ushankaturdletshoot = ImageResource(name: "ushankaturdletshoot", bundle: resourceBundle)

    /// The "volumemeterempty" asset catalog image resource.
    static let volumemeterempty = ImageResource(name: "volumemeterempty", bundle: resourceBundle)

    /// The "volumemeterfull" asset catalog image resource.
    static let volumemeterfull = ImageResource(name: "volumemeterfull", bundle: resourceBundle)

}

// MARK: - Backwards Deployment Support -

/// A color resource.
struct ColorResource: Swift.Hashable, Swift.Sendable {

    /// An asset catalog color resource name.
    fileprivate let name: Swift.String

    /// An asset catalog color resource bundle.
    fileprivate let bundle: Foundation.Bundle

    /// Initialize a `ColorResource` with `name` and `bundle`.
    init(name: Swift.String, bundle: Foundation.Bundle) {
        self.name = name
        self.bundle = bundle
    }

}

/// An image resource.
struct ImageResource: Swift.Hashable, Swift.Sendable {

    /// An asset catalog image resource name.
    fileprivate let name: Swift.String

    /// An asset catalog image resource bundle.
    fileprivate let bundle: Foundation.Bundle

    /// Initialize an `ImageResource` with `name` and `bundle`.
    init(name: Swift.String, bundle: Foundation.Bundle) {
        self.name = name
        self.bundle = bundle
    }

}

#if canImport(AppKit)
@available(macOS 10.13, *)
@available(macCatalyst, unavailable)
extension AppKit.NSColor {

    /// Initialize a `NSColor` with a color resource.
    convenience init(resource: ColorResource) {
        self.init(named: NSColor.Name(resource.name), bundle: resource.bundle)!
    }

}

protocol _ACResourceInitProtocol {}
extension AppKit.NSImage: _ACResourceInitProtocol {}

@available(macOS 10.7, *)
@available(macCatalyst, unavailable)
extension _ACResourceInitProtocol {

    /// Initialize a `NSImage` with an image resource.
    init(resource: ImageResource) {
        self = resource.bundle.image(forResource: NSImage.Name(resource.name))! as! Self
    }

}
#endif

#if canImport(UIKit)
@available(iOS 11.0, tvOS 11.0, *)
@available(watchOS, unavailable)
extension UIKit.UIColor {

    /// Initialize a `UIColor` with a color resource.
    convenience init(resource: ColorResource) {
#if !os(watchOS)
        self.init(named: resource.name, in: resource.bundle, compatibleWith: nil)!
#else
        self.init()
#endif
    }

}

@available(iOS 11.0, tvOS 11.0, *)
@available(watchOS, unavailable)
extension UIKit.UIImage {

    /// Initialize a `UIImage` with an image resource.
    convenience init(resource: ImageResource) {
#if !os(watchOS)
        self.init(named: resource.name, in: resource.bundle, compatibleWith: nil)!
#else
        self.init()
#endif
    }

}
#endif

#if canImport(SwiftUI)
@available(iOS 13.0, macOS 10.15, tvOS 13.0, watchOS 6.0, *)
extension SwiftUI.Color {

    /// Initialize a `Color` with a color resource.
    init(_ resource: ColorResource) {
        self.init(resource.name, bundle: resource.bundle)
    }

}

@available(iOS 13.0, macOS 10.15, tvOS 13.0, watchOS 6.0, *)
extension SwiftUI.Image {

    /// Initialize an `Image` with an image resource.
    init(_ resource: ImageResource) {
        self.init(resource.name, bundle: resource.bundle)
    }

}
#endif