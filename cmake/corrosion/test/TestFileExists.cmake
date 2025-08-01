# CMake script to test if a file exists. Errors if the file does not exist.
# Expect actual arguments to start at index 3 (cmake -P <script_name>)

# Expect one argument
if(NOT (CMAKE_ARGC EQUAL "4"))
    message(FATAL_ERROR "Test Internal Error: Unexpected ARGC Value: ${CMAKE_ARGC}.")
endif()

set(FILE_PATH "${CMAKE_ARGV3}")

if(NOT ( EXISTS "${FILE_PATH}" ))
    set(error_details "File `${FILE_PATH}` does not exist!\n")
    set(PARENT_TREE "${FILE_PATH}")
    cmake_path(HAS_PARENT_PATH PARENT_TREE has_parent)
    while(has_parent)
        cmake_path(GET PARENT_TREE PARENT_PATH PARENT_TREE)
        cmake_path(HAS_PARENT_PATH PARENT_TREE has_parent)
        if(EXISTS "${PARENT_TREE}")
            file(GLOB dir_contents LIST_DIRECTORIES true "${PARENT_TREE}/*")
            list(APPEND error_details "Found Parent directory `${PARENT_TREE}` exists and contains:\n" ${dir_contents})
            break()
        else()
            list(APPEND error_details "Parent directory `${PARENT_TREE}` also does not exist!")
        endif()
    endwhile()
    message(FATAL_ERROR "Test failed: ${error_details}")
endif()
