project('aawordsearch',
 'c',
  version : '0.1.5.999',
  meson_version : '>= 0.49.0',
  default_options : ['warning_level=3'])

cc = meson.get_compiler('c')

extra_flags = [
  '-fno-common',
  '-fanalyzer',
  '-Wformat-security',
  '-Wformat-overflow=2',
  '-Wstrict-overflow=5',
  '-DPROGRAM_NAME="@0@"'.format(meson.project_name()),
  '-DVERSION="@0@"'.format(meson.project_version())
]

deps = []
dep_curl = dependency(
  'libcurl',
  required:false
  )
if dep_curl.found()
  extra_flags += '-DHAVE_CURL'
  deps += dep_curl
endif

foreach cflag : extra_flags
  if cc.has_argument(cflag)
    add_project_arguments(cflag, language: 'c')
  endif
endforeach

src = 'aawordsearch.c'

executable(
  meson.project_name(),
  src,
  dependencies: deps
  )

vg = find_program('valgrind', required : false)

if vg.found()
add_test_setup('valgrind',
  exe_wrapper : [vg, '--error-exitcode=1', '--leak-check=full'],
# env : env
  )
endif

test_bin_name = 'test_'+meson.project_name()
e = executable(test_bin_name, src, c_args : ['-DTEST'])
test(test_bin_name, e)
