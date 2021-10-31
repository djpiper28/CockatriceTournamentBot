cmake_minimum_required(VERSION 3.12)

# set the project name
project(CockatriceTournamentBot)
set(REPO_URL "https://github.com/djpiper28/CockatriceTournamentBot")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# compiler flags
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wno-unused-parameter -Wno-writable-strings -DDEBUG=1")
set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
# -fsanitize=address,undefined ???
set(CMAKE_C_FLAGS_RELEASE "-O2 -Wno-unused-parameter -Wno-writable-strings")
set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake)

if (CMAKE_BUILD_TYPE STREQUAL "TEST")
    include(CodeCoverage)
    
    # Setup coverage
    set(CMAKE_C_COMPILER gcc)
    set(CMAKE_CXX_COMPILER g++)
    
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG} -DMEGA_DEBUG=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -DMEGA_DEBUG=1")
    
    append_coverage_compiler_flags()
    #setup_target_for_coverage_gcovr_html
    setup_target_for_coverage_gcovr_xml(NAME coverage
        EXECUTABLE CockatriceTournamentBotTests
        EXCLUDE "build/*" "tests/*" "src/mongoose.c" "src/mongoose.h" "src/main.c")
endif()

# gendocs generates doc source files from html with helpToSrc.py
set(HELPTOSOURCE_PY "${CMAKE_SOURCE_DIR}/helpToSrc.py")
add_custom_target(gendocs ${HELPTOSOURCE_PY})

# make proto files
add_subdirectory(pb)
set(PB_DIR "${CMAKE_CURRENT_BINARY_DIR}/pb")

# protobuf requires c files to be treated as c++
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -xc++")

# set mbedtls libs
set(MBEDTLS_LIBS -lcrypto -lmbedtls -lmbedcrypto -lmbedx509)

# mongoose
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMG_ENABLE_IPV6=1")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMG_ENABLE_LINES=1")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMG_ENABLE_DIRECTORY_LISTING=1")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMG_ENABLE_FS=1")

# enable mbedtls in mongoose
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMG_ENABLE_MBEDTLS=1")

add_library(mongoose src/mongoose.c)

# src
set(MAIN
    src/main.c
    )
set(SRC_FILES
    src/api_server.c
    src/bot.c
    src/bot_conf.c
    src/cmd_queue.c
    src/game_struct.c
    src/trice_structs.c
    src/player_deck_info.c
    src/commands.c
    )
add_executable(CockatriceTournamentBot ${SRC_FILES} ${MAIN})

# Copy static content (images, favicon, etc.)
file(GLOB STATIC_IMG
  "src/www/img/**"
)
file(COPY "src/www/favicon.ico" DESTINATION "${CMAKE_BINARY_DIR}/static/")
file(COPY ${STATIC_IMG} DESTINATION "${CMAKE_BINARY_DIR}/static/img/")

target_include_directories(CockatriceTournamentBot PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR} ${PB_DIR})
target_link_libraries(CockatriceTournamentBot
    proto mongoose ${Protobuf_LIBRARIES} ${MBEDTLS_LIBS})
add_dependencies(CockatriceTournamentBot gendocs)

if (CMAKE_BUILD_TYPE STREQUAL "TEST")
    # tests, set the flags manually
    set(CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_DEBUG})
    set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_DEBUG})

    set (TEST_FILES
        ${SRC_FILES}
        tests/runner.cpp
        tests/testTriceStructs.cpp
        tests/testBot.cpp
        tests/testBotConfig.cpp
        tests/testCmdQueue.cpp
        tests/testGameStruct.cpp
        tests/testApiServer.cpp
        tests/testPlayerDeckInfo.cpp
        tests/testCommands.cpp
        )
    add_executable(CockatriceTournamentBotTests ${TEST_FILES})
    target_include_directories(CockatriceTournamentBotTests PUBLIC 
        ${CMAKE_CURRENT_BINARY_DIR} ${PB_DIR})
    target_link_libraries(CockatriceTournamentBotTests
        proto mongoose cppunit ${Protobuf_LIBRARIES} ${MBEDTLS_LIBS})
    add_dependencies(CockatriceTournamentBotTests gendocs)
    add_test(NAME CockatriceTournamentBotTests COMMAND CockatriceTournamentBotTests)
endif()

# packaging?
#set(CPACK_PACKAGE_CONTACT "")
#set(CPACK_PACKAGE_DESCRIPTION_SUMMARY CockatriceTournamentBot)
#set(CPACK_PACKAGE_VENDOR "")
#set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")
#set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
#set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
#set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
#set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
#set(CPACK_PACKAGE_FILE_NAME CockatriceTournamentBot)
#SET(CPACK_INSTALL_CMAKE_PROJECTS "CockatriceTournamentBot;CockatriceTournamentBot;ALL;/")
#IF(CPACK_GENERATOR STREQUAL "RPM")
#set(CPACK_RPM_PACKAGE_LICENSE "")
#set(CPACK_RPM_PACKAGE_REQUIRES "protobuf")
#set(CPACK_RPM_PACKAGE_GROUP "Amusements/Games")
#set(CPACK_RPM_PACKAGE_URL ${REPO_URL})
#ELSEIF(CPACK_GENERATOR STREQUAL "DEB")
#set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
#set(CPACK_DEBIAN_PACKAGE_SECTION "games")
#set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${REPO_URL})
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
#ENDIF()
#include(CPack)
