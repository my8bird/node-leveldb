# Copyright (c) 2011 The LevelDB Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file. See the AUTHORS file for names of contributors.

{
  'target_defaults': {
    'defines': [
      'LEVELDB_PLATFORM_POSIX=1'
    ],
    'include_dirs': [
      '.',
      'include/',
    ],
    'conditions': [
      ['OS == "win"', {
        'include_dirs': [
          'port/win',
        ],
      }],
      ['OS == "mac"', {
        'defines': [
          'OS_MACOSX=1',
          'PLATFORM=OS_MACOSX'
        ]
      }],
    ],
  },
  'targets': [
    {
      'target_name': 'snappy',
      'type': 'static_library',
      'dependencies': [
      ],
      'conditions': [
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'include/',
        ],
        'conditions': [
          ['OS == "win"', {
            'include_dirs': [
              'port/win',
            ],
          }],
        ],
      },
      'sources': [
        # Include and then exclude so that all files show up in IDEs, even if
        # they don't build.
        'snappy.cc',
        'snappy-sinksource.cc',
        'snappy-stubs-internal.cc',
        'snappy-c.cc'
      ],
    },
    {
      'target_name': 'snappy_unittest',
      'type': 'executable',
      'dependencies': [
        'snappy',
      ],
      'sources': [
        'snappy_unittest.cc',
	'snappy-test.cc'
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
