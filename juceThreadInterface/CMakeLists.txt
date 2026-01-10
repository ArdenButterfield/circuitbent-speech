project(JuceThreadInterface)

add_library(${PROJECT_NAME} juiceThreadInterface.cpp)

target_link_libraries(${PROJECT_NAME} INTERFACE
        juce_audio_utils
        juce_audio_processors
        juce_dsp
        juce_gui_basics
        juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
