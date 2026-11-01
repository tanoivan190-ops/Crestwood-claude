cmake_minimum_required(VERSION 3.12)
project(samp)

set(CMAKE_CXX_STANDARD 20)

if(${ANDROID_ABI} STREQUAL "armeabi-v7a")
    add_definitions(-DVER_x32=true)
    set(GLOSSHOOK_ABI "ARM")
    add_definitions(-DHAS_CEF=1)
elseif(${ANDROID_ABI} STREQUAL "arm64-v8a")
    add_definitions(-DVER_x32=false)
    set(GLOSSHOOK_ABI "ARM64")
    # libSAMPMobileCef.a n'existe qu'en ARM 32-bit (ELF32), pas de variante arm64.
    # On désactive le module CEF pour cet ABI plutôt que de faire planter le lien.
endif()

file(GLOB_RECURSE SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/**/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/**/**/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/**/**/**/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/**/**/**/**/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/**/**/**/**/**/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/**/**/**/**/**/**/**/*.c*
)


# Ignore les copies "fichier(1).cpp" / "fichier (2).h" créées par un copier-coller ou un
# téléchargement : elles définissent les mêmes symboles et provoquent "duplicate symbol" au link.
list(FILTER SOURCES EXCLUDE REGEX "[ ]?\\([0-9]+\\)\\.[^/]*$")

# Fichiers fantomes : restes de l'ancienne structure "client unifie" (ex-samp32 copie dans samp/)
# qui traînent dans le repo quand on pousse le zip "retour 2.11" par-dessus sans supprimer les anciens.
#  - voice_new/ utilise FLog (inexistant dans ce client 64 bits) -> "use of undeclared identifier 'FLog'"
#    puis "duplicate symbol" (Channel, Effect, VoicePacket, ControlPacket existent deja dans voice/).
#  - les autres redefinissent des symboles deja presents dans ce client.
# Le vrai client 32 bits est dans samp32/. Ces chemins n'existent pas dans le source "retour 2.11".
list(FILTER SOURCES EXCLUDE REGEX "/samp/(voice_new/|game_sa/|nv_event\\.cpp$|game/(CRenderTarget|ES2VertexBuffer|RQ_Commands|Shadows|multitouch|opengl)\\.cpp$)")
list(REMOVE_DUPLICATES SOURCES)

include_directories(
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/util/
        ${CMAKE_CURRENT_LIST_DIR}/vendor/armhook
        ${CMAKE_CURRENT_LIST_DIR}/vendor/imgui
        ${CMAKE_CURRENT_LIST_DIR}/vendor/
        ${CMAKE_CURRENT_LIST_DIR}/game/RW/
        ${CMAKE_CURRENT_LIST_DIR}/game/Core/
)

add_library(samp SHARED ${SOURCES})
target_link_libraries(samp
        log
        android
        EGL
        GLESv3
        opus
        shadowhook::shadowhook
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/bass/libs/${ANDROID_ABI}/libbass.so
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/bass/libs/${ANDROID_ABI}/libbass_ssl.so
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/GlossHook/libs/${GLOSSHOOK_ABI}/libGlossHook.a
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/GlossHook/libs/${GLOSSHOOK_ABI}/libGlossHook.so
)

if(${ANDROID_ABI} STREQUAL "armeabi-v7a")
    add_library(SAMPMobileCef STATIC IMPORTED)
    set_target_properties(SAMPMobileCef PROPERTIES
            IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/vendor/cef/libSAMPMobileCef.a
    )
    target_link_libraries(samp SAMPMobileCef)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -s -g -fvisibility=hidden -pthread -Wall -O3 -fexceptions -fstack-protector-strong")
target_compile_options(samp PRIVATE
        $<$<CONFIG:Debug>:-fvisibility=default>
        $<$<CONFIG:Release>:-fvisibility=hidden>
)
target_link_options(samp PRIVATE "-Wl,-z,max-page-size=16384")

