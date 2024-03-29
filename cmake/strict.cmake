OPTION(USE_STRICT "Enable strict build options." ON)
IF(USE_STRICT)
    ADD_DEFINITIONS(-Wall)
    ADD_DEFINITIONS(-Werror)
    ADD_DEFINITIONS(-Wfatal-errors)

    ADD_DEFINITIONS(-Wno-unused-private-field)
    ADD_DEFINITIONS(-Wno-unused-function)
ENDIF()
