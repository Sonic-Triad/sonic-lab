set(SONIC_LAB_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

# Adds an Audio Unit with the settings shared by every Sonic Lab plugin.
#
#   triad_add_plugin(<Target>
#       PRODUCT_NAME  "Triad Something"   # name shown in GarageBand
#       VERSION       1.0.0
#       PLUGIN_CODE   Abc1                # 4 chars, unique per plugin, at least one uppercase
#       BUNDLE_SUFFIX something           # bundle ID becomes tech.triad.<suffix>
#       [AU_MAIN_TYPE kAudioUnitType_Effect]
#       SOURCES       Source/PluginProcessor.cpp ...)
function(triad_add_plugin target)
    cmake_parse_arguments(ARG "" "PRODUCT_NAME;VERSION;PLUGIN_CODE;BUNDLE_SUFFIX;AU_MAIN_TYPE" "SOURCES" ${ARGN})

    if(NOT ARG_AU_MAIN_TYPE)
        set(ARG_AU_MAIN_TYPE kAudioUnitType_Effect)
    endif()

    juce_add_plugin(${target}
        VERSION "${ARG_VERSION}"
        PRODUCT_NAME "${ARG_PRODUCT_NAME}"
        COMPANY_NAME "Triad"
        BUNDLE_ID "tech.triad.${ARG_BUNDLE_SUFFIX}"
        PLUGIN_MANUFACTURER_CODE Trdt
        PLUGIN_CODE ${ARG_PLUGIN_CODE}
        FORMATS AU
        AU_MAIN_TYPE ${ARG_AU_MAIN_TYPE}
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE)

    target_sources(${target} PRIVATE ${ARG_SOURCES})

    target_compile_definitions(${target}
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0)

    target_link_libraries(${target}
        PRIVATE
            triad_shared
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    if(SONIC_LAB_BUILD_SNAPSHOT_TOOL)
        juce_add_console_app(${target}Snapshot PRODUCT_NAME "${target}Snapshot")

        target_sources(${target}Snapshot
            PRIVATE
                "${SONIC_LAB_ROOT}/tools/EditorSnapshot/EditorSnapshot.cpp"
                ${ARG_SOURCES})

        target_compile_definitions(${target}Snapshot
            PRIVATE
                JucePlugin_Name="${ARG_PRODUCT_NAME}"
                JUCE_WEB_BROWSER=0
                JUCE_USE_CURL=0)

        target_link_libraries(${target}Snapshot
            PRIVATE
                triad_shared
                juce::juce_audio_utils
                juce::juce_dsp
                juce::juce_recommended_config_flags
                juce::juce_recommended_warning_flags)
    endif()
endfunction()
