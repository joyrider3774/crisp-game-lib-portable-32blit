# Basic parameters; check that these match your project / environment
cmake_minimum_required(VERSION 3.9)

# Replace "game" with a name for your project (this is used the name of the output)
project(Cglp)

# Add your sources here (adding headers is optional, but helps some CMake generators)
set(PROJECT_SOURCE cglp32blit.cpp 
	lib/cglp.c lib/menu.c lib/menuGameList.c lib/particle.c lib/random.c lib/sound.c lib/textPattern.c lib/vector.c
	games/gameBallTour.c
	games/gameCastN.c
	games/gameColorRoll.c
	games/gameFroooog.c
	games/gameGrapplingH.c 
	games/gameHexmin.c
	games/gameInTow.c
	games/gameLadderDrop.c
	games/gameLadderDrop.c
	games/gamePakuPaku.c
	games/gamePinClimb.c
	games/gameReflector.c
	games/gameRWheel.c
	games/gameSurvivor.c
	games/gameThunder.c 
	games/gameTimberTest.c 
	games/gameAccelB.c
	games/gameAeralBar.c
	games/gameAntLion.c
	games/gameBalance.c
	games/gameBallsBombs.c
	games/gameBallTour.c
	games/gameBamboo.c
	games/gameBaroll.c
	games/gameBBlast.c
	games/gameBCannon.c
	games/gameBombUp.c
	games/gameBreedC.c
	games/gameBSFish.c
	games/gameBWalls.c
	games/gameCardQ.c
	games/gameCastN.c
	games/gameCatapult.c
	games/gameCateP.c
	games/gameChargeBeam.c	
	games/gameCNodes.c
	games/gameColorRoll.c
	games/gameCounterB.c
	games/gameCrossLine.c
	games/gameCTower.c
	games/gameDarkCave.c
	games/gameDescents.c
	games/gameDFight.c
	games/gameDivarr.c
	games/gameDLaser.c
	games/gameFroooog.c
	games/gameGrapplingH.c
	games/gameHexmin.c
	games/gameInTow.c
	games/gameLadderDrop.c
	games/gamePakuPaku.c
	games/gamePinClimb.c
	games/gameReflector.c
	games/gameRWheel.c
	games/gameSurvivor.c
	games/gameThunder.c
	games/gameTimberTest.c
	games/gameTwoFaced.c
	games/gameFracave.c
	)

	#broken uses needs changes to lib that requires too much memory
	#games/gameCircleW.c
	

include_directories(PRIVATE lib)

# ... and any other files you want in the release here
set(PROJECT_DISTRIBS LICENSE README.md)

# Build configuration; approach this with caution!
if(MSVC)
  add_compile_options("/W4" "/wd4244" "/wd4324" "/wd4458" "/wd4100")
else()
  add_compile_options("-Wall" "-Wextra" "-Wdouble-promotion" "-Wno-unused-parameter")
endif()

find_package (32BLIT CONFIG REQUIRED PATHS ../32blit-sdk $ENV{PATH_32BLIT_SDK})

blit_executable (${PROJECT_NAME} ${PROJECT_SOURCE})
blit_assets_yaml (${PROJECT_NAME} assets.yml)
blit_metadata (${PROJECT_NAME} metadata.yml)
add_custom_target (flash DEPENDS ${PROJECT_NAME}.flash)

# setup release packages
install (FILES ${PROJECT_DISTRIBS} DESTINATION .)
set (CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set (CPACK_GENERATOR "ZIP" "TGZ")
include (CPack)
