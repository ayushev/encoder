import os
import sys
import glob

sources = []
includes = []
prj_path = './'

Import('genv')

# Function to find already included files in the source list
def add_sources(srcs):
    global sources
    global genv

    for src in srcs:
        for tmp_src in genv.Glob(src):
            try:
                sources.remove(File(tmp_src))
            except ValueError:
                pass
            sources.append(File(tmp_src))

# Function to find already included files in the include list
def add_includes(incs):
    global includes
    global genv
    
    for inc in incs:
        incpath = os.path.dirname(inc)
    
        try:
            includes.remove(str(incpath))
        except ValueError:
            pass
        includes.append(str(incpath))
    
        # Compile program
        genv.MergeFlags({'CPPPATH' : includes})

################################################################################
############################## MAIN APPLICATION ################################
################################################################################
trg = 'encoder'
conf = {
    'CFLAGS'  : ['-Wall',
                 '-O0',
                 '-g3',
                 '-Werror-implicit-function-declaration',
                 '-ffunction-sections', 
                 '-fdata-sections',
                 '-Wno-format',
                 '-Wno-comment',
                 '-std=gnu11' ],
    'CPPDEFINES':['E4C_THREADSAFE',
                  'E4C_NOKEYWORDS'
    ],
    'LINKFLAGS':[ '-Wl,--gc-sections',
    ],
    'LIBS'    : [ 'pthread',
                  'mp3lame'
    ],
    'LIBPATH' : [ './'
    ]
}
genv.MergeFlags(conf)

incs = [ prj_path + 'inc/*'
]

if sys.platform == 'linux':
    os_src = 'os/os_posix.c'
	
if sys.platform == 'win32':
	os_src = 'os/os_win.c'

srcs =  [ prj_path + '*.c',
          os_src
]
add_sources(srcs)
add_includes(incs)

genv.Program(target = trg, source = sources)
