enable_language(ASM)

aux_source_directory(. DIR_SRCS)

FILE(GLOB_RECURSE  ASM_SRC *.S)


list(APPEND DIR_SRCS ${ASM_SRC})

add_library(coroutine ${DIR_SRCS})
