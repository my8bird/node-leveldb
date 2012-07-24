# Copyright (c) 2011 The LevelDB Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file. See the AUTHORS file for names of contributors.

{
  'variables': {
    'conditions': [
      # Define an "os_include" variable that points at the OS-specific generated
      # headers.  These were generated by running the configure script offline.
      ['OS != "win32" and OS != "mac"', {
        'os_include': '../../port/snappy/linux'
      }],
      ['OS=="mac"', {'os_include': '../../port/snappy/mac'}],
      ['OS=="win"', {'os_include': '../../port/snappy/win32'}],
    ],
  },
  'targets': [
    {
      'target_name': 'snappy',
      'type': 'static_library',
      'include_dirs': [
        '<(os_include)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(os_include)',
        ],
      },
      'sources': [
        'snappy-internal.h',
        'snappy-sinksource.cc',
        'snappy-sinksource.h',
        'snappy-stubs-internal.cc',
        'snappy-stubs-internal.h',
        'snappy.cc',
        'snappy.h',
      ],
    },
    {
      'target_name': 'snappy_unittest',
      'type': 'executable',
      'dependencies': [
        'snappy',
      ],
      'sources': [
        'snappy-test.cc',
        'snappy-test.h',
        'snappy_unittest.cc',
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
