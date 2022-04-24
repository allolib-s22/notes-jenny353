if(NOT EXISTS "/Users/jenniferlo/cs/cs190D/lab01-jenny353/allolib/build/Release/external/rtmidi/install_manifest.txt")
  message(FATAL_ERROR "Cannot find install manifest: \"/Users/jenniferlo/cs/cs190D/lab01-jenny353/allolib/build/Release/external/rtmidi/install_manifest.txt\"")
endif(NOT EXISTS "/Users/jenniferlo/cs/cs190D/lab01-jenny353/allolib/build/Release/external/rtmidi/install_manifest.txt")

file(READ "/Users/jenniferlo/cs/cs190D/lab01-jenny353/allolib/build/Release/external/rtmidi/install_manifest.txt" files)
string(REGEX REPLACE "\n" ";" files "${files}")
foreach(file ${files})
  message(STATUS "Uninstalling \"$ENV{DESTDIR}${file}\"")
  if(EXISTS "$ENV{DESTDIR}${file}")
    exec_program(
      "/usr/local/Cellar/cmake/3.23.0/bin/cmake" ARGS "-E remove \"$ENV{DESTDIR}${file}\""
      OUTPUT_VARIABLE rm_out
      RETURN_VALUE rm_retval
      )
    if(NOT "${rm_retval}" STREQUAL 0)
      message(FATAL_ERROR "Problem when removing \"$ENV{DESTDIR}${file}\"")
    endif(NOT "${rm_retval}" STREQUAL 0)
  else(EXISTS "$ENV{DESTDIR}${file}")
    message(STATUS "File \"$ENV{DESTDIR}${file}\" does not exist.")
  endif(EXISTS "$ENV{DESTDIR}${file}")
endforeach(file)
