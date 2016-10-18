import sys
import string
import os

def color_out(__env):
    # Enable color building message output
    colors = {}
    if GetOption('use_colors'):
            colors['purple'] = '\033[95m'
            colors['blue']   = '\033[94m'
            colors['yellow'] = '\033[93m'
            colors['red']    = '\033[91m'
            colors['end']    = '\033[0m'
    else:
            colors['purple'] = ''
            colors['blue']   = ''
            colors['yellow'] = ''
            colors['red']    = ''
            colors['end']    = ''


    compile = '%sCompiling %s==> %s$SOURCE%s' % \
    (colors['blue'], colors['purple'], colors['yellow'], colors['end'])
    
    link    = '%sLinking Program %s==> %s$TARGET%s' % \
    (colors['red'], colors['purple'], colors['yellow'], colors['end'])

    link_lib= '%sLinking Static Library %s==> %s$TARGET%s' % \
    (colors['red'], colors['purple'], colors['yellow'], colors['end'])

    # Set default values
    __env['CXXCOMSTR']  = compile
    __env['CCCOMSTR']   = compile
    __env['ARCOMSTR']   = link_lib
    __env['LINKCOMSTR'] = link    

# Print a logo and a project name

def assign_opt():
    AddOption('--colors',
          dest='use_colors', action='store_true',
          default=True,
          help='Enable colored build output')
          
    AddOption('--lamepath',
          dest='lamepath', action='store',
          nargs=1,
          help='Specify path to mp3lame library')

# Process users options and generate target description
def proc_opt():
    global genv

    # Make output colorfull
    if genv.GetOption('use_colors'):
        color_out(genv)

######################## MAIN ###################################
# Get global environment
genv = Environment(ENV = os.environ, tools=['gcc', 'gnulink'])

# Set number of jobs
num_cpu = int(os.environ.get('NUM_CPU', 4))
SetOption('num_jobs', num_cpu)
CacheDir('./build/cache')

# Install and assign available options
assign_opt()
proc_opt()

lp = genv.GetOption('lamepath')

genv.SConscript('SConscript', variant_dir='./build/', duplicate=0, exports='genv lp')