cmake_minimum_required(VERSION 2.8)
project (cld2)
enable_language(CXX)

set (VERSION "0.0.197")
set (common_SOURCE_FILES
    internal/cldutil.cc
    internal/cldutil_shared.cc
    internal/compact_lang_det.cc
    internal/compact_lang_det_hint_code.cc
    internal/compact_lang_det_impl.cc
    internal/debug.cc
    internal/fixunicodevalue.cc 
    internal/generated_entities.cc
    internal/generated_language.cc
    internal/generated_ulscript.cc
    internal/getonescriptspan.cc
    internal/lang_script.cc
    internal/offsetmap.cc
    internal/scoreonescriptspan.cc
    internal/tote.cc
    internal/utf8statetable.cc
    )

set (cld2_SOURCE_FILES
    internal/generated_distinct_bi_0.cc
    internal/cld_generated_cjk_uni_prop_80.cc
    internal/cld2_generated_cjk_compatible.cc
    internal/cld_generated_cjk_delta_bi_4.cc
    internal/cld2_generated_quadchrome_2.cc
    internal/cld2_generated_deltaoctachrome.cc
    internal/cld2_generated_distinctoctachrome.cc
    internal/cld_generated_score_quad_octa_2.cc
    )

set (cld2_full_SOURCE_FILES
    internal/generated_distinct_bi_0.cc
    internal/cld_generated_cjk_uni_prop_80.cc
    internal/cld2_generated_cjk_compatible.cc
    internal/cld_generated_cjk_delta_bi_32.cc
    internal/cld2_generated_quad0122.cc
    internal/cld2_generated_deltaocta0122.cc
    internal/cld2_generated_distinctocta0122.cc
    internal/cld_generated_score_quad_octa_0122.cc
    )

set (cld2_dynamic_SOURCE_FILES
    internal/cld2_dynamic_data.cc
    internal/cld2_dynamic_data_loader.cc
    )

add_library(cld2 SHARED ${common_SOURCE_FILES} ${cld2_SOURCE_FILES})
set_target_properties(cld2 PROPERTIES
  ENABLE_EXPORTS On
  OUTPUT_NAME cld2
  VERSION ${VERSION}
  SOVERSION 0
  )
add_library(cld2_full SHARED ${cld2_full_SOURCE_FILES})
set_target_properties(cld2_full PROPERTIES
  ENABLE_EXPORTS On
  OUTPUT_NAME cld2_full
  VERSION ${VERSION}
  SOVERSION 0
  )

add_library(cld2_dynamic SHARED ${cld2_dynamic_SOURCE_FILES})
set_target_properties(cld2_dynamic PROPERTIES
  ENABLE_EXPORTS On
  OUTPUT_NAME cld2_dynamic
  VERSION ${VERSION}
  SOVERSION 0
  COMPILE_FLAGS "-DCLD2_DYNAMIC_MODE"
  )
install(TARGETS cld2 DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE})
install(TARGETS cld2_full DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE})
install(TARGETS cld2_dynamic DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE})

set (cld2_internal_HEADERS
    internal/cld2_dynamic_compat.h
    internal/cld2_dynamic_data_extractor.h
    internal/cld2_dynamic_data.h
    internal/cld2_dynamic_data_loader.h
    internal/cld2tablesummary.h
    internal/cldutil.h
    internal/cldutil_offline.h
    internal/cldutil_shared.h
    internal/compact_lang_det_hint_code.h
    internal/compact_lang_det_impl.h
    internal/debug.h
    internal/fixunicodevalue.h
    internal/generated_language.h
    internal/generated_ulscript.h
    internal/getonescriptspan.h
    internal/integral_types.h
    internal/lang_script.h
    internal/langspan.h
    internal/offsetmap.h
    internal/port.h
    internal/scoreonescriptspan.h
    internal/stringpiece.h
    internal/tote.h
    internal/unittest_data.h
    internal/utf8acceptinterchange.h
    internal/utf8prop_lettermarkscriptnum.h
    internal/utf8repl_lettermarklower.h
    internal/utf8scannot_lettermarkspecial.h
    internal/utf8statetable.h
    )

install(FILES ${cld2_internal_HEADERS} DESTINATION include/cld2/internal)
set (cld2_public_HEADERS
    public/compact_lang_det.h
    public/encodings.h
    )
install(FILES ${cld2_public_HEADERS} DESTINATION include/cld2/public)

set (full_SOURCE_FILES
    internal/cld_generated_cjk_uni_prop_80.cc
    internal/cld2_generated_cjk_compatible.cc
    internal/cld_generated_cjk_delta_bi_32.cc
    internal/generated_distinct_bi_0.cc
    internal/cld2_generated_quad0122.cc
    internal/cld2_generated_deltaocta0122.cc
    internal/cld2_generated_distinctocta0122.cc
    internal/cld_generated_score_quad_octa_0122.cc
    )

add_executable(compact_lang_det_test_full ${full_SOURCE_FILES} internal/compact_lang_det_test.cc)
add_executable(cld2_unittest_full         ${full_SOURCE_FILES} internal/cld2_unittest_full.cc)
add_executable(cld2_unittest_full_avoid   ${full_SOURCE_FILES} internal/cld2_unittest_full.cc)
set_target_properties(cld2_unittest_full_avoid PROPERTIES COMPILE_FLAGS "-Davoid_utf8_string_constants")

add_executable(cld2_dynamic_data_tool               internal/cld2_dynamic_data_extractor.cc internal/cld2_dynamic_data_tool.cc)
add_executable(compact_lang_det_dynamic_test_chrome ${common_SOURCE_FILES} internal/cld2_dynamic_data_extractor.cc internal/compact_lang_det_test.cc)
add_executable(cld2_dynamic_unittest                ${common_SOURCE_FILES} internal/cld2_unittest.cc)
set_target_properties(compact_lang_det_dynamic_test_chrome PROPERTIES COMPILE_FLAGS "-DCLD2_DYNAMIC_MODE")
set_target_properties(cld2_dynamic_unittest PROPERTIES COMPILE_FLAGS "-DCLD2_DYNAMIC_MODE")

add_executable(compact_lang_det_test_chrome_2  internal/compact_lang_det_test.cc)
add_executable(compact_lang_det_test_chrome_16 internal/compact_lang_det_test.cc)
add_executable(cld2_unittest_chrome_2          internal/cld2_unittest.cc)
add_executable(cld2_unittest_avoid_chrome_2    internal/cld2_unittest.cc)
set_target_properties(cld2_unittest_avoid_chrome_2 PROPERTIES COMPILE_FLAGS "-Davoid_utf8_string_constants")

target_link_libraries(compact_lang_det_test_full cld2)
target_link_libraries(cld2_unittest_full cld2)
target_link_libraries(cld2_unittest_full_avoid cld2)
target_link_libraries(cld2_dynamic_data_tool cld2 cld2_dynamic)
target_link_libraries(compact_lang_det_dynamic_test_chrome cld2_dynamic)
target_link_libraries(cld2_dynamic_unittest cld2_dynamic)
target_link_libraries(compact_lang_det_test_chrome_2 cld2)
target_link_libraries(compact_lang_det_test_chrome_16 cld2)
target_link_libraries(cld2_unittest_chrome_2 cld2)
target_link_libraries(cld2_unittest_avoid_chrome_2 cld2)
