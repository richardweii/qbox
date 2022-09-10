FILE(GLOB_RECURSE  test_files *_test.cc)

foreach(test_file ${test_files})
  add(${test_file})
endforeach(test_file)