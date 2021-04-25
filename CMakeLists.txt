cmake_minimum_required(VERSION 3.12)

# set the project name
project(CockatriceTournamentBot)
set(REPO_URL "https://github.com/djpiper28/CockatriceTournamentBot")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# compiler flags
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Wextra -DDEBUG=1 -DMEGA_DEBUG=1")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -DDEBUG=1 -DMEGA_DEBUG=1")
set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# gendocs generates doc source files from html with helpToSrc.py
set(HELPTOSOURCE_PY "${CMAKE_SOURCE_DIR}/helpToSrc.py")
add_custom_target(gendocs ${HELPTOSOURCE_PY})

# make proto files
add_subdirectory(pb)
set(PB_DIR "${CMAKE_CURRENT_BINARY_DIR}/pb")

# protobuf requires c files to be treated as c++
set(CMAKE_C_FLAGS "-xc++")

# set mbedtls libs
set(MBEDTLS_LIBS -lcrypto -lmbedtls -lmbedcrypto -lmbedx509)

# mongoose
set(MG_ENABLE_MBEDTLS 1)
set(MG_ENABLE_IPV6 1)
set(MG_ENABLE_LINES 1)
set(MG_ENABLE_DIRECTORY_LISTING 1)
add_library(mongoose src/mongoose.c)

# src
set(SRC_FILES
    src/apiserver.cpp
    src/bot.cpp
    src/botconf.c
    src/cmd_queue.c
    src/gamestruct.c
    src/main.c
    src/trice_structs.c
    )
add_executable(CockatriceTournamentBot ${SRC_FILES})
target_include_directories(CockatriceTournamentBot PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR} ${PB_DIR})
target_link_libraries(CockatriceTournamentBot
    proto mongoose ${Protobuf_LIBRARIES} ${MBEDTLS_LIBS})
add_dependencies(CockatriceTournamentBot gendocs)

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